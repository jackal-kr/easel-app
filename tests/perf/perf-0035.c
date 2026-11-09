/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0035 with the real model (T006; AS-2, AS-7, REQ-0035-09). Run in the real session, the model named by
   EASEL_TEST_FINDER (or the one this build installs):

     perf-0035            AS-2 on each picture of docs/reference/0035/test-set (the 95 % rule 4 pixels away from the true mask's
                          edge, removed pixels alpha 0 with their color kept, Undo exact), the same pixels in a second process, then
                          the budgets, each in a new process (the first run includes the model's loading)
     perf-0035 set        AS-2 only (PERF_0035_KEEP=dir keeps each result as dir/NN-result.png)
     perf-0035 hashes     (the second process) one line per picture: the result's FNV-1a hash
     perf-0035 time W H   (one budget process) a W x H photo-like picture: the first run and two later runs, to the painted frame,
                          and the process's peak memory growth over each

   It prints numbers and does not fail on a budget; it exits 1 when AS-2 fails, 77 when there is no model. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "perf-common.h"

#define SET 6

static const char *config_dir;

static double
now_ms (void)
{
  return g_get_monotonic_time () / 1000.0;
}

/* The process's peak resident memory (VmHWM) in bytes, and its reset to what is resident now. */
static gsize
peak_bytes (void)
{
  g_autofree char *text = NULL;
  const char *p;

  if (!g_file_get_contents ("/proc/self/status", &text, NULL, NULL) || (p = strstr (text, "VmHWM:")) == NULL)
    return 0;
  return (gsize) g_ascii_strtoull (p + 6, NULL, 10) * 1024;
}

static void
reset_peak (void)
{
  g_file_set_contents ("/proc/self/clear_refs", "5", 1, NULL);
}

static EaselWindow *
window_on (const char *path)
{
  g_autoptr (GFile) file = g_file_new_for_path (path);
  EaselWindow *win;
  gint64 deadline = g_get_monotonic_time () + 60 * G_USEC_PER_SEC;

  write_settings (config_dir, 64, 64);
  win = open_window ();
  easel_window_open_file (win, file);
  while (easel_window_is_loading (win) && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  if (easel_window_is_loading (win) || easel_window_get_picture (win)->width < 64)
    {
      g_printerr ("could not open %s\n", path);
      exit (1);
    }
  wait_for_paint (win);
  return win;
}

/* Remove Background, from the choice of the command to its result painted, in ms. */
static double
run (EaselWindow *win)
{
  double start = now_ms ();
  gint64 deadline = g_get_monotonic_time () + 120 * G_USEC_PER_SEC;

  g_action_group_activate_action (G_ACTION_GROUP (win), "remove-background", NULL);
  while (easel_window_is_removing_background (win) && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, TRUE);
  wait_for_paint (win);
  return now_ms () - start;
}

static guint64
hash_of (const EaselImage *img)
{
  guint64 h = 14695981039346656037u;
  const guint8 *b = (const guint8 *) img->px;

  for (size_t i = 0; i < (size_t) img->width * img->height * 4; i++)
    h = (h ^ b[i]) * 1099511628211u;
  return h;
}

/* The true mask of picture n (1 in the subject) and, from it, which pixels count: those farther than 4 pixels from its edge (no
   pixel of the other kind within 4 in x and in y). */
static guint8 *
mask_of (int n, int w, int h, guint8 **counted)
{
  g_autofree char *path = g_strdup_printf ("%s/%02d-mask.png", TEST_SET, n);
  g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new_from_file (path, NULL);
  guint8 *m;

  if (pb == NULL || gdk_pixbuf_get_width (pb) != w || gdk_pixbuf_get_height (pb) != h)
    {
      g_printerr ("bad mask %s\n", path);
      exit (1);
    }
  m = g_malloc ((gsize) w * h);
  *counted = g_malloc ((gsize) w * h);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      m[y * w + x] = gdk_pixbuf_get_pixels (pb)[y * gdk_pixbuf_get_rowstride (pb) + x * gdk_pixbuf_get_n_channels (pb)] > 127;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        gboolean far = TRUE;

        for (int dy = -4; dy <= 4 && far; dy++)
          for (int dx = -4; dx <= 4 && far; dx++)
            {
              int u = x + dx, v = y + dy;

              if (u >= 0 && v >= 0 && u < w && v < h && m[v * w + u] != m[y * w + x])
                far = FALSE;
            }
        (*counted)[y * w + x] = far;
      }
  return m;
}

/* AS-2 on picture n: returns its result's hash, prints its numbers, and sets *ok to FALSE when a rule fails. */
static guint64
as2 (int n, gboolean quiet, gboolean *ok)
{
  g_autofree char *path = g_strdup_printf ("%s/%02d.png", TEST_SET, n);
  EaselWindow *win = window_on (path);
  const EaselImage *img = easel_window_get_picture (win);
  int w = img->width, h = img->height;
  g_autofree uint32_t *before = g_memdup2 (img->px, (gsize) w * h * 4);
  g_autofree guint8 *counted = NULL;
  g_autofree guint8 *mask = mask_of (n, w, h, &counted);
  gsize bg = 0, bg_removed = 0, subj = 0, subj_kept = 0, wrong = 0;
  double ms = run (win);
  guint64 hash = hash_of (img);

  for (gsize i = 0; i < (gsize) w * h; i++)
    {
      uint32_t a = before[i], b = img->px[i];
      gboolean removed = b != a;

      if (removed && !((b >> 24) == 0 && (b & 0xFFFFFFu) == (a & 0xFFFFFFu)))
        wrong++;
      if (!counted[i])
        continue;
      if (mask[i])
        subj++, subj_kept += !removed;
      else
        bg++, bg_removed += removed;
    }
  if (g_getenv ("PERF_0035_KEEP") != NULL)       /* the results kept to look at: PERF_0035_KEEP=dir */
    {
      g_autofree char *out = g_strdup_printf ("%s/%02d-result.png", g_getenv ("PERF_0035_KEEP"), n);
      g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, w, h);

      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
          {
            guint8 *q = gdk_pixbuf_get_pixels (pb) + y * gdk_pixbuf_get_rowstride (pb) + x * 4;
            uint32_t v = img->px[y * w + x];

            q[0] = v >> 16, q[1] = v >> 8, q[2] = v, q[3] = v >> 24;
          }
      gdk_pixbuf_save (pb, out, "png", NULL, NULL);
    }
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  wait_for_paint (win);
  if (!quiet)
    {
      double pb = 100.0 * bg_removed / bg, ps = 100.0 * subj_kept / subj;
      gboolean back = memcmp (before, img->px, (gsize) w * h * 4) == 0;
      gboolean good = wrong == 0 && pb >= 95 && ps >= 95 && back;

      g_print ("%02d.png %d x %d: background removed %.2f %% (%" G_GSIZE_FORMAT " of %" G_GSIZE_FORMAT "), subject kept %.2f %% (%"
               G_GSIZE_FORMAT " of %" G_GSIZE_FORMAT "), other changes %" G_GSIZE_FORMAT ", undo %s, %.0f ms: %s\n", n, w, h, pb,
               bg_removed, bg, ps, subj_kept, subj, wrong, back ? "exact" : "DIFFERS", ms, good ? "pass" : "FAIL");
      if (!good)
        *ok = FALSE;
    }
  gtk_window_destroy (GTK_WINDOW (win));
  return hash;
}

/* A W x H photo-like picture (test-set picture 1 scaled) saved where the window can open it. */
static char *
photo (int w, int h)
{
  g_autofree char *src = g_strdup_printf ("%s/01.png", TEST_SET);
  g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new_from_file (src, NULL);
  g_autoptr (GdkPixbuf) big = pb != NULL ? gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR) : NULL;
  char *path = g_strdup_printf ("%s/photo-%dx%d.png", config_dir, w, h);

  if (big == NULL || !gdk_pixbuf_save (big, path, "png", NULL, "compression", "1", NULL))
    {
      g_printerr ("could not make %s\n", path);
      exit (1);
    }
  return path;
}

static void
time_one (int w, int h)
{
  g_autofree char *path = photo (w, h);
  EaselWindow *win = window_on (path);

  for (int i = 0; i < 3; i++)
    {
      gsize start, peak;
      double ms;

      reset_peak ();
      start = peak_bytes ();
      ms = run (win);
      peak = peak_bytes ();
      g_print ("%d x %d %s: %.0f ms, memory growth %.0f MB\n", w, h, i == 0 ? "first" : "later", ms, (peak - start) / 1048576.0);
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
      wait_for_paint (win);
    }
  gtk_window_destroy (GTK_WINDOW (win));
}

static char *
child (const char *self, const char *const *args)
{
  g_autoptr (GPtrArray) argv = g_ptr_array_new ();
  char *out = NULL;
  int status = 0;

  g_ptr_array_add (argv, (char *) self);
  for (; *args != NULL; args++)
    g_ptr_array_add (argv, (char *) *args);
  g_ptr_array_add (argv, NULL);
  if (!g_spawn_sync (NULL, (char **) argv->pdata, NULL, G_SPAWN_CHILD_INHERITS_STDIN, NULL, NULL, &out, NULL, &status, NULL)
      || !g_spawn_check_wait_status (status, NULL))
    {
      g_printerr ("a process of %s failed\n", self);
      exit (1);
    }
  return out;
}

int
main (int argc, char **argv)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  g_autofree char *self = g_file_read_link ("/proc/self/exe", NULL);
  double load[3];
  gboolean ok = TRUE;

  config_dir = dir;
  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (g_getenv ("EASEL_TEST_FINDER") == NULL && BACKGROUND_MODEL[0] != '\0')
    g_setenv ("EASEL_TEST_FINDER", BACKGROUND_MODEL, TRUE);
  if (g_getenv ("EASEL_TEST_FINDER") == NULL || !g_file_test (g_getenv ("EASEL_TEST_FINDER"), G_FILE_TEST_EXISTS))
    {
      g_printerr ("No model: set EASEL_TEST_FINDER to the IS-Net general-use ONNX file (ADR-0006).\n");
      return 77;
    }
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);

  if (argc > 1 && strcmp (argv[1], "hashes") == 0)
    {
      for (int n = 1; n <= SET; n++)
        g_print ("%016" G_GINT64_MODIFIER "x\n", as2 (n, TRUE, &ok));
      return 0;
    }
  if (argc > 1 && strcmp (argv[1], "set") == 0)
    {
      for (int n = 1; n <= SET; n++)
        as2 (n, FALSE, &ok);
      return ok ? 0 : 1;
    }
  if (argc > 3 && strcmp (argv[1], "time") == 0)
    {
      time_one (atoi (argv[2]), atoi (argv[3]));
      return 0;
    }

  if (getloadavg (load, 3) == 3)
    g_print ("load %.2f %.2f %.2f\n", load[0], load[1], load[2]);
  {
    g_autofree char *mine = NULL;
    g_autofree char *other = NULL;
    static const char *const hashes[] = { "hashes", NULL };

    for (int n = 1; n <= SET; n++)
      {
        char *line = g_strdup_printf ("%016" G_GINT64_MODIFIER "x\n", as2 (n, FALSE, &ok));
        char *joined = g_strconcat (mine != NULL ? mine : "", line, NULL);

        g_free (line);
        g_free (mine);
        mine = joined;
      }
    other = child (self, hashes);
    g_print ("a second process: %s\n", g_strcmp0 (mine, other) == 0 ? "the same pixels" : "DIFFERENT pixels");
    if (g_strcmp0 (mine, other) != 0)
      ok = FALSE;
  }
  {
    static const char *const hd[] = { "time", "1920", "1080", NULL }, *const big[] = { "time", "8000", "8000", NULL };

    for (int i = 0; i < 3; i++)
      {
        g_autofree char *a = child (self, hd);
        g_autofree char *b = child (self, big);

        g_print ("%s%s", a, b);
      }
    g_print ("(budgets: 2000 ms at 1920 x 1080, 3000 ms at 8000 x 8000, the first run included; 2048 MB beyond the picture and its "
             "step)\n");
  }
  g_object_unref (app);
  return ok ? 0 : 1;
}
