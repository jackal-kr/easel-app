/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Opening pictures (0005): formats, orientation, size limit, errors. The files are in tests/data (make-fixtures.py). */

#include <glib/gstdio.h>
#include <sys/resource.h>
#include <unistd.h>

#include "picture-file.h"

enum { W = 7, H = 5 };

static void assert_near (uint32_t c, uint32_t want);

static char *
data (const char *name)
{
  const char *dir = g_getenv ("EASEL_TEST_DATA");

  return g_build_filename (dir ? dir : "tests/data", name, NULL);
}

/* The pattern of make-fixtures.py, as the picture's pixels (fully transparent ones as 0). */
static uint32_t
pattern (int x, int y)
{
  uint32_t a = (x + y) % 4 == 0 ? 0 : (x % 3 == 0 ? 128 : 255);
  uint32_t r = (uint32_t) (30 * x + 20), g = (uint32_t) (40 * y + 10), b = (uint32_t) (100 + x + y);

  return a == 0 ? 0 : (a << 24) | (r << 16) | (g << 8) | b;
}

static EaselImage *
open_ok (const char *name)
{
  g_autofree char *path = data (name);
  EaselImage *img = NULL;
  int w = -1, h = -1;

  g_assert_cmpint (easel_picture_open (path, &img, &w, &h), ==, EASEL_OPEN_OK);
  g_assert_nonnull (img);
  g_assert_cmpint (img->width, ==, w);
  g_assert_cmpint (img->height, ==, h);
  return img;
}

static EaselOpenResult
open_fail (const char *name, int *w, int *h)
{
  g_autofree char *path = data (name);
  EaselImage *img = (EaselImage *) 0x1;
  EaselOpenResult r = easel_picture_open (path, &img, w, h);

  g_assert_null (img);
  return r;
}

static void
assert_pattern (const char *name)
{
  EaselImage *img = open_ok (name);

  g_assert_cmpint (img->width, ==, W);
  g_assert_cmpint (img->height, ==, H);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      g_assert_cmphex (easel_image_get (img, x, y), ==, pattern (x, y));
  easel_image_free (img);
}

/* AS-5: every format opens at its size; transparency is kept pixel for pixel; JPEG and BMP are opaque; an animation is
   its first frame; the content decides, not the name. */
static void
as5_open_formats (void)
{
  EaselImage *img;

  assert_pattern ("rgba.png");
  assert_pattern ("rgba.tiff");
  assert_pattern ("rgba.webp");
  assert_pattern ("named.dat");                      /* a PNG with another name opens */

  /* BMP: 24 bit, and 32 bit whose alpha channel is ignored; every pixel opaque. */
  img = open_ok ("opaque.bmp");
  g_assert_cmpint (img->width, ==, W);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      g_assert_cmphex (easel_image_get (img, x, y), ==, 0xFF000000u | (uint32_t) ((30 * x + 20) << 16)
                                                           | (uint32_t) ((40 * y + 10) << 8) | (uint32_t) (100 + x + y));
  easel_image_free (img);
  img = open_ok ("rgba.bmp");
  g_assert_cmpint (img->width, ==, W);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      g_assert_cmpuint (easel_image_get (img, x, y) >> 24, ==, 0xFF);
  easel_image_free (img);

  /* JPEG: opaque, one flat color within the codec's error. */
  img = open_ok ("flat.jpg");
  g_assert_cmpint (img->width, ==, 16);
  g_assert_cmpint (img->height, ==, 16);
  for (int i = 0; i < 16 * 16; i++)
    {
      uint32_t c = img->px[i];

      g_assert_cmphex (c >> 24, ==, 0xFF);
      g_assert_cmpint (abs ((int) ((c >> 16) & 0xFF) - 200), <=, 3);
      g_assert_cmpint (abs ((int) ((c >> 8) & 0xFF) - 100), <=, 3);
      g_assert_cmpint (abs ((int) (c & 0xFF) - 50), <=, 3);
    }
  easel_image_free (img);

  /* Gray, indexed and 16-bit PNG come as RGBA. */
  img = open_ok ("gray.png");
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      {
        uint32_t v = (uint32_t) (10 * x + 20 * y);

        g_assert_cmphex (easel_image_get (img, x, y), ==, 0xFF000000u | (v << 16) | (v << 8) | v);
      }
  easel_image_free (img);
  img = open_ok ("gray16.png");
  g_assert_cmphex (easel_image_get (img, 3, 2), ==, 0xFF000000u | (70u << 16) | (70u << 8) | 70u);
  easel_image_free (img);
  img = open_ok ("indexed.png");
  g_assert_cmphex (easel_image_get (img, 0, 0), ==, 0xFFFF0000u);
  g_assert_cmphex (easel_image_get (img, 1, 0), ==, 0xFF00FF00u);
  g_assert_cmphex (easel_image_get (img, 2, 0), ==, 0xFF0000FFu);
  g_assert_cmphex (easel_image_get (img, 3, 0), ==, 0xFF0A141Eu);
  easel_image_free (img);

  /* GIF: the transparent color is transparent pixels (#00000000); an animation is its first frame. */
  img = open_ok ("transparent.gif");
  g_assert_cmpint (img->width, ==, W);
  g_assert_cmphex (easel_image_get (img, 0, 0), ==, 0x00000000u);   /* (0 + 0) % 4 == 0: the transparent index */
  g_assert_cmphex (easel_image_get (img, 1, 0), ==, 0xFF00FF00u);   /* index 1 */
  easel_image_free (img);
  img = open_ok ("anim.gif");
  g_assert_cmpint (img->width, ==, 6);
  g_assert_cmpint (img->height, ==, 4);
  g_assert_cmphex (easel_image_get (img, 2, 2), ==, 0xFFFF0000u);
  easel_image_free (img);
}

/* AS-8: every reason has its result and leaves nothing behind. */
static void
as8_open_errors (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *locked = g_build_filename (dir, "locked.png", NULL);
  g_autofree char *missing = g_build_filename (dir, "missing.png", NULL);
  g_autofree char *source = data ("rgba.png");
  g_autofree char *contents = NULL;
  gsize len;
  EaselImage *img = (EaselImage *) 0x1;
  int w = 99, h = 99;

  g_assert_cmpint (easel_picture_open (missing, &img, &w, &h), ==, EASEL_OPEN_UNREADABLE);
  g_assert_null (img);
  g_assert_cmpint (w, ==, 0);
  g_assert_cmpint (h, ==, 0);
  img = (EaselImage *) 0x1;
  g_assert_cmpint (easel_picture_open (dir, &img, &w, &h), ==, EASEL_OPEN_UNREADABLE);     /* a folder */
  g_assert_null (img);

  /* No permission (a root user can read anything, so the check is skipped for them). */
  g_assert_true (g_file_get_contents (source, &contents, &len, NULL));
  g_assert_true (g_file_set_contents (locked, contents, (gssize) len, NULL));
  g_assert_cmpint (g_chmod (locked, 0), ==, 0);
  if (geteuid () != 0)
    {
      img = (EaselImage *) 0x1;
      g_assert_cmpint (easel_picture_open (locked, &img, &w, &h), ==, EASEL_OPEN_UNREADABLE);
      g_assert_null (img);
    }

  g_assert_cmpint (open_fail ("empty.png", &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
  g_assert_cmpint (open_fail ("text.png", &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
  g_assert_cmpint (open_fail ("w10001.png", &w, &h), ==, EASEL_OPEN_TOO_BIG);
  g_assert_cmpint (w, ==, 10001);
  g_assert_cmpint (h, ==, 1);

  /* A picture that decodes but cannot get its memory: the address space is limited to a little more than the process
     uses now, and it is 6000 x 6000 pixels (144 MB for the picture alone). */
  {
    struct rlimit old, tight;
    FILE *f = fopen ("/proc/self/statm", "r");
    long pages = 0;
    EaselOpenResult r;

    g_assert_nonnull (f);
    g_assert_cmpint (fscanf (f, "%ld", &pages), ==, 1);
    fclose (f);
    g_assert_cmpint (getrlimit (RLIMIT_AS, &old), ==, 0);
    tight = old;
    tight.rlim_cur = (rlim_t) pages * (rlim_t) sysconf (_SC_PAGESIZE) + (rlim_t) 100 * 1024 * 1024;
    if (old.rlim_max != RLIM_INFINITY && tight.rlim_cur > old.rlim_max)
      tight.rlim_cur = old.rlim_max;
    g_assert_cmpint (setrlimit (RLIMIT_AS, &tight), ==, 0);
    r = open_fail ("flat6000.png", &w, &h);
    g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
    g_assert_cmpint (r, ==, EASEL_OPEN_NO_MEMORY);
    g_assert_cmpint (w, ==, 6000);
    g_assert_cmpint (h, ==, 6000);
    /* And it opens once the memory is there. */
    img = open_ok ("flat6000.png");
    g_assert_cmpint (img->width, ==, 6000);
    easel_image_free (img);
  }
  (void) g_chmod (locked, 0600);
}

/* AS-17: 10000 opens, 10001 is refused from the header, also after the orientation turns it, and without decoding. */
static void
as17_too_big_refused (void)
{
  EaselImage *img;
  int w = 0, h = 0;
  gint64 start;

  img = open_ok ("w10000.png");
  g_assert_cmpint (img->width, ==, 10000);
  g_assert_cmpint (img->height, ==, 1);
  easel_image_free (img);
  img = open_ok ("h10000.png");
  g_assert_cmpint (img->width, ==, 1);
  g_assert_cmpint (img->height, ==, 10000);
  easel_image_free (img);

  g_assert_cmpint (open_fail ("w10001.png", &w, &h), ==, EASEL_OPEN_TOO_BIG);
  g_assert_cmpint (open_fail ("h10001.png", &w, &h), ==, EASEL_OPEN_TOO_BIG);
  g_assert_cmpint (w, ==, 1);
  g_assert_cmpint (h, ==, 10001);

  /* 10001 x 10001 is 100 million pixels: refused from the header, so it takes no time and no memory to say so. */
  start = g_get_monotonic_time ();
  g_assert_cmpint (open_fail ("big10001.png", &w, &h), ==, EASEL_OPEN_TOO_BIG);
  g_assert_cmpint (w, ==, 10001);
  g_assert_cmpint (h, ==, 10001);
  g_assert_cmpint (g_get_monotonic_time () - start, <, G_USEC_PER_SEC / 2);

  /* Stored 10001 x 3 and tagged "rotate 90": upright it is 3 x 10001, which is refused too (the size reported is the
   stored one). */
  g_assert_cmpint (open_fail ("wide-rot6.jpg", &w, &h), ==, EASEL_OPEN_TOO_BIG);
  g_assert_cmpint (w, ==, 10001);
  g_assert_cmpint (h, ==, 3);
}

/* The color a block of orient-N.jpg should have, within the codec's error. */
static void
assert_near (uint32_t c, uint32_t want)
{
  g_assert_cmphex (c >> 24, ==, 0xFF);
  for (int shift = 0; shift <= 16; shift += 8)
    g_assert_cmpint (abs ((int) ((c >> shift) & 0xFF) - (int) ((want >> shift) & 0xFF)), <=, 8);
}

/* AS-18: eight orientation tags on a 32 x 16 picture whose blocks are red (top left), green (top right), blue (bottom
   left) and yellow (bottom right) as stored. Where each color must be after the tag is applied: the center of each
   16 x 8 block of the stored picture goes to the tag's place. */
static void
as18_orientation (void)
{
  /* For each tag: the upright size, and the colors at the upright picture's top-left, top-right, bottom-left and
     bottom-right quarters. */
  static const struct { int w, h; uint32_t q[4]; } tags[8] = {
    { 32, 16, { 0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu, 0xFFFFFF00u } },   /* 1 as stored: R G / B Y */
    { 32, 16, { 0xFF00FF00u, 0xFFFF0000u, 0xFFFFFF00u, 0xFF0000FFu } },   /* 2 mirrored left to right: G R / Y B */
    { 32, 16, { 0xFFFFFF00u, 0xFF0000FFu, 0xFF00FF00u, 0xFFFF0000u } },   /* 3 turned 180: Y B / G R */
    { 32, 16, { 0xFF0000FFu, 0xFFFFFF00u, 0xFFFF0000u, 0xFF00FF00u } },   /* 4 mirrored top to bottom: B Y / R G */
    { 16, 32, { 0xFFFF0000u, 0xFF0000FFu, 0xFF00FF00u, 0xFFFFFF00u } },   /* 5 transposed: R B / G Y */
    { 16, 32, { 0xFF0000FFu, 0xFFFF0000u, 0xFFFFFF00u, 0xFF00FF00u } },   /* 6 clockwise: B R / Y G */
    { 16, 32, { 0xFFFFFF00u, 0xFF00FF00u, 0xFF0000FFu, 0xFFFF0000u } },   /* 7 transversed: Y G / B R */
    { 16, 32, { 0xFF00FF00u, 0xFFFFFF00u, 0xFFFF0000u, 0xFF0000FFu } },   /* 8 counter-clockwise: G Y / R B */
  };

  for (int tag = 1; tag <= 8; tag++)
    {
      g_autofree char *name = g_strdup_printf ("orient-%d.jpg", tag);
      EaselImage *img = open_ok (name);
      int w = tags[tag - 1].w, h = tags[tag - 1].h;

      g_assert_cmpint (img->width, ==, w);
      g_assert_cmpint (img->height, ==, h);
      assert_near (easel_image_get (img, w / 4, h / 4), tags[tag - 1].q[0]);
      assert_near (easel_image_get (img, 3 * w / 4, h / 4), tags[tag - 1].q[1]);
      assert_near (easel_image_get (img, w / 4, 3 * h / 4), tags[tag - 1].q[2]);
      assert_near (easel_image_get (img, 3 * w / 4, 3 * h / 4), tags[tag - 1].q[3]);
      easel_image_free (img);
    }
}

/* A format that is not installed is "not a picture", as a text file is (REQ-0005-06). */
static void
missing_loader_is_not_a_picture (void)
{
  int w, h;
  EaselImage *img;

  easel_picture_file_hide_format ("webp");
  g_assert_cmpint (open_fail ("rgba.webp", &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
  img = open_ok ("rgba.png");                        /* the others still open */
  easel_image_free (img);
  easel_picture_file_hide_format (NULL);
  img = open_ok ("rgba.webp");
  easel_image_free (img);
}

/* A format gdk-pixbuf can read but the app does not open (SVG) is not a picture. */
static void
unknown_format_is_not_a_picture (void)
{
  int w, h;

  g_assert_cmpint (open_fail ("vector.svg", &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
}

/* The sizes the sniffer must find in each fixture: the size the decoder gives, as stored (before the orientation). */
static void
sniff_sizes_match_decoder (void)
{
  static const struct { const char *name, *format; int w, h; } files[] = {
    { "rgba.png", "png", 7, 5 }, { "named.dat", "png", 7, 5 }, { "gray.png", "png", 7, 5 },
    { "indexed.png", "png", 7, 5 }, { "gray16.png", "png", 7, 5 }, { "w10001.png", "png", 10001, 1 },
    { "big10001.png", "png", 10001, 10001 }, { "flat6000.png", "png", 6000, 6000 },
    { "rgba.tiff", "tiff", 7, 5 }, { "rgba.webp", "webp", 7, 5 }, { "opaque.bmp", "bmp", 7, 5 },
    { "rgba.bmp", "bmp", 7, 5 }, { "flat.jpg", "jpeg", 16, 16 }, { "orient-6.jpg", "jpeg", 32, 16 },
    { "wide-rot6.jpg", "jpeg", 10001, 3 }, { "transparent.gif", "gif", 7, 5 }, { "anim.gif", "gif", 6, 4 },
  };

  for (gsize i = 0; i < G_N_ELEMENTS (files); i++)
    {
      g_autofree char *path = data (files[i].name);
      const char *format = NULL;
      int w = 0, h = 0;

      g_assert_true (easel_picture_sniff (path, &format, &w, &h));
      g_assert_cmpstr (format, ==, files[i].format);
      g_assert_cmpint (w, ==, files[i].w);
      g_assert_cmpint (h, ==, files[i].h);
    }
  /* And what the decoder says is the same for the ones that are small enough to open. */
  for (gsize i = 0; i < G_N_ELEMENTS (files); i++)
    {
      g_autofree char *path = data (files[i].name);
      EaselImage *img = NULL;
      int w, h;

      if (files[i].w > 1000 || files[i].h > 1000)
        continue;
      g_assert_cmpint (easel_picture_open (path, &img, &w, &h), ==, EASEL_OPEN_OK);
      g_assert_true ((img->width == files[i].w && img->height == files[i].h)
                     || (img->width == files[i].h && img->height == files[i].w));   /* the orientation may turn it */
      easel_image_free (img);
    }
}

/* Honest-file protection (Plan): files that are cut off, empty, of another kind, or that state a size they do not
   have are refused without decoding them, in a moment. */
static void
sniff_refuses_broken_files (void)
{
  static const char *none[] = { "empty.png", "text.png", "cut-header.png", "no-size.jpg", "sos-first.jpg", "zero-width.png",
                                "vector.svg" };
  const char *format = NULL;
  int w = 0, h = 0;
  gint64 start = g_get_monotonic_time ();
  EaselImage *img = NULL;

  for (gsize i = 0; i < G_N_ELEMENTS (none); i++)
    {
      g_autofree char *path = data (none[i]);

      g_assert_false (easel_picture_sniff (path, &format, &w, &h));
      g_assert_cmpint (easel_picture_open (path, &img, &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
      g_assert_null (img);
    }
  /* A folder and a missing file are not sniffed. */
  g_assert_false (easel_picture_sniff ("/", &format, &w, &h));
  g_assert_false (easel_picture_sniff ("/nonexistent/x.png", &format, &w, &h));

  /* The size is there but the data is cut off: the sniffer passes it and the decoder says it is not a picture. */
  {
    g_autofree char *path = data ("cut-data.jpg");

    g_assert_true (easel_picture_sniff (path, &format, &w, &h));
    g_assert_cmpint (w, ==, 32);
    g_assert_cmpint (easel_picture_open (path, &img, &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
    g_assert_null (img);
  }
  /* A header of a tiny file that claims a legal 10000 x 10000: the sniffer cannot know; the decoder refuses it. */
  {
    g_autofree char *path = data ("claims-10000.png");

    g_assert_true (easel_picture_sniff (path, &format, &w, &h));
    g_assert_cmpint (w, ==, 10000);
    g_assert_cmpint (h, ==, 10000);
    g_assert_cmpint (easel_picture_open (path, &img, &w, &h), ==, EASEL_OPEN_NOT_PICTURE);
    g_assert_null (img);
  }
  /* A header that claims 40000 x 40000 is refused as too big, from its first bytes. */
  {
    g_autofree char *path = data ("claims-40000.png");

    g_assert_cmpint (easel_picture_open (path, &img, &w, &h), ==, EASEL_OPEN_TOO_BIG);
    g_assert_cmpint (w, ==, 40000);
    g_assert_null (img);
  }
  g_assert_cmpint (g_get_monotonic_time () - start, <, 5 * G_USEC_PER_SEC);
}

/* ---- saving (T005) ---- */

static char *
temp_path (const char *name)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-pf-XXXXXX", NULL);

  return g_build_filename (dir, name, NULL);
}

/* The pattern of the fixtures as a picture, with its transparent and half transparent pixels. */
static EaselImage *
pattern_image (void)
{
  EaselImage *img = easel_image_new (W, H, 0);

  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      easel_image_set (img, x, y, pattern (x, y));
  return img;
}

static EaselImage *
save_and_open (const EaselImage *img, const char *name, EaselFormat format, int quality)
{
  g_autofree char *path = temp_path (name);
  g_autoptr (GError) error = NULL;
  EaselImage *back = NULL;
  int w, h;

  g_assert_true (easel_picture_save (img, path, format, quality, &error));
  g_assert_no_error (error);
  g_assert_cmpint (easel_picture_open (path, &back, &w, &h), ==, EASEL_OPEN_OK);
  return back;
}

/* AS-10: PNG, TIFF and WebP give back every pixel; BMP gives back the picture flattened on white by the one formula;
   JPEG is opaque and, in flat areas, within 8 of that. The picture itself is not touched. */
static void
as10_round_trips (void)
{
  EaselImage *img = pattern_image (), *back;
  static const struct { const char *name; EaselFormat format; } exact[] = {
    { "a.png", EASEL_FORMAT_PNG }, { "a.tiff", EASEL_FORMAT_TIFF }, { "a.webp", EASEL_FORMAT_WEBP },
  };

  for (gsize i = 0; i < G_N_ELEMENTS (exact); i++)
    {
      back = save_and_open (img, exact[i].name, exact[i].format, 90);
      g_assert_cmpint (back->width, ==, W);
      g_assert_cmpint (back->height, ==, H);
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
          g_assert_cmphex (easel_image_get (back, x, y), ==, pattern (x, y));
      easel_image_free (back);
    }

  /* BMP: flattened, so it equals the formula's result at every pixel, and is opaque. */
  back = save_and_open (img, "a.bmp", EASEL_FORMAT_BMP, 90);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      {
        uint32_t p = pattern (x, y);
        uint8_t rgb[3];

        easel_flatten_on_white (&p, rgb, 1);
        g_assert_cmphex (easel_image_get (back, x, y), ==,
                         0xFF000000u | (uint32_t) rgb[0] << 16 | (uint32_t) rgb[1] << 8 | rgb[2]);
      }
  easel_image_free (back);
  /* The picture still has its transparency. */
  g_assert_cmphex (easel_image_get (img, 0, 0), ==, 0);
  easel_image_free (img);

  /* WebP is lossless whatever the encoder's defaults: noise comes back bit for bit. */
  img = easel_image_new (32, 32, 0);
  for (int y = 0; y < 32; y++)
    for (int x = 0; x < 32; x++)
      easel_image_set (img, x, y, 0xFF000000u | (uint32_t) ((x * 7919 + y * 104729) % 16777216u));
  back = save_and_open (img, "noise.webp", EASEL_FORMAT_WEBP, 10);
  for (int y = 0; y < 32; y++)
    for (int x = 0; x < 32; x++)
      g_assert_cmphex (easel_image_get (back, x, y), ==, easel_image_get (img, x, y));
  easel_image_free (back);
  easel_image_free (img);

  /* JPEG: the left half is transparent (white when flattened), the right half one flat color. */
  img = easel_image_new (32, 32, 0);
  for (int y = 0; y < 32; y++)
    for (int x = 16; x < 32; x++)
      easel_image_set (img, x, y, 0xFFC86432u);
  back = save_and_open (img, "a.jpg", EASEL_FORMAT_JPEG, 90);
  assert_near (easel_image_get (back, 4, 16), 0xFFFFFFFFu);
  assert_near (easel_image_get (back, 27, 16), 0xFFC86432u);
  easel_image_free (back);
  easel_image_free (img);
}

/* A failed save leaves the old file as it was, whatever the format: it starts from an existing file, since only then do
   an atomic write and an in-place one differ. */
static void
as10_failed_save_keeps_old_file (void)
{
  static const struct { const char *name; EaselFormat format; } formats[] = {
    { "a.png", EASEL_FORMAT_PNG }, { "a.jpg", EASEL_FORMAT_JPEG }, { "a.tif", EASEL_FORMAT_TIFF },
    { "a.bmp", EASEL_FORMAT_BMP }, { "a.webp", EASEL_FORMAT_WEBP },
  };
  EaselImage *img = pattern_image ();

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      easel_image_free (img);
      return;
    }
  for (gsize i = 0; i < G_N_ELEMENTS (formats); i++)
    {
      g_autofree char *path = temp_path (formats[i].name);
      g_autofree char *dir = g_path_get_dirname (path);
      g_autofree char *after = NULL;
      g_autoptr (GError) error = NULL;

      g_assert_true (g_file_set_contents (path, "old content", -1, NULL));
      g_assert_cmpint (g_chmod (dir, 0500), ==, 0);
      g_assert_false (easel_picture_save (img, path, formats[i].format, 90, &error));
      g_assert_nonnull (error);
      g_assert_true (g_file_get_contents (path, &after, NULL, NULL));
      g_assert_cmpstr (after, ==, "old content");
      (void) g_chmod (dir, 0700);
    }
  easel_image_free (img);
}

/* AS-16: the quality changes the file; the default of the settings is a middle quality. */
static void
as16_quality_changes_size (void)
{
  EaselImage *img = easel_image_new (64, 64, 0xFF000000u);
  g_autofree char *low = temp_path ("low.jpg");
  g_autofree char *high = temp_path ("high.jpg");
  g_autoptr (GError) error = NULL;
  gsize low_size = 0, high_size = 0;
  g_autofree char *contents = NULL;

  for (int y = 0; y < 64; y++)
    for (int x = 0; x < 64; x++)
      easel_image_set (img, x, y, 0xFF000000u | (uint32_t) ((x * 37 + y * 11) % 256) << 16
                                   | (uint32_t) ((x * 5 + y * 53) % 256) << 8 | (uint32_t) ((x * y) % 256));
  g_assert_true (easel_picture_save (img, low, EASEL_FORMAT_JPEG, 30, &error));
  g_assert_true (easel_picture_save (img, high, EASEL_FORMAT_JPEG, 100, &error));
  g_assert_true (g_file_get_contents (low, &contents, &low_size, NULL));
  g_free (contents);
  contents = NULL;
  g_assert_true (g_file_get_contents (high, &contents, &high_size, NULL));
  g_assert_cmpuint (low_size, <, high_size);
  easel_image_free (img);
}

/* A format that cannot be saved (not installed): the error is gdk-pixbuf's own, nothing is written, and an existing
   file keeps its bytes (REQ-0001-31). */
static void
missing_saver_reports_reason (void)
{
  EaselImage *img = pattern_image ();
  g_autofree char *path = temp_path ("a.webp");
  g_autofree char *after = NULL;
  g_autoptr (GError) error = NULL;

  g_assert_true (g_file_set_contents (path, "old webp", -1, NULL));
  easel_picture_file_hide_format ("webp");
  g_assert_false (easel_picture_save (img, path, EASEL_FORMAT_WEBP, 90, &error));
  easel_picture_file_hide_format (NULL);
  g_assert_nonnull (error);
  g_assert_true (error->message != NULL && error->message[0] != '\0');
  g_assert_true (g_file_get_contents (path, &after, NULL, NULL));
  g_assert_cmpstr (after, ==, "old webp");

  g_clear_error (&error);
  g_assert_true (easel_picture_save (img, path, EASEL_FORMAT_WEBP, 90, &error));
  easel_image_free (img);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/0005/as5_open_formats", as5_open_formats);
  g_test_add_func ("/0005/as8_open_errors", as8_open_errors);
  g_test_add_func ("/0005/as17_too_big_refused", as17_too_big_refused);
  g_test_add_func ("/0005/as18_orientation", as18_orientation);
  g_test_add_func ("/0005/sniff_sizes_match_decoder", sniff_sizes_match_decoder);
  g_test_add_func ("/0005/sniff_refuses_broken_files", sniff_refuses_broken_files);
  g_test_add_func ("/0005/as10_round_trips", as10_round_trips);
  g_test_add_func ("/0005/as10_failed_save_keeps_old_file", as10_failed_save_keeps_old_file);
  g_test_add_func ("/0005/as16_quality_changes_size", as16_quality_changes_size);
  g_test_add_func ("/0005/missing_saver_reports_reason", missing_saver_reports_reason);
  g_test_add_func ("/0005/missing_loader_is_not_a_picture", missing_loader_is_not_a_picture);
  g_test_add_func ("/0005/unknown_format_is_not_a_picture", unknown_format_is_not_a_picture);
  return g_test_run ();
}
