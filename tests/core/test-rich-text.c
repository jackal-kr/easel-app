/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Rich text on the clipboard, scenarios of feature 0017 (REQ-0017-19, REQ-0017-20): the HTML Easel writes, and the HTML other apps
   offer, read back. The samples under tests/data/rich were saved by probe-clipboard-rich from LibreOffice Writer and a Chromium-based
   browser (T001). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "rich-text.h"

#define BLACK 0xFF000000u
#define RED 0xFFFF0000u

static char *
read_sample (const char *name, size_t *len)
{
  const char *dir = getenv ("EASEL_TEST_DATA");
  char path[1024];
  FILE *f;
  char *buf;
  long n;

  CHECK (dir != NULL);
  snprintf (path, sizeof path, "%s/rich/%s", dir, name);
  f = fopen (path, "rb");
  CHECK (f != NULL);
  CHECK (fseek (f, 0, SEEK_END) == 0);
  n = ftell (f);
  CHECK (n >= 0 && fseek (f, 0, SEEK_SET) == 0);
  buf = malloc ((size_t) n + 1);
  CHECK (buf != NULL && fread (buf, 1, (size_t) n, f) == (size_t) n);
  buf[n] = '\0';
  fclose (f);
  *len = (size_t) n;
  return buf;
}

/* The run holding byte `pos`. */
static const EaselRichRun *
run_at (const EaselRichText *rt, size_t pos)
{
  size_t i = 0;

  while (i + 1 < rt->n && rt->runs[i + 1].start <= pos)
    i++;
  return &rt->runs[i];
}

static const EaselRichRun *
run_of (const EaselRichText *rt, const char *word)
{
  const char *p = strstr (rt->text, word);

  CHECK (p != NULL);
  return run_at (rt, (size_t) (p - rt->text));
}

/* AS-19 (REQ-0017-19), the core part: "plain bold" with "bold" bold and red in DejaVu Serif 18, then a line break and "a<b&c": the
   HTML has one span per run with the inline styles, the special characters escaped and the line break as <br>; read back, the same text
   and the same runs. */
static void
as19_html_out (void)
{
  const char text[] = "plain bold\na<b&c";
  EaselTextRun runs[3] = {
    { 0, { "DejaVu Sans", 12, 0, BLACK } },
    { 6, { "DejaVu Serif", 18, EASEL_TEXT_BOLD | EASEL_TEXT_UNDERLINE | EASEL_TEXT_STRIKE, RED } },
    { 10, { "DejaVu Sans", 12, EASEL_TEXT_ITALIC, BLACK } },
  };
  size_t n;
  char *html = easel_rich_text_to_html (text, strlen (text), runs, 3, &n);
  EaselRichText rt;
  const EaselRichRun *r;

  CHECK (html != NULL && n == strlen (html));
  CHECK (strstr (html, "font-family:'DejaVu Serif'") != NULL);
  CHECK (strstr (html, "font-size:18pt") != NULL);
  CHECK (strstr (html, "color:#ff0000") != NULL);
  CHECK (strstr (html, "font-weight:bold") != NULL);
  CHECK (strstr (html, "text-decoration:underline line-through") != NULL);
  CHECK (strstr (html, "font-style:italic") != NULL);
  CHECK (strstr (html, "<br>") != NULL);
  CHECK (strstr (html, "a&lt;b&amp;c") != NULL);
  CHECK (easel_rich_text_from_html (html, n, &rt));
  CHECK (strcmp (rt.text, text) == 0);
  r = run_of (&rt, "plain");
  CHECK (strcmp (r->family, "DejaVu Sans") == 0 && r->size_pt == 12 && r->has_color && r->color == BLACK && r->flags == 0);
  r = run_of (&rt, "bold");
  CHECK (strcmp (r->family, "DejaVu Serif") == 0 && r->size_pt == 18 && r->color == RED);
  CHECK (r->flags == (EASEL_TEXT_BOLD | EASEL_TEXT_UNDERLINE | EASEL_TEXT_STRIKE));
  r = run_of (&rt, "a<b");
  CHECK (r->flags == EASEL_TEXT_ITALIC && r->size_pt == 12);
  easel_rich_text_free (&rt);
  free (html);
}

/* AS-20 (REQ-0017-20), the core part: what LibreOffice Writer offers (a whole document: head and style skipped, nested font tags, b, i,
   u, strike, line breaks inside the text read as spaces, a <br/>), and what a Chromium-based browser offers (one span, rgb() colors, a
   size in px, a background color dropped), read as the box would hold them. */
static void
as20_html_in (void)
{
  size_t len;
  char *html = read_sample ("writer.html", &len);
  EaselRichText rt;
  const EaselRichRun *r;

  CHECK (easel_rich_text_from_html (html, len, &rt));
  CHECK (strcmp (rt.text, "plain bold red serif italic blue mono underlined struck green\nsecond line") == 0);
  r = run_of (&rt, "plain");
  CHECK (strcmp (r->family, "DejaVu Sans") == 0 && r->size_pt == 12 && r->color == BLACK && r->flags == 0);
  r = run_of (&rt, "red serif");
  CHECK (strcmp (r->family, "DejaVu Serif") == 0 && r->size_pt == 18 && r->color == RED && r->flags == EASEL_TEXT_BOLD);
  r = run_of (&rt, "blue mono");
  CHECK (strcmp (r->family, "DejaVu Sans Mono") == 0 && r->size_pt == 10 && r->color == 0xFF0000FFu && r->flags == EASEL_TEXT_ITALIC);
  r = run_of (&rt, "struck");
  CHECK (r->size_pt == 14 && r->color == 0xFF008000u && r->flags == (EASEL_TEXT_UNDERLINE | EASEL_TEXT_STRIKE));
  r = run_of (&rt, "second");
  CHECK (r->size_pt == 12 && r->flags == 0);
  easel_rich_text_free (&rt);
  free (html);
  html = read_sample ("chromium.html", &len);
  CHECK (easel_rich_text_from_html (html, len, &rt));
  CHECK (strcmp (rt.text, "build/tests/probe-clipboard-rich read writer") == 0);
  CHECK (rt.n == 1);
  CHECK (strcmp (rt.runs[0].family, "monospace") == 0);
  CHECK (rt.runs[0].size_pt == 9);                                      /* 11.7 px = 8.775 points, rounded */
  CHECK (rt.runs[0].has_color && rt.runs[0].color == 0xFF8C8C8Cu);       /* the text's color; its background dropped */
  CHECK (rt.runs[0].flags == 0);
  easel_rich_text_free (&rt);
  free (html);
}

/* REQ-0017-20: what a box cannot hold is dropped and its text kept (a link, a picture, a table's cells); entities; a part with no
   family, size or color leaves them unset; HTML whitespace collapses (tabs and line breaks in the source are spaces), paragraphs and
   divisions are lines. */
static void
html_subset (void)
{
  static const char html[] =
    "<p>see <a href=\"https://x\">the <b>site</b></a>&nbsp;now &amp; &#233;&#xE9; &lt;3</p>"
    "<p>  two\tspaces\n here </p><img src=\"a.png\"><table><tr><td>c1</td><td>c2</td></tr></table>"
    "<div><span style=\"color: rgba(255, 0, 0, 0.5); font-weight: 700; font-style: oblique\">x</span></div>"
    "<span style=\"font-family: &quot;Noto Sans&quot;, sans-serif; text-decoration-line: underline\">y</span>"
    "<!-- a <b>comment</b> --><font size=\"7\">z</font>";
  EaselRichText rt;
  const EaselRichRun *r;

  CHECK (easel_rich_text_from_html (html, strlen (html), &rt));
  CHECK (strncmp (rt.text, "see the site\xC2\xA0now & \xC3\xA9\xC3\xA9 <3\ntwo spaces here\n", 41) == 0);
  r = run_of (&rt, "see");
  CHECK (r->family == NULL && r->size_pt == 0 && !r->has_color && r->flags == 0);
  r = run_of (&rt, "site");
  CHECK (r->flags == EASEL_TEXT_BOLD);
  CHECK (strstr (rt.text, "c1") != NULL && strstr (rt.text, "c2") != NULL);
  r = run_of (&rt, "x");
  CHECK (r->has_color && r->color == 0x80FF0000u && r->flags == (EASEL_TEXT_BOLD | EASEL_TEXT_ITALIC));
  r = run_of (&rt, "y");
  CHECK (strcmp (r->family, "Noto Sans") == 0 && r->flags == EASEL_TEXT_UNDERLINE);
  r = run_of (&rt, "z");
  CHECK (r->size_pt == 36);                                              /* <font size="7"> */
  CHECK (strstr (rt.text, "comment") == NULL);
  easel_rich_text_free (&rt);
  /* several spaces are one, and a paragraph keeps no space at its end */
  CHECK (easel_rich_text_from_html ("<p>a   b  </p><p>c</p>", 21, &rt));
  CHECK (strcmp (rt.text, "a b\nc") == 0);
  easel_rich_text_free (&rt);
}

/* More than a pipe holds (64 KiB, engineering.md): a long text written and read back whole. */
static void
html_large_round_trip (void)
{
  size_t len = 100000, n;
  char *text = malloc (len + 1);
  EaselTextRun runs[2] = { { 0, { "DejaVu Sans", 12, 0, BLACK } }, { 50000, { "DejaVu Sans", 12, EASEL_TEXT_BOLD, RED } } };
  char *html;
  EaselRichText rt;

  CHECK (text != NULL);
  for (size_t i = 0; i < len; i++)
    text[i] = (char) ('a' + i % 26);
  text[len] = '\0';
  html = easel_rich_text_to_html (text, len, runs, 2, &n);
  CHECK (html != NULL && n > 65536);
  CHECK (easel_rich_text_from_html (html, n, &rt));
  CHECK (rt.len == len && memcmp (rt.text, text, len) == 0);
  CHECK (rt.n == 2 && rt.runs[1].start == 50000 && rt.runs[1].flags == EASEL_TEXT_BOLD);
  easel_rich_text_free (&rt);
  free (html);
  free (text);
}

/* The size the reader gives "A" in `html` (0: not given). */
static int
size_of_a (const char *html)
{
  EaselRichText rt;
  int size;

  CHECK (easel_rich_text_from_html (html, strlen (html), &rt));
  CHECK (rt.len == 1 && rt.text[0] == 'A' && rt.n == 1);
  size = rt.runs[0].size_pt;
  easel_rich_text_free (&rt);
  return size;
}

/* AS-20 (REQ-0017-20), the sizes of pasted rich text: taken as 1 to 500 points (600pt is 500, 0.4pt is 1); <font size> 7 is 36 and
   1 is 8; 24px is 18; 10.6pt is 11; 2em is not given. */
static void
as20_size_in_points (void)
{
  CHECK (size_of_a ("<span style=\"font-size:600pt\">A</span>") == 500);
  CHECK (size_of_a ("<span style=\"font-size:0.4pt\">A</span>") == 1);
  CHECK (size_of_a ("<font size=\"7\">A</font>") == 36);
  CHECK (size_of_a ("<font size=\"1\">A</font>") == 8);
  CHECK (size_of_a ("<span style=\"font-size:24px\">A</span>") == 18);
  CHECK (size_of_a ("<span style=\"font-size:10.6pt\">A</span>") == 11);
  CHECK (size_of_a ("<span style=\"font-size:2em\">A</span>") == 0);
}

int
main (void)
{
  RUN (as19_html_out);
  RUN (as20_html_in);
  RUN (html_subset);
  RUN (html_large_round_trip);
  RUN (as20_size_in_points);
  return 0;
}
