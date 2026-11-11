/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "preferences-dialog.h"

#include <adwaita.h>
#include <pango/pangocairo.h>
#include <string.h>

#include "settings-file.h"
#include "unit-row.h"
#include "catalog.h"
#include "i18n.h"

#define NEXT_START _ ("Takes effect the next time Easel starts.")

/* A key of the settings file: what its row would write now, in the state to go back to, and in the file as far as the dialog knows.
   NULL is no key. A failed write leaves `in_file`, so that what is owing is tried again (REQ-0027-12). */
typedef struct
{
  const char *section, *key;
  char *now, *kept, *in_file;
} Key;

typedef struct
{
  EaselWindow *win;
  EaselSettings *chosen;             /* the window's: what counts, changed only by a keep */
  EaselSettings draft;               /* what the rows hold (REQ-0027-05) */
  EaselSettings state;               /* the state to go back to (REQ-0027-04), with the two below */
  gboolean shown[3];                 /* each panel's shown state */
  EaselPalette palette;
  GPtrArray *keys;                   /* in the order of the rows (REQ-0027-07) */
  Key *width_key, *height_key, *language_key;
  GtkWidget *apply;
  AdwSwitchRow *automatic;
  AdwSpinRow *width, *height;
  gboolean updating;                 /* a change made here, not by the user: it changes no row's value */
  GSList *fields;                    /* the number fields */
  int language_index;
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
  Key *written;
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
key_start (Key *k, const char *value)
{
  g_free (k->now);
  g_free (k->kept);
  g_free (k->in_file);
  k->now = g_strdup (value);
  k->kept = g_strdup (value);
  k->in_file = g_strdup (value);
}

static Key *
key_new (Prefs *p, const char *section, const char *key, const char *value)
{
  Key *k = g_new0 (Key, 1);

  k->section = section;
  k->key = key;
  key_start (k, value);
  g_ptr_array_add (p->keys, k);
  return k;
}

static void
key_free (gpointer data)
{
  Key *k = data;

  g_free (k->now);
  g_free (k->kept);
  g_free (k->in_file);
  g_free (k);
}

/* Apply follows what differs, what is owing and text not yet committed (REQ-0027-04). */
static void
update_apply (Prefs *p)
{
  gboolean differs = memcmp (easel_window_get_palette (p->win), &p->palette, sizeof p->palette) != 0;

  for (guint i = 0; i < p->keys->len && !differs; i++)
    {
      Key *k = p->keys->pdata[i];

      differs = g_strcmp0 (k->now, k->kept) != 0 || g_strcmp0 (k->now, k->in_file) != 0;
    }
  for (GSList *l = p->fields; l != NULL && !differs; l = l->next)
    differs = easel_unit_row_is_typed (l->data);
  if (p->apply != NULL)
    gtk_widget_set_sensitive (p->apply, differs);
}

static void
key_set (Prefs *p, Key *k, const char *value)
{
  g_free (k->now);
  k->now = g_strdup (value);
  update_apply (p);
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
  key_set (b->p, b->written, text);
  if (b->apply != NULL)
    b->apply (b->p);
}

static void
on_field_text (GtkEditable *row, Prefs *p)
{
  update_apply (p);
}

static AdwSpinRow *
add_number (Prefs *p, AdwPreferencesGroup *group, const char *title, int lower, int upper, int *number, const char *unit, const char *subtitle,
            const char *section, const char *key, Apply apply)
{
  AdwSpinRow *row = easel_unit_row_new (title, lower, upper, CLAMP (*number, lower, upper), unit);
  Bound *b = bound_new (p, section, key, apply);
  g_autofree char *shown = g_strdup_printf ("%d", CLAMP (*number, lower, upper));

  b->number = number;
  b->written = key_new (p, section, key, shown);
  g_signal_connect (row, "changed", G_CALLBACK (on_field_text), p);
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
  key_set (b->p, b->written, b->names[at]);
  if (b->apply != NULL)
    b->apply (b->p);
}

/* A value the file holds in no list shows the default (REQ-0020-06). `labels` are what the row shows, `names` what the file holds. */
static AdwComboRow *
add_choice (Prefs *p, AdwPreferencesGroup *group, const char *title, const char *const *labels, const char *const *names, int *index, const char *subtitle,
            const char *section, const char *key, Apply apply)
{
  AdwComboRow *row = ADW_COMBO_ROW (adw_combo_row_new ());
  g_autoptr (GtkStringList) model = gtk_string_list_new (NULL);
  Bound *b = bound_new (p, section, key, apply);

  for (const char *const *l = labels; *l != NULL; l++)
    gtk_string_list_append (model, _ (*l));

  b->index = index;
  b->names = names;
  b->written = key_new (p, section, key, names[*index]);
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
  key_set (b->p, b->written, on ? b->on : b->off);
  if (b->apply != NULL)
    b->apply (b->p);
}

static AdwSwitchRow *
add_switch (Prefs *p, AdwPreferencesGroup *group, const char *title, bool *flag, const char *on, const char *off, const char *section, const char *key,
            Apply apply)
{
  AdwSwitchRow *row = ADW_SWITCH_ROW (adw_switch_row_new ());
  Bound *b = bound_new (p, section, key, apply);

  b->flag = flag;
  b->on = on;
  b->off = off;
  b->written = key_new (p, section, key, *flag ? on : off);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_switch_row_set_active (row, *flag);
  g_signal_connect_data (row, "notify::active", G_CALLBACK (on_switch_changed), b, (GClosureNotify) (void (*) (void)) g_free, 0);
  adw_preferences_group_add (group, GTK_WIDGET (row));
  return row;
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
  easel_window_apply_style (p->win, p->draft.style);
}

static void
apply_tint (Prefs *p)
{
  easel_window_apply_tint (p->win, p->draft.tint);
}

static void
apply_frame_time (Prefs *p)
{
  easel_window_apply_frame_time (p->win, p->draft.frame_time);
}

/* A switch away from the state makes the panel match it; back at the state, the panel is as the state has it (REQ-0027-06). */
static void
apply_panels (Prefs *p)
{
  easel_window_apply_panels (p->win,
                             p->draft.show_toolbox == p->state.show_toolbox ? p->shown[0] : p->draft.show_toolbox,
                             p->draft.show_colorbox == p->state.show_colorbox ? p->shown[1] : p->draft.show_colorbox,
                             p->draft.show_statusbar == p->state.show_statusbar ? p->shown[2] : p->draft.show_statusbar);
  refresh_automatic_size (p);          /* the canvas area has changed */
}

static gboolean
panel_is_shown (Prefs *p, const char *action)
{
  g_autoptr (GVariant) state = g_action_group_get_action_state (G_ACTION_GROUP (p->win), action);

  return g_variant_get_boolean (state);
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
  Key *written;
} ColorRow;

static void
show_workspace_swatch (ColorRow *c)
{
  const EaselSettings *s = &c->p->draft;
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
  key_set (c->p, c->written, text);
  gtk_widget_set_sensitive (c->use_default, TRUE);
  easel_window_preview_workspace (c->p->win, &c->p->draft);
}

static void
on_workspace_default (ColorRow *c)
{
  *c->has = false;
  key_set (c->p, c->written, NULL);
  show_workspace_swatch (c);
  easel_window_preview_workspace (c->p->win, &c->p->draft);
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
  {
    g_autofree char *kept = *has ? g_strdup_printf ("#%06X", *rgb & 0xFFFFFFu) : NULL;

    c->written = key_new (p, "view", key, kept);
  }
  c->swatch = GTK_COLOR_DIALOG_BUTTON (gtk_color_dialog_button_new (g_object_ref (chooser)));
  c->use_default = gtk_button_new_with_label (_ ("Use Default"));
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

static char *
canvas_color_text (uint32_t c)
{
  uint32_t a = c >> 24;

  return a == 255 ? g_strdup_printf ("#%06X", c & 0xFFFFFFu) : g_strdup_printf ("#%06X%02X", c & 0xFFFFFFu, a);
}

static void
on_canvas_color (GtkColorDialogButton *swatch, GParamSpec *pspec, Key *k)
{
  Prefs *p = g_object_get_data (G_OBJECT (swatch), "easel-prefs");
  const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba (swatch);
  uint32_t r = (uint32_t) (rgba->red * 255 + 0.5f), g = (uint32_t) (rgba->green * 255 + 0.5f), b = (uint32_t) (rgba->blue * 255 + 0.5f);
  uint32_t a = (uint32_t) (rgba->alpha * 255 + 0.5f);
  g_autofree char *text = canvas_color_text (a << 24 | r << 16 | g << 8 | b);

  if (p->updating)
    return;
  p->draft.background = a << 24 | r << 16 | g << 8 | b;
  key_set (p, k, text);
}

static void
add_canvas_color (Prefs *p, AdwPreferencesGroup *group)
{
  AdwActionRow *row = ADW_ACTION_ROW (adw_action_row_new ());
  g_autoptr (GtkColorDialog) chooser = gtk_color_dialog_new ();
  GtkWidget *swatch = gtk_color_dialog_button_new (g_object_ref (chooser));
  uint32_t c = p->draft.background;
  g_autofree char *kept = canvas_color_text (c);
  GdkRGBA rgba = { ((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f, (c >> 24) / 255.0f };

  gtk_color_dialog_set_with_alpha (chooser, TRUE);
  gtk_widget_set_valign (swatch, GTK_ALIGN_CENTER);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Canvas color"));
  adw_action_row_add_suffix (row, swatch);
  gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON (swatch), &rgba);
  g_object_set_data (G_OBJECT (swatch), "easel-prefs", p);
  g_signal_connect (swatch, "notify::rgba", G_CALLBACK (on_canvas_color), key_new (p, "new-picture", "canvas-color", kept));
  adw_preferences_group_add (group, GTK_WIDGET (row));
}

/* ---- the palette ---- */

/* The classic colors, as a change like any other (REQ-0027-14). */
static void
on_reset_palette (GtkWidget *button, Prefs *p)
{
  EaselPalette classic;

  easel_palette_defaults (&classic);
  easel_window_set_palette (p->win, &classic);
  update_apply (p);
}

/* ---- New Picture ---- */

/* The switch owns both keys; off, it shows the state's numbers if the state's switch was off, else the size shown (REQ-0027-09). */
static void
on_automatic_changed (AdwSwitchRow *row, GParamSpec *pspec, Prefs *p)
{
  gboolean on = adw_switch_row_get_active (row);
  gboolean state_on = p->state.width == EASEL_SIZE_AUTO || p->state.height == EASEL_SIZE_AUTO;
  int w = state_on ? (int) adw_spin_row_get_value (p->width) : p->state.width;
  int h = state_on ? (int) adw_spin_row_get_value (p->height) : p->state.height;
  g_autofree char *width = on ? g_strdup ("auto") : g_strdup_printf ("%d", w);
  g_autofree char *height = on ? g_strdup ("auto") : g_strdup_printf ("%d", h);

  gtk_widget_set_sensitive (GTK_WIDGET (p->width), !on);
  gtk_widget_set_sensitive (GTK_WIDGET (p->height), !on);
  p->draft.width = on ? EASEL_SIZE_AUTO : w;
  p->draft.height = on ? EASEL_SIZE_AUTO : h;
  p->updating = TRUE;
  if (!on)
    {
      adw_spin_row_set_value (p->width, w);
      adw_spin_row_set_value (p->height, h);
    }
  p->updating = FALSE;
  key_set (p, p->width_key, width);
  key_set (p, p->height_key, height);
  refresh_automatic_size (p);
}

static void
build_new_picture (Prefs *p, AdwPreferencesPage *page)
{
  AdwPreferencesGroup *group = add_group (page, _ ("Size and color"), NULL);
  gboolean automatic = p->draft.width == EASEL_SIZE_AUTO || p->draft.height == EASEL_SIZE_AUTO;

  p->automatic = ADW_SWITCH_ROW (adw_switch_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (p->automatic), _ ("Automatic size"));
  adw_switch_row_set_active (p->automatic, automatic);
  adw_preferences_group_add (group, GTK_WIDGET (p->automatic));
  p->width = add_number (p, group, _ ("Width"), 1, EASEL_SIZE_MAX, &p->draft.width, "px", NULL, "new-picture", "width", NULL);
  p->height = add_number (p, group, _ ("Height"), 1, EASEL_SIZE_MAX, &p->draft.height, "px", NULL, "new-picture", "height", NULL);
  p->width_key = p->keys->pdata[p->keys->len - 2];
  p->height_key = p->keys->pdata[p->keys->len - 1];
  if (automatic)
    {
      key_start (p->width_key, "auto");
      key_start (p->height_key, "auto");
    }
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
  static const char *const style_labels[] = { N_ ("System"), N_ ("Light"), N_ ("Dark"), NULL };
  static const char *const style_names[] = { "system", "light", "dark" };
  EaselSettings *s = &p->draft;
  AdwPreferencesGroup *style = add_group (page, _ ("Style"), NULL);
  AdwPreferencesGroup *workspace = add_group (page, _ ("Workspace"), NULL);
  AdwPreferencesGroup *selection = add_group (page, _ ("Selection"), NULL);
  AdwPreferencesGroup *palette = add_group (page, _ ("Palette"), NULL);
  AdwActionRow *reset_row = ADW_ACTION_ROW (adw_action_row_new ());
  GtkWidget *reset = gtk_button_new_with_label (_ ("Reset Palette"));

  add_choice (p, style, _ ("Style"), style_labels, style_names, (int *) &s->style, NULL, "view", "style", apply_style);
  add_workspace_color (p, workspace, _ ("Workspace color"), "workspace-color", &s->has_workspace_color, &s->workspace_color);
  add_workspace_color (p, workspace, _ ("Workspace color, dark"), "workspace-color-dark", &s->has_workspace_color_dark, &s->workspace_color_dark);
  add_number (p, selection, _ ("Tint strength"), EASEL_TINT_MIN, EASEL_TINT_MAX, &s->tint, "%", NULL, "view", "tint", apply_tint);
  gtk_widget_set_valign (reset, GTK_ALIGN_CENTER);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (reset_row), _ ("Colors"));
  adw_action_row_add_suffix (reset_row, reset);
  g_signal_connect (reset, "clicked", G_CALLBACK (on_reset_palette), p);
  adw_preferences_group_add (palette, GTK_WIDGET (reset_row));
}

/* ---- Interface ---- */

static void
sync_language (Prefs *p)
{
  g_strlcpy (p->draft.language, p->language_index == 0 ? "auto" : easel_language_code (p->language_index - 1), sizeof p->draft.language);
}

static void
build_interface (Prefs *p, AdwPreferencesPage *page)
{
  EaselSettings *s = &p->draft;
  AdwPreferencesGroup *language = add_group (page, _ ("Language"), NULL);
  AdwPreferencesGroup *panels = add_group (page, _ ("Panels"), NULL);

  /* "System default", then the twelve by their own names in the order of their codes: never translated, fixed in every language (REQ-0023-04). */
  {
    static const char *labels[14] = { N_ ("System default") }, *codes[14] = { "auto" };

    for (int i = 0; i < easel_language_count (); i++)
      {
        labels[i + 1] = easel_language_name (i);
        codes[i + 1] = easel_language_code (i);
        if (strcmp (s->language, codes[i + 1]) == 0)
          p->language_index = i + 1;
      }
    add_choice (p, language, _ ("Language"), labels, codes, &p->language_index, NEXT_START, "interface", "language", sync_language);
    p->language_key = p->keys->pdata[p->keys->len - 1];
  }
  add_switch (p, panels, _ ("Tool Box"), &s->show_toolbox, "on", "off", "view", "toolbox", apply_panels);
  add_switch (p, panels, _ ("Color Box"), &s->show_colorbox, "on", "off", "view", "colorbox", apply_panels);
  add_switch (p, panels, _ ("Status Bar"), &s->show_statusbar, "on", "off", "view", "statusbar", apply_panels);
  add_switch (p, panels, _ ("Frame time"), &s->frame_time, "on", "off", "view", "frame-time", apply_frame_time);
}

/* ---- Drawing ---- */

static void
on_font_changed (AdwComboRow *row, GParamSpec *pspec, Key *k)
{
  Prefs *p = g_object_get_data (G_OBJECT (row), "easel-prefs");
  guint at = adw_combo_row_get_selected (row);
  const char *family;

  if (p->updating || at == GTK_INVALID_LIST_POSITION)
    return;
  family = at == 0 ? "auto" : gtk_string_list_get_string (GTK_STRING_LIST (adw_combo_row_get_model (row)), at);
  g_strlcpy (p->draft.text_font, at == 0 ? "" : family, sizeof p->draft.text_font);
  key_set (p, k, family);
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
  g_autoptr (GtkStringList) model = gtk_string_list_new ((const char *const[]) { _ ("Automatic"), NULL });
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
      if (g_ascii_strcasecmp (names[i], p->draft.text_font) == 0)
        selected = (guint) i + 1;
    }
  g_free (families);
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Font"));
  adw_combo_row_set_model (row, G_LIST_MODEL (model));
  adw_combo_row_set_expression (row, gtk_property_expression_new (GTK_TYPE_STRING_OBJECT, NULL, "string"));
  adw_combo_row_set_enable_search (row, TRUE);
  adw_combo_row_set_selected (row, selected);
  g_object_set_data (G_OBJECT (row), "easel-prefs", p);
  g_signal_connect (row, "notify::selected", G_CALLBACK (on_font_changed),
                    key_new (p, "text", "font", selected == 0 ? "auto" : gtk_string_list_get_string (model, selected)));
  adw_preferences_group_add (group, GTK_WIDGET (row));
}

static void
sync_eraser_mode (Prefs *p)
{
  p->draft.eraser_transparent = p->eraser_mode == 1;
}

static void
sync_line_width (Prefs *p)
{
  p->draft.line_width = p->line_width_index + 1;
}

static const int magnifier_levels[] = { 1, 2, 4, 6, 8, 16 };

static void
sync_magnifier (Prefs *p)
{
  p->draft.magnifier = magnifier_levels[p->magnifier_index];
}

static void
build_drawing (Prefs *p, AdwPreferencesPage *page)
{
  static const char *const brush_labels[] = { N_ ("Circle, 7 pixels"), N_ ("Circle, 4 pixels"), N_ ("Circle, 1 pixel"), N_ ("Square, 8 pixels"), N_ ("Square, 5 pixels"), N_ ("Square, 2 pixels"),
                                              N_ ("Slash, 8 pixels"), N_ ("Slash, 5 pixels"), N_ ("Slash, 2 pixels"), N_ ("Backslash, 8 pixels"), N_ ("Backslash, 5 pixels"), N_ ("Backslash, 2 pixels"), NULL };
  static const char *brush_names[13];
  static const char *const air_labels[] = { N_ ("9 pixels"), N_ ("16 pixels"), N_ ("24 pixels"), NULL };
  static const char *const air_names[] = { "9", "16", "24" };
  static const char *const eraser_labels[] = { N_ ("4 pixels"), N_ ("6 pixels"), N_ ("8 pixels"), N_ ("10 pixels"), NULL };
  static const char *const eraser_names[] = { "4", "6", "8", "10" };
  static const char *const mode_labels[] = { N_ ("Opaque"), N_ ("Transparent"), NULL };
  static const char *const mode_names[] = { "opaque", "transparent" };
  static const char *const width_labels[] = { N_ ("1 pixel"), N_ ("2 pixels"), N_ ("3 pixels"), N_ ("4 pixels"), N_ ("5 pixels"), NULL };
  static const char *const width_names[] = { "1", "2", "3", "4", "5" };
  static const char *const fill_labels[] = { N_ ("Outline"), N_ ("Outline and fill"), N_ ("Fill"), NULL };
  static const char *const fill_names[] = { "outline", "outline-and-fill", "fill" };
  static const char *const level_labels[] = { "1x", "2x", "4x", "6x", "8x", "16x", NULL };
  static const char *const level_names[] = { "1", "2", "4", "6", "8", "16" };
  EaselSettings *s = &p->draft;
  AdwPreferencesGroup *tools = add_group (page, _ ("Tools"), NULL);
  AdwPreferencesGroup *text = add_group (page, _ ("Text"), NEXT_START);
  AdwPreferencesGroup *history = add_group (page, _ ("History"), NULL);
  AdwPreferencesGroup *saving = add_group (page, _ ("Saving"), NULL);

  for (int i = 0; i < 12; i++)
    brush_names[i] = easel_settings_brush_name (i);
  p->eraser_mode = s->eraser_transparent ? 1 : 0;
  p->line_width_index = s->line_width - 1;
  for (guint i = 0; i < G_N_ELEMENTS (magnifier_levels); i++)
    if (magnifier_levels[i] == s->magnifier)
      p->magnifier_index = (int) i;
  add_choice (p, tools, _ ("Brush"), brush_labels, brush_names, &s->brush, NEXT_START, "drawing", "brush", NULL);
  add_choice (p, tools, _ ("Airbrush"), air_labels, air_names, &s->airbrush, NEXT_START, "drawing", "airbrush", NULL);
  add_choice (p, tools, _ ("Eraser size"), eraser_labels, eraser_names, &s->eraser, NEXT_START, "drawing", "eraser", NULL);
  add_choice (p, tools, _ ("Eraser mode"), mode_labels, mode_names, &p->eraser_mode, NEXT_START, "drawing", "eraser-mode", sync_eraser_mode);
  add_choice (p, tools, _ ("Line and shape width"), width_labels, width_names, &p->line_width_index, NEXT_START, "drawing", "line-width", sync_line_width);
  add_choice (p, tools, _ ("Fill style"), fill_labels, fill_names, (int *) &s->fill_style, NEXT_START, "drawing", "fill-style", NULL);
  /* a switch where the group's other rows are choices; like them, the start-up choice (REQ-0039-03) */
  adw_action_row_set_subtitle (ADW_ACTION_ROW (add_switch (p, tools, _ ("Smooth shapes and brushes"), &s->smooth, "true", "false", "drawing",
                                                           "smooth", NULL)),
                               NEXT_START);
  add_choice (p, tools, _ ("Magnifier level"), level_labels, level_names, &p->magnifier_index, NEXT_START, "drawing", "magnifier", sync_magnifier);
  add_font (p, text);
  add_number (p, text, _ ("Size"), EASEL_TEXT_SIZE_MIN, EASEL_TEXT_SIZE_MAX, &s->text_size, "pt", NULL, "text", "size", NULL);
  add_switch (p, text, _ ("Bold"), &s->text_bold, "true", "false", "text", "bold", NULL);
  add_switch (p, text, _ ("Italic"), &s->text_italic, "true", "false", "text", "italic", NULL);
  add_switch (p, text, _ ("Underline"), &s->text_underline, "true", "false", "text", "underline", NULL);
  add_switch (p, text, _ ("Strikethrough"), &s->text_strike, "true", "false", "text", "strikethrough", NULL);
  add_switch (p, text, _ ("Smooth text"), &s->text_smooth, "true", "false", "text", "smooth", NULL);   /* the start-up choice (REQ-0040-03) */
  add_number (p, history, _ ("Undo memory"), EASEL_UNDO_MB_MIN, EASEL_UNDO_MB_MAX, &s->undo_budget_mb, "MB", NEXT_START, "history", "memory", NULL);
  add_number (p, saving, _ ("JPEG quality"), EASEL_JPEG_QUALITY_MIN, EASEL_JPEG_QUALITY_MAX, &s->jpeg_quality, NULL, NULL, "save", "jpeg-quality", NULL);
}

/* ---- the dialog ---- */

static void
commit_field (gpointer row, gpointer data)
{
  adw_spin_row_update (ADW_SPIN_ROW (row));
}

/* What the panels show and the palette is, as the state to go back to. */
static void
take_state (Prefs *p)
{
  p->state = p->draft;
  p->shown[0] = panel_is_shown (p, "show-toolbox");
  p->shown[1] = panel_is_shown (p, "show-colorbox");
  p->shown[2] = panel_is_shown (p, "show-statusbar");
  p->palette = *easel_window_get_palette (p->win);
}

/* OK and Apply: the one place a change comes to count. FALSE if the settings file could not be written; the values count all the same,
   and in_file says what is still owing (REQ-0027-07, REQ-0027-12). */
static gboolean
keep (Prefs *p)
{
  g_autofree EaselSettingChange *changes = NULL;
  gsize n = 0;
  gboolean written = TRUE;

  g_slist_foreach (p->fields, commit_field, NULL);
  changes = g_new (EaselSettingChange, p->keys->len);
  for (guint i = 0; i < p->keys->len; i++)
    {
      Key *k = p->keys->pdata[i];

      if (g_strcmp0 (k->now, k->in_file) != 0)
        changes[n++] = (EaselSettingChange) { k->section, k->key, k->now };
    }
  if (n > 0)
    written = easel_window_write_settings (p->win, changes, n);
  for (guint i = 0; i < p->keys->len; i++)
    {
      Key *k = p->keys->pdata[i];

      g_free (k->kept);
      k->kept = g_strdup (k->now);
      if (written)
        {
          g_free (k->in_file);
          k->in_file = g_strdup (k->now);
        }
    }
  /* The two files are apart: only the settings write can fail aloud. */
  if (memcmp (easel_window_get_palette (p->win), &p->palette, sizeof p->palette) != 0)
    easel_window_remove_palette_file (p->win);
  *p->chosen = p->draft;
  easel_window_apply_chosen (p->win);
  take_state (p);
  update_apply (p);
  return written;
}

/* Every close that is not OK: Cancel, Esc, the close button, a quit (REQ-0027-10, REQ-0027-11). */
static void
go_back (Prefs *p)
{
  if (p->draft.style != p->state.style)
    easel_window_apply_style (p->win, p->state.style);
  if (p->draft.tint != p->state.tint)
    easel_window_apply_tint (p->win, p->state.tint);
  if (p->draft.frame_time != p->state.frame_time)
    easel_window_apply_frame_time (p->win, p->state.frame_time);
  easel_window_preview_workspace (p->win, &p->state);
  easel_window_apply_panels (p->win, p->shown[0], p->shown[1], p->shown[2]);
  easel_window_set_palette (p->win, &p->palette);
}

static void
on_closed (AdwDialog *dialog, Prefs *p)
{
  p->updating = TRUE;
  go_back (p);                       /* after OK the state is what shows, and this changes nothing */
  easel_window_preferences_closed (p->win);
}

/* A language other than the one running, about to be kept (REQ-0027-16). */
static gboolean
language_changes (Prefs *p)
{
  return g_strcmp0 (p->language_key->now, p->language_key->in_file) != 0
         && strcmp (easel_i18n_choose (p->draft.language), easel_i18n_language ()) != 0;
}

static void
on_ok (GtkButton *button, Prefs *p)
{
  EaselWindow *win = p->win;
  AdwDialog *dialog = ADW_DIALOG (gtk_widget_get_ancestor (GTK_WIDGET (button), ADW_TYPE_DIALOG));
  gboolean changes = language_changes (p), written = keep (p);

  adw_dialog_force_close (dialog);
  if (!written)
    easel_window_show_settings_error (win);
  else if (changes)
    easel_window_show_language_notice (win);
}

static void
on_cancel (GtkButton *button, Prefs *p)
{
  adw_dialog_force_close (ADW_DIALOG (gtk_widget_get_ancestor (GTK_WIDGET (button), ADW_TYPE_DIALOG)));
}

static void
on_apply (GtkButton *button, Prefs *p)
{
  gboolean changes = language_changes (p);

  if (!keep (p))
    easel_window_show_settings_error (p->win);
  else if (changes)
    easel_window_show_language_notice (p->win);
}

static void
free_prefs (gpointer data, GObject *where_the_dialog_was)
{
  Prefs *p = data;

  g_slist_free (p->fields);
  g_ptr_array_unref (p->keys);
  g_free (p);
}

static AdwPreferencesPage *
add_page (GtkWidget *stack, const char *title, const char *name, const char *icon)
{
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());

  adw_preferences_page_set_title (page, title);
  adw_preferences_page_set_name (page, name);
  adw_preferences_page_set_icon_name (page, icon);
  adw_view_stack_add_titled_with_icon (ADW_VIEW_STACK (stack), GTK_WIDGET (page), name, title, icon);
  return page;
}

static GtkWidget *
add_button (Prefs *p, GtkWidget *row, const char *label, GCallback pressed)
{
  GtkWidget *button = gtk_button_new_with_mnemonic (label);

  g_signal_connect (button, "clicked", pressed, p);
  gtk_box_append (GTK_BOX (row), button);
  return button;
}

/* Our own dialog, as a preferences dialog has no place for a row that stays (the Plan's probe). */
void
easel_preferences_dialog_present (EaselWindow *win)
{
  AdwDialog *dialog = adw_dialog_new ();
  GtkWidget *view = adw_toolbar_view_new (), *header = adw_header_bar_new (), *stack = adw_view_stack_new ();
  GtkWidget *switcher = adw_view_switcher_new ();
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  Prefs *p = g_new0 (Prefs, 1);

  p->win = win;
  p->chosen = easel_window_get_chosen_settings (win);
  p->draft = *p->chosen;
  p->keys = g_ptr_array_new_with_free_func (key_free);
  take_state (p);
  adw_dialog_set_title (dialog, _ ("Preferences"));
  adw_dialog_set_content_width (dialog, 640);
  adw_dialog_set_content_height (dialog, 576);
  build_appearance (p, add_page (stack, _ ("Appearance"), "appearance", "applications-graphics-symbolic"));
  build_interface (p, add_page (stack, _ ("Interface"), "interface", "preferences-desktop-symbolic"));
  build_new_picture (p, add_page (stack, _ ("New Picture"), "new-picture", "document-new-symbolic"));
  build_drawing (p, add_page (stack, _ ("Drawing"), "drawing", "document-edit-symbolic"));
  adw_view_switcher_set_stack (ADW_VIEW_SWITCHER (switcher), ADW_VIEW_STACK (stack));
  adw_view_switcher_set_policy (ADW_VIEW_SWITCHER (switcher), ADW_VIEW_SWITCHER_POLICY_NARROW);    /* the icon over the title, which gets the width */
  /* A bar of its own: in the header the four titles are cut in the longer languages (REQ-0027-15). */
  gtk_widget_set_margin_start (switcher, 6);
  gtk_widget_set_margin_end (switcher, 6);
  gtk_widget_set_hexpand (switcher, TRUE);
  gtk_widget_set_halign (switcher, GTK_ALIGN_FILL);
  gtk_widget_set_halign (row, GTK_ALIGN_END);
  gtk_widget_set_margin_top (row, 8);
  gtk_widget_set_margin_bottom (row, 8);
  gtk_widget_set_margin_end (row, 12);
  gtk_widget_add_css_class (add_button (p, row, _ ("_OK"), G_CALLBACK (on_ok)), "suggested-action");
  add_button (p, row, _ ("_Cancel"), G_CALLBACK (on_cancel));
  p->apply = add_button (p, row, _ ("_Apply"), G_CALLBACK (on_apply));
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (view), header);
  adw_toolbar_view_add_top_bar (ADW_TOOLBAR_VIEW (view), switcher);
  adw_toolbar_view_set_content (ADW_TOOLBAR_VIEW (view), stack);
  adw_toolbar_view_add_bottom_bar (ADW_TOOLBAR_VIEW (view), row);
  adw_toolbar_view_set_bottom_bar_style (ADW_TOOLBAR_VIEW (view), ADW_TOOLBAR_RAISED_BORDER);
  adw_dialog_set_child (dialog, view);
  update_apply (p);
  g_signal_connect (dialog, "closed", G_CALLBACK (on_closed), p);
  g_object_weak_ref (G_OBJECT (dialog), free_prefs, p);
  adw_dialog_present (dialog, GTK_WIDGET (win));
  easel_window_preferences_opened (win, dialog);
}
