/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "preferences-dialog.h"

#include <adwaita.h>
#include <pango/pangocairo.h>
#include <string.h>

#include "settings-file.h"
#include "unit-row.h"

#define NEXT_START "Takes effect the next time Easel starts."

typedef struct
{
  EaselWindow *win;
  EaselSettings *chosen;             /* what the rows show: the window's, read at start and changed here (REQ-0020-02) */
  AdwSwitchRow *automatic;
  AdwSpinRow *width, *height;
  gboolean updating;                 /* a change made here, not by the user: it writes nothing */
  GSList *fields;                    /* the number fields, committed when the dialog closes */
  int eraser_mode, line_width_index, magnifier_index;    /* the choices' places, for settings that are not places */
} Prefs;

/* A row's binding: what it writes, and what it does at once. */
typedef void (*Apply) (Prefs *p);

typedef struct
{
  Prefs *p;
  const char *section, *key;
  int *number;
  bool *flag;
  int *index;
  const char *const *names;
  const char *on, *off;
  Apply apply;
} Bound;

static Bound *
bound_new (Prefs *p, const char *section, const char *key, Apply apply)
{
  Bound *b = g_new0 (Bound, 1);

  b->p = p;
  b->section = section;
  b->key = key;
  b->apply = apply;
  return b;
}

static void
write_one (Prefs *p, const char *section, const char *key, const char *value)
{
  EaselSettingChange change = { section, key, value };

  easel_window_write_settings (p->win, &change, 1);
}

static AdwPreferencesGroup *
add_group (AdwPreferencesPage *page, const char *title, const char *description)
{
  AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());

  adw_preferences_group_set_title (group, title);
  adw_preferences_group_set_description (group, description);
  adw_preferences_page_add (page, group);
  return group;
}

/* ---- number fields ---- */

static void
on_number_changed (AdwSpinRow *row, GParamSpec *pspec, Bound *b)
{
  int value = (int) adw_spin_row_get_value (row);
  g_autofree char *text = g_strdup_printf ("%d", value);

  if (b->p->updating)
    return;
  *b->number = value;
  write_one (b->p, b->section, b->key, text);
  if (b->apply != NULL)
    b->apply (b->p);
}

static AdwSpinRow *
add_number (Prefs *p, AdwPreferencesGroup *group, const char *title, int lower, int upper, int *number, const char *unit, const char *subtitle,
            const char *section, const char *key, Apply apply)
{
  AdwSpinRow *row = easel_unit_row_new (title, lower, upper, CLAMP (*number, lower, upper), unit);
  Bound *b = bound_new (p, section, key, apply);

  b->number = number;
  adw_action_row_set_subtitle (ADW_ACTION_ROW (row), subtitle);
  easel_unit_row_wheel_with_focus (row);
  g_signal_connect_data (row, "notify::value", G_CALLBACK (on_number_changed), b, (GClosureNotify) (void (*) (void)) g_free, 0);
  adw_preferences_group_add (group, GTK_WIDGET (row));
  p->fields = g_slist_prepend (p->fields, row);
  return row;
}

/* ---- choices ---- */

static void
on_choice_changed (AdwComboRow *row, GParamSpec *pspec, Bound *b)
{
  guint at = adw_combo_row_get_selected (row);

  if (b->p->updating || at == GTK_INVALID_LIST_POSITION)
    return;
  *b->index = (int) at;
  write_one (b->p, b->section, b->key, b->names[at]);
  if (b->apply != NULL)
    b->apply (b->p);
}

/* A value the file holds in no list shows the default (REQ-0020-06). `labels` are what the row shows, `names` what the file holds. */
static AdwComboRow *
add_choice (Prefs *p, AdwPreferencesGroup *group, const char *title, const char *const *labels, const char *const *names, int *index, const char *subtitle,
            const char *section, const char *key, Apply apply)
{
  AdwComboRow *row = ADW_COMBO_ROW (adw_combo_row_new ());
  g_autoptr (GtkStringList) model = gtk_string_list_new (labels);
  Bound *b = bound_new (p, section, key, apply);

  b->index = index;
  b->names = names;
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_action_row_set_subtitle (ADW_ACTION_ROW (row), subtitle);
  adw_combo_row_set_model (row, G_LIST_MODEL (model));
  adw_combo_row_set_selected (row, (guint) *index);
  g_signal_connect_data (row, "notify::selected", G_CALLBACK (on_choice_changed), b, (GClosureNotify) (void (*) (void)) g_free, 0);
  adw_preferences_group_add (group, GTK_WIDGET (row));
  return row;
}

/* ---- switches ---- */

static void
on_switch_changed (AdwSwitchRow *row, GParamSpec *pspec, Bound *b)
{
  gboolean on = adw_switch_row_get_active (row);

  if (b->p->updating)
    return;
  *b->flag = on;
  write_one (b->p, b->section, b->key, on ? b->on : b->off);
  if (b->apply != NULL)
    b->apply (b->p);
}

static void
add_switch (Prefs *p, AdwPreferencesGroup *group, const char *title, bool *flag, const char *on, const char *off, const char *section, const char *key,
            Apply apply)
{
  AdwSwitchRow *row = ADW_SWITCH_ROW (adw_switch_row_new ());
  Bound *b = bound_new (p, section, key, apply);

  b->flag = flag;
  b->on = on;
  b->off = off;
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_switch_row_set_active (row, *flag);
  g_signal_connect_data (row, "notify::active", G_CALLBACK (on_switch_changed), b, (GClosureNotify) (void (*) (void)) g_free, 0);
  adw_preferences_group_add (group, GTK_WIDGET (row));
}

/* ---- what applies at once ---- */

static void
refresh_automatic_size (Prefs *p)
{
  int w, h;

  if (p->automatic == NULL || !adw_switch_row_get_active (p->automatic))
    return;
  easel_window_automatic_size (p->win, &w, &h);
  p->updating = TRUE;
  adw_spin_row_set_value (p->width, w);
  adw_spin_row_set_value (p->height, h);
  p->updating = FALSE;
}

static void
apply_style (Prefs *p)
{
  easel_window_apply_style (p->win, p->chosen->style);
}

static void
apply_tint (Prefs *p)
{
  easel_window_apply_tint (p->win, p->chosen->tint);
}

static void
apply_frame_time (Prefs *p)
{
  easel_window_apply_frame_time (p->win, p->chosen->frame_time);
}

static void
apply_panels (Prefs *p)
{
  easel_window_apply_panels (p->win, p->chosen->show_toolbox, p->chosen->show_colorbox, p->chosen->show_statusbar);
  refresh_automatic_size (p);          /* the canvas area has changed */
}

static void
apply_now (Prefs *p)
{
  easel_window_apply_chosen (p->win);
}

/* ---- colors ---- */

typedef struct
{
  Prefs *p;
  const char *key;
  bool *has;
  uint32_t *rgb;
  GtkWidget *use_default;
  GtkColorDialogButton *swatch;
} ColorRow;

static void
show_workspace_swatch (ColorRow *c)
{
  const EaselSettings *s = c->p->chosen;
  gboolean dark = g_str_equal (c->key, "workspace-color-dark");
  uint32_t rgb = *c->has ? *c->rgb : dark ? (s->has_workspace_color ? s->workspace_color : 0x2C2C2Cu) : 0xE7E7E7u;
  GdkRGBA rgba = { ((rgb >> 16) & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f, (rgb & 255) / 255.0f, 1 };

  c->p->updating = TRUE;
  gtk_color_dialog_button_set_rgba (c->swatch, &rgba);
  c->p->updating = FALSE;
  gtk_widget_set_sensitive (c->use_default, *c->has);
}

static void
on_workspace_color (GtkColorDialogButton *swatch, GParamSpec *pspec, ColorRow *c)
{
  const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba (swatch);
  uint32_t rgb = (uint32_t) (rgba->red * 255 + 0.5f) << 16 | (uint32_t) (rgba->green * 255 + 0.5f) << 8 | (uint32_t) (rgba->blue * 255 + 0.5f);
  g_autofree char *text = g_strdup_printf ("#%06X", rgb);

  if (c->p->updating)
    return;
  *c->has = true;
  *c->rgb = rgb;
  write_one (c->p, "view", c->key, text);
  gtk_widget_set_sensitive (c->use_default, TRUE);
  easel_window_apply_chosen (c->p->win);
}

static void
on_workspace_default (ColorRow *c)
{
  *c->has = false;
  write_one (c->p, "view", c->key, NULL);
  show_workspace_swatch (c);
  easel_window_apply_chosen (c->p->win);
}

static void
add_workspace_color (Prefs *p, AdwPreferencesGroup *group, const char *title, const char *key, bool *has, uint32_t *rgb)
{
  AdwActionRow *row = ADW_ACTION_ROW (adw_action_row_new ());
  g_autoptr (GtkColorDialog) chooser = gtk_color_dialog_new ();
  ColorRow *c = g_new0 (ColorRow, 1);

  gtk_color_dialog_set_with_alpha (chooser, FALSE);
  c->p = p;
  c->key = key;
  c->has = has;
  c->rgb = rgb;
  c->swatch = GTK_COLOR_DIALOG_BUTTON (gtk_color_dialog_button_new (g_object_ref (chooser)));
  c->use_default = gtk_button_new_with_label ("Use Default");
  gtk_widget_set_valign (GTK_WIDGET (c->swatch), GTK_ALIGN_CENTER);
  gtk_widget_set_valign (c->use_default, GTK_ALIGN_CENTER);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_action_row_add_suffix (row, GTK_WIDGET (c->swatch));
  adw_action_row_add_suffix (row, c->use_default);
  show_workspace_swatch (c);
  g_signal_connect_data (c->swatch, "notify::rgba", G_CALLBACK (on_workspace_color), c, (GClosureNotify) (void (*) (void)) g_free, 0);
  g_signal_connect_swapped (c->use_default, "clicked", G_CALLBACK (on_workspace_default), c);
  adw_preferences_group_add (group, GTK_WIDGET (row));
}

static void
on_canvas_color (GtkColorDialogButton *swatch, GParamSpec *pspec, Prefs *p)
{
  const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba (swatch);
  uint32_t r = (uint32_t) (rgba->red * 255 + 0.5f), g = (uint32_t) (rgba->green * 255 + 0.5f), b = (uint32_t) (rgba->blue * 255 + 0.5f);
  uint32_t a = (uint32_t) (rgba->alpha * 255 + 0.5f);
  g_autofree char *text = a == 255 ? g_strdup_printf ("#%02X%02X%02X", r, g, b) : g_strdup_printf ("#%02X%02X%02X%02X", r, g, b, a);

  if (p->updating)
    return;
  p->chosen->background = a << 24 | r << 16 | g << 8 | b;
  write_one (p, "new-picture", "canvas-color", text);
  easel_window_apply_chosen (p->win);
}

static void
add_canvas_color (Prefs *p, AdwPreferencesGroup *group)
{
  AdwActionRow *row = ADW_ACTION_ROW (adw_action_row_new ());
  g_autoptr (GtkColorDialog) chooser = gtk_color_dialog_new ();
  GtkWidget *swatch = gtk_color_dialog_button_new (g_object_ref (chooser));
  uint32_t c = p->chosen->background;
  GdkRGBA rgba = { ((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f, (c >> 24) / 255.0f };

  gtk_color_dialog_set_with_alpha (chooser, TRUE);
  gtk_widget_set_valign (swatch, GTK_ALIGN_CENTER);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), "Canvas color");
  adw_action_row_add_suffix (row, swatch);
  gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON (swatch), &rgba);
  g_signal_connect (swatch, "notify::rgba", G_CALLBACK (on_canvas_color), p);
  adw_preferences_group_add (group, GTK_WIDGET (row));
}

/* ---- the palette ---- */

static void
on_reset_answer (AdwAlertDialog *dialog, const char *response, Prefs *p)
{
  if (g_str_equal (response, "reset"))
    easel_window_reset_palette (p->win);
}

static void
on_reset_palette (GtkWidget *button, Prefs *p)
{
  AdwDialog *ask = adw_alert_dialog_new ("Reset the Palette?", "All 28 colors go back to the classic ones. This cannot be undone.");

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (ask), "cancel", "_Cancel", "reset", "_Reset", NULL);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (ask), "reset", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (ask), "cancel");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (ask), "cancel");
  g_signal_connect (ask, "response", G_CALLBACK (on_reset_answer), p);
  adw_dialog_present (ask, GTK_WIDGET (p->win));
}

/* ---- New Picture ---- */

static void
on_automatic_changed (AdwSwitchRow *row, GParamSpec *pspec, Prefs *p)
{
  gboolean on = adw_switch_row_get_active (row);
  int w = (int) adw_spin_row_get_value (p->width), h = (int) adw_spin_row_get_value (p->height);
  g_autofree char *width = on ? g_strdup ("auto") : g_strdup_printf ("%d", w);
  g_autofree char *height = on ? g_strdup ("auto") : g_strdup_printf ("%d", h);
  EaselSettingChange both[] = { { "new-picture", "width", width }, { "new-picture", "height", height } };

  gtk_widget_set_sensitive (GTK_WIDGET (p->width), !on);
  gtk_widget_set_sensitive (GTK_WIDGET (p->height), !on);
  p->chosen->width = on ? EASEL_SIZE_AUTO : w;
  p->chosen->height = on ? EASEL_SIZE_AUTO : h;
  easel_window_write_settings (p->win, both, 2);                 /* both or neither (REQ-0020-04) */
  easel_window_apply_chosen (p->win);
  refresh_automatic_size (p);
}

static void
build_new_picture (Prefs *p, AdwPreferencesPage *page)
{
  AdwPreferencesGroup *group = add_group (page, "Size and color", NULL);
  gboolean automatic = p->chosen->width == EASEL_SIZE_AUTO || p->chosen->height == EASEL_SIZE_AUTO;

  p->automatic = ADW_SWITCH_ROW (adw_switch_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (p->automatic), "Automatic size");
  adw_switch_row_set_active (p->automatic, automatic);
  adw_preferences_group_add (group, GTK_WIDGET (p->automatic));
  p->width = add_number (p, group, "Width", 1, EASEL_SIZE_MAX, &p->chosen->width, "px", NULL, "new-picture", "width", apply_now);
  p->height = add_number (p, group, "Height", 1, EASEL_SIZE_MAX, &p->chosen->height, "px", NULL, "new-picture", "height", apply_now);
  gtk_widget_set_sensitive (GTK_WIDGET (p->width), !automatic);
  gtk_widget_set_sensitive (GTK_WIDGET (p->height), !automatic);
  refresh_automatic_size (p);
  g_signal_connect (p->automatic, "notify::active", G_CALLBACK (on_automatic_changed), p);
  add_canvas_color (p, group);
}

/* ---- Appearance ---- */

static void
build_appearance (Prefs *p, AdwPreferencesPage *page)
{
  static const char *const style_labels[] = { "System", "Light", "Dark", NULL };
  static const char *const style_names[] = { "system", "light", "dark" };
  EaselSettings *s = p->chosen;
  AdwPreferencesGroup *style = add_group (page, "Style", NULL);
  AdwPreferencesGroup *workspace = add_group (page, "Workspace", NULL);
  AdwPreferencesGroup *selection = add_group (page, "Selection", NULL);
  AdwPreferencesGroup *panels = add_group (page, "Panels", NULL);
  AdwPreferencesGroup *palette = add_group (page, "Palette", NULL);
  AdwActionRow *reset_row = ADW_ACTION_ROW (adw_action_row_new ());
  GtkWidget *reset = gtk_button_new_with_label ("Reset Palette…");

  add_choice (p, style, "Style", style_labels, style_names, (int *) &s->style, NULL, "view", "style", apply_style);
  add_workspace_color (p, workspace, "Workspace color", "workspace-color", &s->has_workspace_color, &s->workspace_color);
  add_workspace_color (p, workspace, "Workspace color, dark", "workspace-color-dark", &s->has_workspace_color_dark, &s->workspace_color_dark);
  add_number (p, selection, "Tint strength", EASEL_TINT_MIN, EASEL_TINT_MAX, &s->tint, "%", NULL, "view", "tint", apply_tint);
  add_switch (p, panels, "Tool Box", &s->show_toolbox, "on", "off", "view", "toolbox", apply_panels);
  add_switch (p, panels, "Color Box", &s->show_colorbox, "on", "off", "view", "colorbox", apply_panels);
  add_switch (p, panels, "Status Bar", &s->show_statusbar, "on", "off", "view", "statusbar", apply_panels);
  add_switch (p, panels, "Frame time", &s->frame_time, "on", "off", "view", "frame-time", apply_frame_time);
  gtk_widget_set_valign (reset, GTK_ALIGN_CENTER);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (reset_row), "Colors");
  adw_action_row_add_suffix (reset_row, reset);
  g_signal_connect (reset, "clicked", G_CALLBACK (on_reset_palette), p);
  adw_preferences_group_add (palette, GTK_WIDGET (reset_row));
}

/* ---- Drawing ---- */

static void
on_font_changed (AdwComboRow *row, GParamSpec *pspec, Prefs *p)
{
  guint at = adw_combo_row_get_selected (row);
  const char *family;

  if (p->updating || at == GTK_INVALID_LIST_POSITION)
    return;
  family = at == 0 ? "auto" : gtk_string_list_get_string (GTK_STRING_LIST (adw_combo_row_get_model (row)), at);
  g_strlcpy (p->chosen->text_font, at == 0 ? "" : family, sizeof p->chosen->text_font);
  write_one (p, "text", "font", family);
}

static int
compare_names (const void *a, const void *b)
{
  return g_utf8_collate (*(const char *const *) a, *(const char *const *) b);
}

/* Automatic first, then the installed families by name, with a search (REQ-0020-05). */
static void
add_font (Prefs *p, AdwPreferencesGroup *group)
{
  PangoFontFamily **families = NULL;
  int n = 0;
  AdwComboRow *row = ADW_COMBO_ROW (adw_combo_row_new ());
  g_autoptr (GtkStringList) model = gtk_string_list_new ((const char *const[]) { "Automatic", NULL });
  g_autofree const char **names = NULL;
  guint selected = 0;

  pango_font_map_list_families (pango_cairo_font_map_get_default (), &families, &n);
  names = g_new (const char *, n);
  for (int i = 0; i < n; i++)
    names[i] = pango_font_family_get_name (families[i]);
  qsort (names, (size_t) n, sizeof *names, compare_names);
  for (int i = 0; i < n; i++)
    {
      gtk_string_list_append (model, names[i]);
      if (g_ascii_strcasecmp (names[i], p->chosen->text_font) == 0)
        selected = (guint) i + 1;
    }
  g_free (families);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), "Font");
  adw_combo_row_set_model (row, G_LIST_MODEL (model));
  adw_combo_row_set_expression (row, gtk_property_expression_new (GTK_TYPE_STRING_OBJECT, NULL, "string"));
  adw_combo_row_set_enable_search (row, TRUE);
  adw_combo_row_set_selected (row, selected);
  g_signal_connect (row, "notify::selected", G_CALLBACK (on_font_changed), p);
  adw_preferences_group_add (group, GTK_WIDGET (row));
}

static void
sync_eraser_mode (Prefs *p)
{
  p->chosen->eraser_transparent = p->eraser_mode == 1;
}

static void
sync_line_width (Prefs *p)
{
  p->chosen->line_width = p->line_width_index + 1;
}

static const int magnifier_levels[] = { 1, 2, 4, 6, 8, 16 };

static void
sync_magnifier (Prefs *p)
{
  p->chosen->magnifier = magnifier_levels[p->magnifier_index];
}

static void
build_drawing (Prefs *p, AdwPreferencesPage *page)
{
  static const char *const brush_labels[] = { "Circle, 7 pixels", "Circle, 4 pixels", "Circle, 1 pixel", "Square, 8 pixels", "Square, 5 pixels", "Square, 2 pixels",
                                              "Slash, 8 pixels", "Slash, 5 pixels", "Slash, 2 pixels", "Backslash, 8 pixels", "Backslash, 5 pixels", "Backslash, 2 pixels", NULL };
  static const char *brush_names[13];
  static const char *const air_labels[] = { "9 pixels", "16 pixels", "24 pixels", NULL };
  static const char *const air_names[] = { "9", "16", "24" };
  static const char *const eraser_labels[] = { "4 pixels", "6 pixels", "8 pixels", "10 pixels", NULL };
  static const char *const eraser_names[] = { "4", "6", "8", "10" };
  static const char *const mode_labels[] = { "Opaque", "Transparent", NULL };
  static const char *const mode_names[] = { "opaque", "transparent" };
  static const char *const width_labels[] = { "1 pixel", "2 pixels", "3 pixels", "4 pixels", "5 pixels", NULL };
  static const char *const width_names[] = { "1", "2", "3", "4", "5" };
  static const char *const fill_labels[] = { "Outline", "Outline and fill", "Fill", NULL };
  static const char *const fill_names[] = { "outline", "outline-and-fill", "fill" };
  static const char *const level_labels[] = { "1x", "2x", "4x", "6x", "8x", "16x", NULL };
  static const char *const level_names[] = { "1", "2", "4", "6", "8", "16" };
  EaselSettings *s = p->chosen;
  AdwPreferencesGroup *tools = add_group (page, "Tools", NULL);
  AdwPreferencesGroup *text = add_group (page, "Text", NEXT_START);
  AdwPreferencesGroup *history = add_group (page, "History", NULL);
  AdwPreferencesGroup *saving = add_group (page, "Saving", NULL);

  for (int i = 0; i < 12; i++)
    brush_names[i] = easel_settings_brush_name (i);
  p->eraser_mode = s->eraser_transparent ? 1 : 0;
  p->line_width_index = s->line_width - 1;
  for (guint i = 0; i < G_N_ELEMENTS (magnifier_levels); i++)
    if (magnifier_levels[i] == s->magnifier)
      p->magnifier_index = (int) i;
  add_choice (p, tools, "Brush", brush_labels, brush_names, &s->brush, NEXT_START, "drawing", "brush", NULL);
  add_choice (p, tools, "Airbrush", air_labels, air_names, &s->airbrush, NEXT_START, "drawing", "airbrush", NULL);
  add_choice (p, tools, "Eraser size", eraser_labels, eraser_names, &s->eraser, NEXT_START, "drawing", "eraser", NULL);
  add_choice (p, tools, "Eraser mode", mode_labels, mode_names, &p->eraser_mode, NEXT_START, "drawing", "eraser-mode", sync_eraser_mode);
  add_choice (p, tools, "Line and shape width", width_labels, width_names, &p->line_width_index, NEXT_START, "drawing", "line-width", sync_line_width);
  add_choice (p, tools, "Fill style", fill_labels, fill_names, (int *) &s->fill_style, NEXT_START, "drawing", "fill-style", NULL);
  add_choice (p, tools, "Magnifier level", level_labels, level_names, &p->magnifier_index, NEXT_START, "drawing", "magnifier", sync_magnifier);
  add_font (p, text);
  add_number (p, text, "Size", EASEL_TEXT_SIZE_MIN, EASEL_TEXT_SIZE_MAX, &s->text_size, "pt", NULL, "text", "size", NULL);
  add_switch (p, text, "Bold", &s->text_bold, "true", "false", "text", "bold", NULL);
  add_switch (p, text, "Italic", &s->text_italic, "true", "false", "text", "italic", NULL);
  add_switch (p, text, "Underline", &s->text_underline, "true", "false", "text", "underline", NULL);
  add_switch (p, text, "Strikethrough", &s->text_strike, "true", "false", "text", "strikethrough", NULL);
  add_number (p, history, "Undo memory", EASEL_UNDO_MB_MIN, EASEL_UNDO_MB_MAX, &s->undo_budget_mb, "MB", NEXT_START, "history", "memory", NULL);
  add_number (p, saving, "JPEG quality", EASEL_JPEG_QUALITY_MIN, EASEL_JPEG_QUALITY_MAX, &s->jpeg_quality, NULL, NULL, "save", "jpeg-quality", apply_now);
}

/* ---- the dialog ---- */

static void
commit_field (gpointer row, gpointer data)
{
  adw_spin_row_update (ADW_SPIN_ROW (row));
}

/* A typed value counts when the dialog closes (REQ-0020-06). */
static void
on_closed (AdwDialog *dialog, Prefs *p)
{
  g_slist_foreach (p->fields, commit_field, NULL);
  easel_window_preferences_closed (p->win);
}

static void
free_prefs (gpointer data, GObject *where_the_dialog_was)
{
  Prefs *p = data;

  g_slist_free (p->fields);
  g_free (p);
}

static AdwPreferencesPage *
add_page (AdwPreferencesDialog *dialog, const char *title, const char *name, const char *icon)
{
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());

  adw_preferences_page_set_title (page, title);
  adw_preferences_page_set_name (page, name);
  adw_preferences_page_set_icon_name (page, icon);
  adw_preferences_dialog_add (dialog, page);
  return page;
}

void
easel_preferences_dialog_present (EaselWindow *win)
{
  AdwPreferencesDialog *dialog = ADW_PREFERENCES_DIALOG (adw_preferences_dialog_new ());
  Prefs *p = g_new0 (Prefs, 1);

  p->win = win;
  p->chosen = easel_window_get_chosen_settings (win);
  adw_dialog_set_title (ADW_DIALOG (dialog), "Preferences");
  build_appearance (p, add_page (dialog, "Appearance", "appearance", "applications-graphics-symbolic"));
  build_new_picture (p, add_page (dialog, "New Picture", "new-picture", "document-new-symbolic"));
  build_drawing (p, add_page (dialog, "Drawing", "drawing", "document-edit-symbolic"));
  g_signal_connect (dialog, "closed", G_CALLBACK (on_closed), p);
  g_object_weak_ref (G_OBJECT (dialog), free_prefs, p);
  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (win));
  easel_window_preferences_opened (win, ADW_DIALOG (dialog));
}
