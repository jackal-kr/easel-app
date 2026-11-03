/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Print (0026): what a print leaves for the next, the tab "Picture", and a print job: a copy of the picture as it was confirmed, drawn sheet by sheet on any cairo context. */
#pragma once

#include <cairo.h>
#include <gtk/gtk.h>

#include "image.h"
#include "print-layout.h"

G_BEGIN_DECLS

typedef enum
{
  EASEL_PRINT_SHARP,
  EASEL_PRINT_SMOOTH_WHEN_SHRUNK,
  EASEL_PRINT_SMOOTH,
} EaselPrintPixels;

typedef struct
{
  double paper_w_mm, paper_h_mm;  /* as the orientation turns them */
  int margins_mm;
  gboolean fit;
  int percent;
  gboolean center_h, center_v;
  EaselPrintPixels pixels;
} EaselPrintPage;

/* What a confirmed print leaves for the next, until the app quits (REQ-0026-04). */
typedef struct
{
  char *paper;                    /* a paper size's name */
  gboolean landscape;
  gboolean orientation_set;       /* else the orientation follows the picture */
  gboolean fit;
  EaselPrintPixels pixels;
} EaselPageSetup;

/* The starting values (REQ-0026-04). */
void easel_page_setup_init (EaselPageSetup *setup);
void easel_page_setup_clear (EaselPageSetup *setup);
/* The orientation for a picture of w x h. */
gboolean easel_page_setup_landscape (const EaselPageSetup *setup, int w, int h);
/* The page the setup gives for a picture of w x h. */
void easel_page_setup_page (const EaselPageSetup *setup, int w, int h, EaselPrintPage *page);

/* The tab "Picture" of the print dialog (REQ-0026-03, -10), for a picture of w x h. */
GtkWidget *easel_print_tab_new (const EaselPageSetup *setup, int w, int h);
/* The paper as the dialog has it now, its orientation in the sizes: the count of sheets follows. */
void easel_print_tab_set_paper (GtkWidget *tab, double paper_w_mm, double paper_h_mm);
/* Copies the tab's choices to setup. */
void easel_print_tab_read (GtkWidget *tab, EaselPageSetup *setup);

typedef struct _EaselPrintJob EaselPrintJob;

/* The flattened copy of the picture (REQ-0026-14), one allocation of its size; NULL with no memory (REQ-0026-15). */
EaselPrintJob *easel_print_job_new (const EaselImage *picture, const EaselPrintPage *page);
/* The same, refusing a copy above max_bytes (a test's way to no memory). */
EaselPrintJob *easel_print_job_new_limited (const EaselImage *picture, const EaselPrintPage *page, gsize max_bytes);
void easel_print_job_free (EaselPrintJob *job);
int easel_print_job_sheets (const EaselPrintJob *job);
double easel_print_job_percent (const EaselPrintJob *job);
/* Draws a sheet on cr, whose unit is one dot of dots_per_inch (REQ-0026-07 to -09). */
void easel_print_job_draw_sheet (const EaselPrintJob *job, cairo_t *cr, int index, double dots_per_inch);

G_END_DECLS
