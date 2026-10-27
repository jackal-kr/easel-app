/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "rich-text.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- A growing byte buffer ---- */

typedef struct
{
  char *p;
  size_t len, cap;
  bool failed;
} Buf;

static void
buf_add (Buf *b, const char *s, size_t n)
{
  if (b->failed)
    return;
  if (n > SIZE_MAX - b->len - 1)
    {
      b->failed = true;
      return;
    }
  if (b->len + n + 1 > b->cap)
    {
      size_t cap = b->cap ? b->cap : 256;
      char *q;

      while (cap < b->len + n + 1)
        {
          if (cap > SIZE_MAX / 2)
            {
              b->failed = true;
              return;
            }
          cap *= 2;
        }
      q = realloc (b->p, cap);
      if (q == NULL)
        {
          b->failed = true;
          return;
        }
      b->p = q;
      b->cap = cap;
    }
  memcpy (b->p + b->len, s, n);
  b->len += n;
  b->p[b->len] = '\0';
}

static void
buf_str (Buf *b, const char *s)
{
  buf_add (b, s, strlen (s));
}

/* ---- Writing ---- */

/* `s` (n bytes) with &, <, > and " escaped; '\n' as <br>. */
static void
add_escaped (Buf *b, const char *s, size_t n)
{
  for (size_t i = 0; i < n; i++)
    switch (s[i])
      {
      case '&':
        buf_str (b, "&amp;");
        break;
      case '<':
        buf_str (b, "&lt;");
        break;
      case '>':
        buf_str (b, "&gt;");
        break;
      case '"':
        buf_str (b, "&quot;");
        break;
      case '\n':
        buf_str (b, "<br>");
        break;
      default:
        buf_add (b, &s[i], 1);
      }
}

char *
easel_rich_text_to_html (const char *text, size_t len, const EaselTextRun *runs, size_t n, size_t *out_len)
{
  Buf b = { 0 };

  buf_str (&b, "<meta charset=\"utf-8\">");
  for (size_t i = 0; i < n; i++)
    {
      const EaselTextStyle *s = &runs[i].style;
      size_t from = runs[i].start, to = i + 1 < n ? runs[i + 1].start : len;
      char tmp[96];
      unsigned a = s->color >> 24;

      if (from >= to || to > len)
        continue;
      buf_str (&b, "<span style=\"font-family:'");
      for (const char *f = s->family; *f != '\0'; f++)
        if (*f != '\'')
          add_escaped (&b, f, 1);
      if (a == 255)
        snprintf (tmp, sizeof tmp, "';font-size:%dpt;color:#%06x", s->size_pt, s->color & 0xFFFFFFu);
      else
        snprintf (tmp, sizeof tmp, "';font-size:%dpt;color:rgba(%u,%u,%u,%.3f)", s->size_pt, (s->color >> 16) & 255,
                  (s->color >> 8) & 255, s->color & 255, a / 255.0);
      buf_str (&b, tmp);
      if (s->flags & EASEL_TEXT_BOLD)
        buf_str (&b, ";font-weight:bold");
      if (s->flags & EASEL_TEXT_ITALIC)
        buf_str (&b, ";font-style:italic");
      if ((s->flags & EASEL_TEXT_UNDERLINE) && (s->flags & EASEL_TEXT_STRIKE))
        buf_str (&b, ";text-decoration:underline line-through");
      else if (s->flags & EASEL_TEXT_UNDERLINE)
        buf_str (&b, ";text-decoration:underline");
      else if (s->flags & EASEL_TEXT_STRIKE)
        buf_str (&b, ";text-decoration:line-through");
      buf_str (&b, "\">");
      add_escaped (&b, text + from, to - from);
      buf_str (&b, "</span>");
    }
  if (b.failed)
    {
      free (b.p);
      return NULL;
    }
  *out_len = b.len;
  return b.p;
}

/* ---- Reading ---- */

typedef struct
{
  const char *family;
  int size_pt;
  bool has_color;
  uint32_t color;
  uint8_t flags;
} Style;

typedef struct
{
  char name[16];
  Style saved;
} Open;

typedef struct
{
  EaselRichText *rt;
  Buf text;
  size_t runs_cap, families_cap;
  Style style;                /* the style of the text read now */
  Open *stack;
  size_t depth, stack_cap;
  bool at_line_start;         /* nothing but collapsed whitespace since the start or the last line break */
  bool space_pending;         /* collapsed whitespace waiting for more text on the line */
  Style space_style;
  bool break_pending;         /* a block ended or began: a line break before more text */
  bool failed;
} Reader;

static bool
same (const Style *a, const Style *b)
{
  return a->family == b->family && a->size_pt == b->size_pt && a->has_color == b->has_color && (!a->has_color || a->color == b->color)
         && a->flags == b->flags;
}

static bool
grow (void **items, size_t *cap, size_t n, size_t size)
{
  size_t want = *cap ? *cap : 16;
  void *p;

  if (n <= *cap)
    return true;
  while (want < n)
    {
      if (want > SIZE_MAX / 2 / size)
        return false;
      want *= 2;
    }
  p = realloc (*items, want * size);
  if (p == NULL)
    return false;
  *items = p;
  *cap = want;
  return true;
}

/* The reader's own copy of a family name of `n` bytes; NULL without memory. */
static const char *
intern (Reader *r, const char *name, size_t n)
{
  EaselRichText *rt = r->rt;
  char *copy;

  for (size_t i = 0; i < rt->n_families; i++)
    if (strlen (rt->families[i]) == n && memcmp (rt->families[i], name, n) == 0)
      return rt->families[i];
  if (!grow ((void **) &rt->families, &r->families_cap, rt->n_families + 1, sizeof *rt->families) || (copy = malloc (n + 1)) == NULL)
    {
      r->failed = true;
      return NULL;
    }
  memcpy (copy, name, n);
  copy[n] = '\0';
  rt->families[rt->n_families++] = copy;
  return copy;
}

/* Appends `n` bytes of text in `style`. */
static void
emit (Reader *r, const char *s, size_t n, const Style *style)
{
  EaselRichText *rt = r->rt;
  size_t at = r->text.len;

  buf_add (&r->text, s, n);
  if (r->text.failed)
    {
      r->failed = true;
      return;
    }
  if (rt->n > 0)
    {
      EaselRichRun *last = &rt->runs[rt->n - 1];
      Style ls = { last->family, last->size_pt, last->has_color, last->color, last->flags };

      if (same (&ls, style))
        return;
    }
  if (!grow ((void **) &rt->runs, &r->runs_cap, rt->n + 1, sizeof *rt->runs))
    {
      r->failed = true;
      return;
    }
  rt->runs[rt->n++] = (EaselRichRun) { at, style->family, style->size_pt, style->has_color, style->has_color ? style->color : 0,
                                       style->flags };
}

/* Text that is not collapsible whitespace: a pending line break, then a pending space, first. */
static void
emit_text (Reader *r, const char *s, size_t n)
{
  if (r->break_pending)
    {
      if (r->text.len > 0 && r->text.p[r->text.len - 1] != '\n')
        emit (r, "\n", 1, &r->style);
      r->break_pending = false;
      r->space_pending = false;
      r->at_line_start = true;
    }
  if (r->space_pending && !r->at_line_start)
    emit (r, " ", 1, &r->space_style);
  r->space_pending = false;
  emit (r, s, n, &r->style);
  r->at_line_start = false;
}

static void
line_break (Reader *r)
{
  emit (r, "\n", 1, &r->style);
  r->space_pending = false;
  r->break_pending = false;
  r->at_line_start = true;
}

static void
block_edge (Reader *r)
{
  if (r->text.len > 0)
    r->break_pending = true;
  r->space_pending = false;
}

/* UTF-8 for code point `c` into out (4 bytes); its length. */
static size_t
utf8 (unsigned long c, char *out)
{
  if (c < 0x80)
    {
      out[0] = (char) c;
      return 1;
    }
  if (c < 0x800)
    {
      out[0] = (char) (0xC0 | (c >> 6));
      out[1] = (char) (0x80 | (c & 0x3F));
      return 2;
    }
  if (c < 0x10000)
    {
      out[0] = (char) (0xE0 | (c >> 12));
      out[1] = (char) (0x80 | ((c >> 6) & 0x3F));
      out[2] = (char) (0x80 | (c & 0x3F));
      return 3;
    }
  if (c > 0x10FFFF)
    c = 0xFFFD;
  out[0] = (char) (0xF0 | (c >> 18));
  out[1] = (char) (0x80 | ((c >> 12) & 0x3F));
  out[2] = (char) (0x80 | ((c >> 6) & 0x3F));
  out[3] = (char) (0x80 | (c & 0x3F));
  return 4;
}

/* An entity at s[0] == '&' within n bytes: its UTF-8 in out and the bytes it took; 0 if it is none. */
static size_t
entity (const char *s, size_t n, char *out, size_t *out_len)
{
  static const struct { const char *name; unsigned long c; } names[] = {
    { "amp", '&' }, { "lt", '<' }, { "gt", '>' }, { "quot", '"' }, { "apos", '\'' }, { "nbsp", 0xA0 },
  };
  size_t end = 1;

  while (end < n && end < 12 && s[end] != ';')
    end++;
  if (end >= n || s[end] != ';')
    return 0;
  if (s[1] == '#')
    {
      unsigned long c = 0;
      bool hex = end > 2 && (s[2] == 'x' || s[2] == 'X');

      for (size_t i = hex ? 3 : 2; i < end; i++)
        {
          int d = s[i] >= '0' && s[i] <= '9' ? s[i] - '0'
                  : hex && s[i] >= 'a' && s[i] <= 'f' ? s[i] - 'a' + 10
                  : hex && s[i] >= 'A' && s[i] <= 'F' ? s[i] - 'A' + 10
                                                       : -1;

          if (d < 0)
            return 0;
          c = c * (hex ? 16 : 10) + (unsigned long) d;
          if (c > 0x10FFFF)
            c = 0xFFFD;
        }
      *out_len = utf8 (c, out);
      return end + 1;
    }
  for (size_t k = 0; k < sizeof names / sizeof names[0]; k++)
    if (strlen (names[k].name) == end - 1 && memcmp (s + 1, names[k].name, end - 1) == 0)
      {
        *out_len = utf8 (names[k].c, out);
        return end + 1;
      }
  return 0;
}

/* `s` (n bytes) with its entities decoded, into a NUL-terminated buffer (caller frees); NULL without memory. */
static char *
decode (const char *s, size_t n)
{
  Buf b = { 0 };

  buf_add (&b, "", 0);
  for (size_t i = 0; i < n;)
    {
      char u[4];
      size_t ul, took = s[i] == '&' ? entity (s + i, n - i, u, &ul) : 0;

      if (took > 0)
        {
          buf_add (&b, u, ul);
          i += took;
        }
      else
        buf_add (&b, &s[i++], 1);
    }
  if (b.failed)
    {
      free (b.p);
      return NULL;
    }
  return b.p;
}

static bool
nocase_eq (const char *a, size_t n, const char *lower)
{
  size_t i = 0;

  for (; i < n && lower[i] != '\0'; i++)
    {
      char c = a[i] >= 'A' && a[i] <= 'Z' ? (char) (a[i] - 'A' + 'a') : a[i];

      if (c != lower[i])
        return false;
    }
  return i == n && lower[i] == '\0';
}

static bool
contains_nocase (const char *s, size_t n, const char *lower)
{
  size_t m = strlen (lower);

  for (size_t i = 0; i + m <= n; i++)
    if (nocase_eq (s + i, m, lower))
      return true;
  return false;
}

static void
trim (const char **s, size_t *n)
{
  while (*n > 0 && (**s == ' ' || **s == '\t' || **s == '\n' || **s == '\r'))
    {
      (*s)++;
      (*n)--;
    }
  while (*n > 0 && ((*s)[*n - 1] == ' ' || (*s)[*n - 1] == '\t' || (*s)[*n - 1] == '\n' || (*s)[*n - 1] == '\r'))
    (*n)--;
}

/* A CSS or HTML color: #rgb, #rrggbb, rgb(r, g, b), rgba(r, g, b, a); false for anything else. */
static bool
parse_color (const char *s, size_t n, uint32_t *out)
{
  trim (&s, &n);
  if (n > 0 && s[0] == '#' && (n == 4 || n == 7))
    {
      uint32_t c = 0;

      for (size_t i = 1; i < n; i++)
        {
          int d = s[i] >= '0' && s[i] <= '9' ? s[i] - '0'
                  : s[i] >= 'a' && s[i] <= 'f' ? s[i] - 'a' + 10
                  : s[i] >= 'A' && s[i] <= 'F' ? s[i] - 'A' + 10
                                               : -1;

          if (d < 0)
            return false;
          c = n == 4 ? c << 8 | (uint32_t) (d * 17) : c << 4 | (uint32_t) d;
        }
      *out = 0xFF000000u | c;
      return true;
    }
  if (n > 4 && (nocase_eq (s, 4, "rgb(") || (n > 5 && nocase_eq (s, 5, "rgba("))))
    {
      char tmp[64];
      double v[4] = { 0, 0, 0, 1 };
      int got;

      if (n >= sizeof tmp)
        return false;
      memcpy (tmp, s, n);
      tmp[n] = '\0';
      got = sscanf (strchr (tmp, '(') + 1, "%lf , %lf , %lf , %lf", &v[0], &v[1], &v[2], &v[3]);
      if (got < 3)
        return false;
      for (int k = 0; k < 3; k++)
        v[k] = v[k] < 0 ? 0 : v[k] > 255 ? 255 : v[k];
      v[3] = v[3] < 0 ? 0 : v[3] > 1 ? 1 : v[3];
      *out = (uint32_t) lround (v[3] * 255) << 24 | (uint32_t) lround (v[0]) << 16 | (uint32_t) lround (v[1]) << 8
             | (uint32_t) lround (v[2]);
      return true;
    }
  return false;
}

/* A CSS size in points: Npt, or Npx at 96 dpi (0.75 points a pixel), rounded, 1..500; 0 for anything else. */
static int
parse_size (const char *s, size_t n)
{
  char tmp[32];
  double v;
  char *end;

  trim (&s, &n);
  if (n == 0 || n >= sizeof tmp)
    return 0;
  memcpy (tmp, s, n);
  tmp[n] = '\0';
  v = strtod (tmp, &end);
  if (end == tmp)
    return 0;
  if (nocase_eq (end, strlen (end), "px"))
    v *= 0.75;
  else if (!nocase_eq (end, strlen (end), "pt"))
    return 0;
  v = round (v);
  return v < 1 ? 1 : v > 500 ? 500 : (int) v;
}

/* The first family of a CSS or <font face> list, its quotes and spaces removed. */
static void
take_family (Reader *r, const char *s, size_t n)
{
  size_t end = 0;
  char quote = 0;

  for (; end < n; end++)
    {
      if (quote != 0 && s[end] == quote)
        quote = 0;
      else if (quote == 0 && (s[end] == '"' || s[end] == '\''))
        quote = s[end];
      else if (quote == 0 && s[end] == ',')
        break;
    }
  n = end;
  trim (&s, &n);
  if (n >= 2 && (s[0] == '"' || s[0] == '\'') && s[n - 1] == s[0])
    {
      s++;
      n -= 2;
    }
  trim (&s, &n);
  if (n > 0)
    r->style.family = intern (r, s, n);
}

/* The declarations of a style attribute. */
static void
apply_css (Reader *r, const char *css)
{
  const char *p = css;

  while (*p != '\0')
    {
      const char *decl = p, *colon, *name, *value;
      size_t dn, nn, vn;
      char quote = 0;

      while (*p != '\0' && (quote != 0 || *p != ';'))
        {
          if (quote != 0 && *p == quote)
            quote = 0;
          else if (quote == 0 && (*p == '"' || *p == '\''))
            quote = *p;
          p++;
        }
      dn = (size_t) (p - decl);
      if (*p == ';')
        p++;
      colon = memchr (decl, ':', dn);
      if (colon == NULL)
        continue;
      name = decl;
      nn = (size_t) (colon - decl);
      value = colon + 1;
      vn = dn - nn - 1;
      trim (&name, &nn);
      trim (&value, &vn);
      if (nocase_eq (name, nn, "font-family"))
        take_family (r, value, vn);
      else if (nocase_eq (name, nn, "font-size"))
        {
          int pt = parse_size (value, vn);

          if (pt > 0)
            r->style.size_pt = pt;
        }
      else if (nocase_eq (name, nn, "color"))
        r->style.has_color = parse_color (value, vn, &r->style.color) || r->style.has_color;
      else if (nocase_eq (name, nn, "font-weight"))
        {
          char *end;
          char tmp[16] = "";
          long w;

          if (vn < sizeof tmp)
            memcpy (tmp, value, vn);
          w = strtol (tmp, &end, 10);
          if (nocase_eq (value, vn, "bold") || nocase_eq (value, vn, "bolder") || (end != tmp && w >= 600))
            r->style.flags |= EASEL_TEXT_BOLD;
          else if (nocase_eq (value, vn, "normal") || nocase_eq (value, vn, "lighter") || (end != tmp && w < 600))
            r->style.flags &= (uint8_t) ~EASEL_TEXT_BOLD;
        }
      else if (nocase_eq (name, nn, "font-style"))
        {
          if (nocase_eq (value, vn, "italic") || nocase_eq (value, vn, "oblique"))
            r->style.flags |= EASEL_TEXT_ITALIC;
          else if (nocase_eq (value, vn, "normal"))
            r->style.flags &= (uint8_t) ~EASEL_TEXT_ITALIC;
        }
      else if (nocase_eq (name, nn, "text-decoration") || nocase_eq (name, nn, "text-decoration-line"))
        {
          if (contains_nocase (value, vn, "underline"))
            r->style.flags |= EASEL_TEXT_UNDERLINE;
          if (contains_nocase (value, vn, "line-through"))
            r->style.flags |= EASEL_TEXT_STRIKE;
        }
    }
}

/* One attribute of the tag being opened. */
static void
apply_attribute (Reader *r, const char *tag, const char *name, size_t nn, const char *value)
{
  size_t vn = strlen (value);

  if (nocase_eq (name, nn, "style"))
    apply_css (r, value);
  else if (strcmp (tag, "font") == 0 && nocase_eq (name, nn, "face"))
    take_family (r, value, vn);
  else if (strcmp (tag, "font") == 0 && nocase_eq (name, nn, "color"))
    r->style.has_color = parse_color (value, vn, &r->style.color) || r->style.has_color;
  else if (strcmp (tag, "font") == 0 && nocase_eq (name, nn, "size") && vn == 1 && value[0] >= '1' && value[0] <= '7')
    {
      static const int pts[] = { 8, 10, 12, 14, 18, 24, 36 };

      r->style.size_pt = pts[value[0] - '1'];
    }
}

static bool
is_one_of (const char *tag, const char *const *list)
{
  for (; *list != NULL; list++)
    if (strcmp (tag, *list) == 0)
      return true;
  return false;
}

static const char *const voids[] = { "br", "img", "meta", "hr", "input", "link", "wbr", "col", "area", "base", NULL };
static const char *const blocks[] = { "p", "div", "li", "tr", "h1", "h2", "h3", "h4", "h5", "h6", "ul", "ol", "table",
                                      "blockquote", "pre", "body", NULL };
static const char *const skipped[] = { "head", "style", "script", "title", NULL };

/* Opens `tag` (lower case) at the tag's attributes `a` (n bytes, up to its '>'). */
static void
open_tag (Reader *r, const char *tag, const char *a, size_t n, bool self_closing)
{
  bool is_void = is_one_of (tag, voids) || self_closing;

  if (strcmp (tag, "br") == 0)
    {
      line_break (r);
      return;
    }
  if (is_one_of (tag, blocks))
    block_edge (r);
  if (is_void)
    return;
  if (!grow ((void **) &r->stack, &r->stack_cap, r->depth + 1, sizeof *r->stack))
    {
      r->failed = true;
      return;
    }
  snprintf (r->stack[r->depth].name, sizeof r->stack[0].name, "%s", tag);
  r->stack[r->depth++].saved = r->style;
  if (strcmp (tag, "b") == 0 || strcmp (tag, "strong") == 0)
    r->style.flags |= EASEL_TEXT_BOLD;
  else if (strcmp (tag, "i") == 0 || strcmp (tag, "em") == 0)
    r->style.flags |= EASEL_TEXT_ITALIC;
  else if (strcmp (tag, "u") == 0 || strcmp (tag, "ins") == 0)
    r->style.flags |= EASEL_TEXT_UNDERLINE;
  else if (strcmp (tag, "s") == 0 || strcmp (tag, "strike") == 0 || strcmp (tag, "del") == 0)
    r->style.flags |= EASEL_TEXT_STRIKE;
  /* The attributes: name, '=', a quoted or bare value. */
  for (size_t i = 0; i < n && !r->failed;)
    {
      size_t ns, ne, vs, ve;
      char *value;

      while (i < n && (a[i] == ' ' || a[i] == '\t' || a[i] == '\n' || a[i] == '\r' || a[i] == '/'))
        i++;
      ns = i;
      while (i < n && a[i] != '=' && a[i] != ' ' && a[i] != '\t' && a[i] != '\n' && a[i] != '\r' && a[i] != '/')
        i++;
      ne = i;
      while (i < n && (a[i] == ' ' || a[i] == '\t' || a[i] == '\n' || a[i] == '\r'))
        i++;
      if (i >= n || a[i] != '=')
        {
          if (i == ns)
            i++;
          continue;
        }
      i++;
      while (i < n && (a[i] == ' ' || a[i] == '\t' || a[i] == '\n' || a[i] == '\r'))
        i++;
      if (i < n && (a[i] == '"' || a[i] == '\''))
        {
          char q = a[i++];

          vs = i;
          while (i < n && a[i] != q)
            i++;
          ve = i;
          if (i < n)
            i++;
        }
      else
        {
          vs = i;
          while (i < n && a[i] != ' ' && a[i] != '\t' && a[i] != '\n' && a[i] != '\r')
            i++;
          ve = i;
        }
      value = decode (a + vs, ve - vs);
      if (value == NULL)
        {
          r->failed = true;
          return;
        }
      apply_attribute (r, tag, a + ns, ne - ns, value);
      free (value);
    }
}

static void
close_tag (Reader *r, const char *tag)
{
  if (is_one_of (tag, blocks))
    block_edge (r);
  if (strcmp (tag, "td") == 0 || strcmp (tag, "th") == 0)
    r->space_pending = r->space_pending || !r->at_line_start;
  for (size_t d = r->depth; d > 0; d--)
    if (strcmp (r->stack[d - 1].name, tag) == 0)
      {
        r->style = r->stack[d - 1].saved;
        r->depth = d - 1;
        return;
      }
}

/* Where the text of a skipped element ends: after its closing tag, or the end. */
static size_t
skip_element (const char *html, size_t len, size_t i, const char *tag)
{
  size_t tn = strlen (tag);

  for (; i + tn + 2 < len; i++)
    if (html[i] == '<' && html[i + 1] == '/' && nocase_eq (html + i + 2, tn, tag))
      {
        const char *gt = memchr (html + i, '>', len - i);

        return gt != NULL ? (size_t) (gt - html) + 1 : len;
      }
  return len;
}

bool
easel_rich_text_from_html (const char *html, size_t len, EaselRichText *out)
{
  Reader r = { 0 };
  size_t i = 0;

  *out = (EaselRichText) { 0 };
  r.rt = out;
  r.at_line_start = true;
  while (i < len && !r.failed)
    {
      if (html[i] == '<')
        {
          const char *gt;
          size_t j = i + 1, ns, ne;
          bool closing = false;
          char tag[16];

          if (len - i >= 4 && memcmp (html + i, "<!--", 4) == 0)
            {
              const char *end = NULL;

              for (size_t k = i + 4; k + 2 < len; k++)
                if (html[k] == '-' && html[k + 1] == '-' && html[k + 2] == '>')
                  {
                    end = html + k + 3;
                    break;
                  }
              i = end != NULL ? (size_t) (end - html) : len;
              continue;
            }
          gt = memchr (html + i, '>', len - i);
          if (gt == NULL)
            break;
          if (j < len && (html[j] == '!' || html[j] == '?'))
            {
              i = (size_t) (gt - html) + 1;
              continue;
            }
          if (j < len && html[j] == '/')
            {
              closing = true;
              j++;
            }
          ns = j;
          while (j < (size_t) (gt - html) && ((html[j] >= 'a' && html[j] <= 'z') || (html[j] >= 'A' && html[j] <= 'Z')
                                              || (html[j] >= '0' && html[j] <= '9')))
            j++;
          ne = j;
          if (ne == ns || ne - ns >= sizeof tag)
            {
              i = (size_t) (gt - html) + 1;
              continue;
            }
          for (size_t k = 0; k < ne - ns; k++)
            tag[k] = html[ns + k] >= 'A' && html[ns + k] <= 'Z' ? (char) (html[ns + k] - 'A' + 'a') : html[ns + k];
          tag[ne - ns] = '\0';
          i = (size_t) (gt - html) + 1;
          if (closing)
            close_tag (&r, tag);
          else if (is_one_of (tag, skipped))
            i = skip_element (html, len, i, tag);
          else
            open_tag (&r, tag, html + ne, (size_t) (gt - html) - ne, gt > html + ne && gt[-1] == '/');
          continue;
        }
      /* Control characters are not text (LibreOffice ends its HTML with a NUL byte, tests/data/rich/writer.html). */
      if ((unsigned char) html[i] < 0x20 && html[i] != '\t' && html[i] != '\n' && html[i] != '\r' && html[i] != '\f')
        {
          i++;
          continue;
        }
      if (html[i] == ' ' || html[i] == '\t' || html[i] == '\n' || html[i] == '\r' || html[i] == '\f')
        {
          if (!r.at_line_start && !r.space_pending && !r.break_pending)
            {
              r.space_pending = true;
              r.space_style = r.style;
            }
          i++;
          continue;
        }
      {
        char u[4];
        size_t ul, took = html[i] == '&' ? entity (html + i, len - i, u, &ul) : 0;

        if (took > 0)
          {
            emit_text (&r, u, ul);
            i += took;
          }
        else
          {
            size_t j = i + 1;

            while (j < len && html[j] != '<' && html[j] != '&' && (unsigned char) html[j] > 0x20)
              j++;
            emit_text (&r, html + i, j - i);
            i = j;
          }
      }
    }
  free (r.stack);
  if (r.failed || r.text.failed)
    {
      free (r.text.p);
      out->text = NULL;
      easel_rich_text_free (out);
      return false;
    }
  if (r.text.p == NULL)
    {
      buf_add (&r.text, "", 0);
      if (r.text.failed)
        {
          easel_rich_text_free (out);
          return false;
        }
    }
  out->text = r.text.p;
  out->len = r.text.len;
  return true;
}

void
easel_rich_text_free (EaselRichText *rt)
{
  for (size_t i = 0; i < rt->n_families; i++)
    free (rt->families[i]);
  free (rt->families);
  free (rt->runs);
  free (rt->text);
  *rt = (EaselRichText) { 0 };
}
