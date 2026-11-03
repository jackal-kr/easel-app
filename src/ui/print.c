/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "print.h"

#include <adwaita.h>
#include <math.h>

#define MARGINS_MM 19

struct _EaselPrintJob
{
  uint32_t *px;              /* opaque, flattened on white */
  int width, height;
  EaselPrintPage page;
  EaselPrintLayout layout;
  double area_w_mm, area_h_mm;
};

void
easel_page_setup_init (EaselPageSetup *setup)
{
  g_autoptr (GtkPaperSize) paper = gtk_paper_size_new (NULL);

  *setup = (EaselPageSetup) { .paper = g_strdup (gtk_paper_size_get_name (paper)), .fit = TRUE, .pixels = EASEL_PRINT_SHARP };
}

void
easel_page_setup_clear (EaselPageSetup *setup)
{
  g_clear_pointer (&setup->paper, g_free);
}

gboolean
easel_page_setup_landscape (const EaselPageSetup *setup, int w, int h)
{
  return setup->orientation_set ? setup->landscape : easel_print_wide_is_landscape (w, h);
}

void
easel_page_setup_page (const EaselPageSetup *setup, int w, int h, EaselPrintPage *page)
{
  g_autoptr (GtkPaperSize) paper = gtk_paper_size_new (setup->paper);
  double a = gtk_paper_size_get_width (paper, GTK_UNIT_MM), b = gtk_paper_size_get_height (paper, GTK_UNIT_MM);
  gboolean landscape = easel_page_setup_landscape (setup, w, h);

  *page = (EaselPrintPage) { .paper_w_mm = landscape ? b : a, .paper_h_mm = landscape ? a : b, .margins_mm = MARGINS_MM, .fit = setup->fit,
                             .percent = 100, .center_h = TRUE, .center_v = TRUE, .pixels = setup->pixels };
}

typedef struct
{
  int w, h;
  double paper_w_mm, paper_h_mm;
  AdwSwitchRow *fit;
  GtkLabel *sheets;
  GtkToggleButton *smooth_shrunk, *smooth;
} PrintTab;

static void
tab_sync (PrintTab *t)
{
  gboolean fit = adw_switch_row_get_active (t->fit);
  EaselPrintLayout l = easel_print_layout (t->w, t->h, t->paper_w_mm - 2 * MARGINS_MM, t->paper_h_mm - 2 * MARGINS_MM, fit, 100);
  int n = easel_print_sheets (&l);
  g_autofree char *text = g_strdup_printf ("Takes %d sheets", n);

  gtk_label_set_text (t->sheets, text);
  gtk_widget_set_visible (GTK_WIDGET (t->sheets), n > 1);
}

static GtkToggleButton *
tab_choice (GtkWidget *row, GtkToggleButton *group, const char *label, const char *name, const char *tooltip, gboolean active)
{
  GtkWidget *b = gtk_toggle_button_new_with_label (label);

  gtk_widget_set_name (b, name);
  gtk_widget_set_tooltip_text (b, tooltip);
  if (group != NULL)
    gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (b), group);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), active);
  gtk_box_append (GTK_BOX (row), b);
  return GTK_TOGGLE_BUTTON (b);
}

GtkWidget *
easel_print_tab_new (const EaselPageSetup *setup, int w, int h)
{
  PrintTab *t = g_new0 (PrintTab, 1);
  GtkWidget *tab = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8), *heading = gtk_label_new ("Pixels"), *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *list = gtk_list_box_new (), *sheets = gtk_label_new (NULL), *fit = adw_switch_row_new ();
  GtkToggleButton *sharp;
  EaselPrintPage page;

  easel_page_setup_page (setup, w, h, &page);
  t->w = w;
  t->h = h;
  t->paper_w_mm = page.paper_w_mm;
  t->paper_h_mm = page.paper_h_mm;
  t->fit = ADW_SWITCH_ROW (fit);
  t->sheets = GTK_LABEL (sheets);
  g_object_set_data_full (G_OBJECT (tab), "easel-print-tab", t, g_free);
  gtk_widget_set_margin_start (tab, 12);
  gtk_widget_set_margin_end (tab, 12);
  gtk_widget_set_margin_top (tab, 12);
  gtk_widget_set_margin_bottom (tab, 12);
  gtk_widget_set_halign (heading, GTK_ALIGN_START);
  gtk_widget_add_css_class (heading, "heading");
  gtk_widget_add_css_class (row, "linked");
  gtk_widget_set_halign (row, GTK_ALIGN_START);
  sharp = tab_choice (row, NULL, "Sharp", "sharp", "Each pixel a sharp square", setup->pixels == EASEL_PRINT_SHARP);
  t->smooth_shrunk = tab_choice (row, sharp, "Smooth when shrunk", "smooth-when-shrunk", "Sharp, but averaged where a printer's dot covers several pixels",
                                 setup->pixels == EASEL_PRINT_SMOOTH_WHEN_SHRUNK);
  t->smooth = tab_choice (row, sharp, "Smooth", "smooth", "Blended between pixels", setup->pixels == EASEL_PRINT_SMOOTH);
  gtk_box_append (GTK_BOX (tab), heading);
  gtk_box_append (GTK_BOX (tab), row);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (fit), "Fit on one page");
  gtk_widget_set_name (fit, "fit");
  adw_switch_row_set_active (t->fit, setup->fit);
  gtk_list_box_append (GTK_LIST_BOX (list), fit);
  gtk_box_append (GTK_BOX (tab), list);
  gtk_widget_set_name (sheets, "sheets");
  gtk_widget_set_halign (sheets, GTK_ALIGN_START);
  gtk_box_append (GTK_BOX (tab), sheets);
  g_signal_connect_swapped (fit, "notify::active", G_CALLBACK (tab_sync), t);
  tab_sync (t);
  return tab;
}

void
easel_print_tab_set_paper (GtkWidget *tab, double paper_w_mm, double paper_h_mm)
{
  PrintTab *t = g_object_get_data (G_OBJECT (tab), "easel-print-tab");

  t->paper_w_mm = paper_w_mm;
  t->paper_h_mm = paper_h_mm;
  tab_sync (t);
}

void
easel_print_tab_read (GtkWidget *tab, EaselPageSetup *setup)
{
  PrintTab *t = g_object_get_data (G_OBJECT (tab), "easel-print-tab");

  setup->fit = adw_switch_row_get_active (t->fit);
  setup->pixels = gtk_toggle_button_get_active (t->smooth) ? EASEL_PRINT_SMOOTH
                  : gtk_toggle_button_get_active (t->smooth_shrunk) ? EASEL_PRINT_SMOOTH_WHEN_SHRUNK : EASEL_PRINT_SHARP;
}

EaselPrintJob *
easel_print_job_new (const EaselImage *picture, const EaselPrintPage *page)
{
  return easel_print_job_new_limited (picture, page, G_MAXSIZE);
}

EaselPrintJob *
easel_print_job_new_limited (const EaselImage *picture, const EaselPrintPage *page, gsize max_bytes)
{
  EaselPrintJob *job = g_new0 (EaselPrintJob, 1);
  gsize n = (gsize) picture->width * (gsize) picture->height;

  job->px = n * sizeof *job->px <= max_bytes ? g_try_malloc (n * sizeof *job->px) : NULL;
  if (job->px == NULL)
    {
      g_free (job);
      return NULL;
    }
  for (gsize i = 0; i < n; i++)
    {
      uint8_t rgb[3];

      easel_flatten_on_white (&picture->px[i], rgb, 1);
      job->px[i] = 0xFF000000u | ((uint32_t) rgb[0] << 16) | ((uint32_t) rgb[1] << 8) | rgb[2];
    }
  job->width = picture->width;
  job->height = picture->height;
  job->page = *page;
  job->area_w_mm = page->paper_w_mm - 2 * page->margins_mm;
  job->area_h_mm = page->paper_h_mm - 2 * page->margins_mm;
  job->layout = easel_print_layout (job->width, job->height, job->area_w_mm, job->area_h_mm, page->fit, page->percent);
  return job;
}

void
easel_print_job_free (EaselPrintJob *job)
{
  g_free (job->px);
  g_free (job);
}

double
easel_print_job_percent (const EaselPrintJob *job)
{
  return job->layout.percent;
}

int
easel_print_job_sheets (const EaselPrintJob *job)
{
  return easel_print_sheets (&job->layout);
}

void
easel_print_job_draw_sheet (const EaselPrintJob *job, cairo_t *cr, int index, double dots_per_inch)
{
  double dots_per_mm = dots_per_inch / 25.4, x_mm, y_mm;
  double dots_per_pixel = dots_per_inch / 96 * job->layout.percent / 100;
  int x, y, w, h;
  g_autofree uint32_t *reduced = NULL;
  const uint32_t *src;
  int src_w, src_h, stride;
  cairo_surface_t *surface;
  double scale = dots_per_pixel;
  /* A context of few dots, as a file's 72 per inch, still reduces at a printer's fineness (R3). */
  double reduce_dpi = MAX (dots_per_inch, 300);
  double reduce_dots_per_pixel = reduce_dpi / 96 * job->layout.percent / 100;

  easel_print_sheet (&job->layout, job->width, job->height, index, &x, &y, &w, &h);
  easel_print_place (&job->layout, job->width, job->height, job->area_w_mm, job->area_h_mm, job->page.center_h, job->page.center_v, &x_mm, &y_mm);
  src = job->px + (gsize) y * job->width + x;
  src_w = w;
  src_h = h;
  stride = job->width;
  if (job->page.pixels == EASEL_PRINT_SMOOTH_WHEN_SHRUNK && reduce_dots_per_pixel < 1)
    {
      int dots_w = (int) ceil (w * reduce_dots_per_pixel), dots_h = (int) ceil (h * reduce_dots_per_pixel);

      /* With no memory for this, the sheet's own pixels are drawn: Sharp, for this sheet. */
      reduced = g_try_malloc ((gsize) dots_w * dots_h * sizeof *reduced);
      if (reduced != NULL)
        {
          easel_print_average (src, w, h, (size_t) job->width, 1 / reduce_dots_per_pixel, reduced, dots_w, dots_h);
          src = reduced;
          src_w = dots_w;
          src_h = dots_h;
          stride = dots_w;
          scale = dots_per_inch / reduce_dpi;
        }
    }
  surface = cairo_image_surface_create_for_data ((unsigned char *) src, CAIRO_FORMAT_ARGB32, src_w, src_h, stride * (int) sizeof (uint32_t));
  cairo_save (cr);
  cairo_translate (cr, (job->page.margins_mm + x_mm) * dots_per_mm, (job->page.margins_mm + y_mm) * dots_per_mm);
  cairo_scale (cr, scale, scale);
  cairo_rectangle (cr, 0, 0, src_w, src_h);
  cairo_clip (cr);
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_pattern_set_filter (cairo_get_source (cr), job->page.pixels == EASEL_PRINT_SMOOTH ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_surface_finish (surface);
  cairo_surface_destroy (surface);
}
