/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "font-card.h"

#include "text-render.h"

static const struct
{
  guint8 flag;
  EaselTextField field;
  const char *icon, *tip;
} toggles[] = {
  { EASEL_TEXT_BOLD, EASEL_TEXT_FLAG_BOLD, "format-text-bold-symbolic", "Bold (Ctrl+B)" },
  { EASEL_TEXT_ITALIC, EASEL_TEXT_FLAG_ITALIC, "format-text-italic-symbolic", "Italic (Ctrl+I)" },
  { EASEL_TEXT_UNDERLINE, EASEL_TEXT_FLAG_UNDERLINE, "format-text-underline-symbolic", "Underline (Ctrl+U)" },
  { EASEL_TEXT_STRIKE, EASEL_TEXT_FLAG_STRIKE, "format-text-strikethrough-symbolic", "Strikethrough (Ctrl+Shift+5)" },
};

struct _EaselFontCard
{
  GtkBox parent_instance;
  EaselCanvas *canvas;
  GtkStringList *families;         /* every installed family, sorted */
  GtkMenuButton *family;           /* "Font family": shows the family, empty when the selection's differ; opens the popover */
  GtkLabel *family_label;
  GtkPopover *popover;             /* the search field over the list, built at the first opening (R5) */
  GtkSearchEntry *search_entry;
  GtkListView *list;
  guint scroll_pending;            /* a row to scroll to once the list has its size, else GTK_INVALID_LIST_POSITION */
  guint scroll_tick;               /* the tick callback that makes it */
  GtkStringFilter *search;         /* the list's filter: anywhere in the name, case ignored */
  GtkFilterListModel *listed;
  GtkSingleSelection *highlight;   /* the highlighted row, none when the selection's families differ */
  GtkSpinButton *size;
  GtkToggleButton *toggles[G_N_ELEMENTS (toggles)];
  GHashTable *own_font;            /* family -> whether it can show its own name (absent: not asked yet) */
  gboolean showing;                /* the fields are being set from the box's style: nothing is applied */
  int shown_size;                  /* the size shown, 0 when the selection's sizes differ */
  gboolean size_blank;             /* the size field is empty (sizes that differ, or emptied): its value is not shown or applied; a
                                      press elsewhere that takes the focus makes the spin read its field, so a typed size is applied */
  gboolean in_output;              /* the spin writing its own text */
  double step_base;                /* while the field is empty, the spin's value before a step: a step is its change (REQ-0017-25) */
  gboolean field_step;             /* a step by a key or the wheel in the field: the focus stays there for the next one */
  EaselTextStyle shown;
  unsigned shown_mixed;
};

G_DEFINE_FINAL_TYPE (EaselFontCard, easel_font_card, GTK_TYPE_BOX)

/* The focus leaves the card: the window gives the keys to the box wherever else the focus is (REQ-0017-06, REQ-0017-14). */
static void
back_to_box (EaselFontCard *self)
{
  GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (self));
  GtkWidget *focus = root != NULL ? gtk_root_get_focus (root) : NULL;

  if (focus != NULL && gtk_widget_is_ancestor (focus, GTK_WIDGET (self)))
    gtk_root_set_focus (root, NULL);
}

static void
apply (EaselFontCard *self, EaselTextField field, const EaselTextStyle *value)
{
  if (self->canvas != NULL)
    easel_canvas_text_apply (self->canvas, field, value);
}

/* The size field's value applied, unless it is what the box has (so that the spin's own update and an Enter apply it once); an empty
   field changes nothing (REQ-0017-14). */
static void
apply_size (EaselFontCard *self)
{
  EaselTextStyle v = { 0 };

  if (self->showing || self->size_blank || gtk_editable_get_text (GTK_EDITABLE (self->size))[0] == '\0')
    return;
  v.size_pt = gtk_spin_button_get_value_as_int (self->size);
  if (v.size_pt == self->shown_size)
    return;
  self->shown_size = v.size_pt;
  apply (self, EASEL_TEXT_SIZE, &v);
}

/* A new size: the field read, or a step (− or +, Up, Down, Page Up, Page Down, the wheel). While the field is empty (the selection's
   sizes differ) a step is the spin's change from its value before, and steps each part of the selection from its own size, as Ctrl+]
   and Ctrl+[ do (REQ-0017-25); otherwise the value is applied. A key or the wheel in the field leaves the focus there for the next;
   any other step gives the keys back to the box (JOD's M1, 2026-10-27). */
static void
on_size_changed (EaselFontCard *self)
{
  double now = gtk_spin_button_get_value (self->size);

  if (self->showing)
    return;
  if (self->size_blank)
    {
      int delta = (int) (now - self->step_base);

      self->step_base = now;
      if (delta != 0 && self->canvas != NULL)
        easel_canvas_text_step_size (self->canvas, delta);
    }
  else
    apply_size (self);
  if (!self->field_step)
    back_to_box (self);
}

/* Up, Down, Page Up and Page Down in the field (the spin's key bindings): steps in the field */
static void
on_size_key_step (EaselFontCard *self)
{
  self->field_step = TRUE;
}

static void
on_size_key_stepped (EaselFontCard *self)
{
  self->field_step = FALSE;
}

/* The wheel over the field with the focus in it: one step a notch, the focus staying (REQ-0017-25); without the focus it is not the
   field's (the window gives it to the view). */
static gboolean
on_size_wheel (GtkEventControllerScroll *controller, double dx, double dy, EaselFontCard *self)
{
  GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (self));
  GtkWidget *focus = root != NULL ? gtk_root_get_focus (root) : NULL;

  if (focus == NULL || !(focus == GTK_WIDGET (self->size) || gtk_widget_is_ancestor (focus, GTK_WIDGET (self->size))) || dy == 0)
    return GDK_EVENT_PROPAGATE;
  self->field_step = TRUE;
  gtk_spin_button_spin (self->size, dy < 0 ? GTK_SPIN_STEP_FORWARD : GTK_SPIN_STEP_BACKWARD, 1);
  self->field_step = FALSE;
  return GDK_EVENT_STOP;
}

/* While the field is blank the spin does not write its value into it. */
static gboolean
on_size_output (GtkSpinButton *spin, EaselFontCard *self)
{
  if (!self->size_blank)
    return FALSE;
  self->in_output = TRUE;
  gtk_editable_set_text (GTK_EDITABLE (spin), "");
  self->in_output = FALSE;
  return TRUE;
}

/* While the field is blank the spin reads it as the value it has (GTK would read 0, so 1): an update before a step, or at a press
   elsewhere, changes nothing. */
static int
on_size_input (GtkSpinButton *spin, double *value, EaselFontCard *self)
{
  if (!self->size_blank)
    return FALSE;
  *value = gtk_spin_button_get_value (spin);
  return TRUE;
}

/* What the user types decides whether the field is blank. */
static void
on_size_text_changed (EaselFontCard *self)
{
  if (!self->showing && !self->in_output)
    {
      self->size_blank = gtk_editable_get_text (GTK_EDITABLE (self->size))[0] == '\0';
      self->step_base = gtk_spin_button_get_value (self->size);
    }
}

/* Enter: the typed number clamped to 1 to 500 (REQ-0017-25) and applied, the focus back in the box. */
static void
on_size_activate (EaselFontCard *self)
{
  if (gtk_editable_get_text (GTK_EDITABLE (self->size))[0] != '\0')
    gtk_spin_button_update (self->size);
  apply_size (self);
  back_to_box (self);
}

/* A pick (Enter, a press on a row, choose_family): the family applied to the selected text or the text typed next, always, also a
   family part of the selection already has; the popover closes and the keys go to the box again (REQ-0017-14). */
static void
pick_family (EaselFontCard *self, const char *family)
{
  EaselTextStyle v = { 0 };

  v.family = family;
  apply (self, EASEL_TEXT_FAMILY, &v);
  if (self->popover != NULL)
    gtk_popover_popdown (self->popover);           /* its closing gives the keys back to the box */
}

static void
on_row_activated (GtkListView *list, guint position, EaselFontCard *self)
{
  g_autoptr (GtkStringObject) item = g_list_model_get_item (G_LIST_MODEL (self->listed), position);

  if (item != NULL)
    pick_family (self, gtk_string_object_get_string (item));
}

/* After a search the first match is highlighted, and shown */
static void schedule_scroll (EaselFontCard *self);

static void
highlight (EaselFontCard *self, guint position)
{
  gtk_single_selection_set_selected (self->highlight, position);
  if (position == GTK_INVALID_LIST_POSITION || self->list == NULL)
    return;
  /* A list view asked to scroll before it has its size shows no row (JOD's M1, 2026-10-26: the popover opened empty on a family far
     down the list): the scroll waits for the size then */
  if (gtk_widget_get_mapped (GTK_WIDGET (self->list)) && gtk_widget_get_height (GTK_WIDGET (self->list)) > 0)
    gtk_list_view_scroll_to (self->list, position, GTK_LIST_SCROLL_NONE, NULL);
  else
    {
      self->scroll_pending = position;
      schedule_scroll (self);
    }
}

/* The frame after the list is shown: a scroll that waited is made, before that frame's layout */
static gboolean
scroll_when_shown (GtkWidget *list, GdkFrameClock *clock, gpointer data)
{
  EaselFontCard *self = data;
  guint at = self->scroll_pending;

  if (gtk_widget_get_height (list) == 0)
    return G_SOURCE_CONTINUE;                       /* not laid out yet: the next frame */
  self->scroll_tick = 0;
  self->scroll_pending = GTK_INVALID_LIST_POSITION;
  if (at != GTK_INVALID_LIST_POSITION && at < g_list_model_get_n_items (G_LIST_MODEL (self->listed)))
    gtk_list_view_scroll_to (self->list, at, GTK_LIST_SCROLL_NONE, NULL);
  return G_SOURCE_REMOVE;
}

/* While the list is shown, at its next frame; else when it is shown */
static void
schedule_scroll (EaselFontCard *self)
{
  if (self->scroll_tick == 0 && self->scroll_pending != GTK_INVALID_LIST_POSITION && gtk_widget_get_mapped (GTK_WIDGET (self->list)))
    self->scroll_tick = gtk_widget_add_tick_callback (GTK_WIDGET (self->list), scroll_when_shown, self, NULL);
}

static void
on_list_mapped (EaselFontCard *self)
{
  schedule_scroll (self);
}

static void
on_list_unmapped (EaselFontCard *self)
{
  if (self->scroll_tick != 0)
    gtk_widget_remove_tick_callback (GTK_WIDGET (self->list), self->scroll_tick);
  self->scroll_tick = 0;
}

static void
search (EaselFontCard *self, const char *text)
{
  gtk_string_filter_set_search (self->search, text);
  highlight (self, g_list_model_get_n_items (G_LIST_MODEL (self->listed)) > 0 ? 0 : GTK_INVALID_LIST_POSITION);
}

/* The search field reports a change a moment late, also the clearing at each opening: a change that leaves the search as it is keeps
   the highlight on the current family (JOD's M1, 2026-10-26: the first family was highlighted instead) */
static void
on_search_changed (EaselFontCard *self)
{
  const char *text = gtk_editable_get_text (GTK_EDITABLE (self->search_entry)), *now = gtk_string_filter_get_search (self->search);

  if (g_strcmp0 (text[0] != '\0' ? text : NULL, now != NULL && now[0] != '\0' ? now : NULL) != 0)
    search (self, text);
}

gboolean
easel_font_card_family_key (EaselFontCard *self, guint keyval)
{
  guint at = gtk_single_selection_get_selected (self->highlight), n = g_list_model_get_n_items (G_LIST_MODEL (self->listed));

  switch (keyval)
    {
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      highlight (self, at == GTK_INVALID_LIST_POSITION ? (n > 0 ? 0 : at) : MIN (at + 1, n - 1));
      return TRUE;
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      highlight (self, at == GTK_INVALID_LIST_POSITION ? (n > 0 ? 0 : at) : (at > 0 ? at - 1 : 0));
      return TRUE;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      if (at != GTK_INVALID_LIST_POSITION)
        on_row_activated (self->list, at, self);
      return TRUE;
    case GDK_KEY_Escape:
      easel_font_card_cancel_family (self);              /* only the popover closes (REQ-0017-14, REQ-0017-17) */
      return TRUE;
    default:
      return FALSE;
    }
}

static gboolean
on_search_key (EaselFontCard *self, guint keyval)
{
  return easel_font_card_family_key (self, keyval) ? GDK_EVENT_STOP : GDK_EVENT_PROPAGATE;
}

/* However it closes (a pick, Esc, a press elsewhere), the focus leaves its search field; the family button cannot take it, and where GTK
   puts it later the window's focus watch takes it off the card (REQ-0017-14) */
static void
on_popover_closed (EaselFontCard *self)
{
  back_to_box (self);
}

static void
on_toggled (GtkToggleButton *button, EaselFontCard *self)
{
  EaselTextStyle v = { 0 };

  if (self->showing)
    return;
  for (guint i = 0; i < G_N_ELEMENTS (toggles); i++)
    if (self->toggles[i] == button)
      {
        v.flags = gtk_toggle_button_get_active (button) ? toggles[i].flag : 0;
        apply (self, toggles[i].field, &v);
      }
}

static gboolean
on_key_pressed (EaselFontCard *self, guint keyval)
{
  if (keyval != GDK_KEY_Escape)
    return GDK_EVENT_PROPAGATE;
  easel_font_card_cancel_field (self);
  return GDK_EVENT_STOP;
}

const char *
easel_font_card_row_font (EaselFontCard *self, const char *family)
{
  gpointer known;

  if (!g_hash_table_lookup_extended (self->own_font, family, NULL, &known))
    {
      /* Loaded when a row is first bound, not for the whole list (R5): a font that has every non-space character of its name */
      PangoFontDescription *fd = pango_font_description_new ();
      PangoContext *ctx = gtk_widget_get_pango_context (GTK_WIDGET (self));
      PangoFont *font;
      gboolean can = TRUE;

      pango_font_description_set_family (fd, family);
      pango_font_description_set_size (fd, 12 * PANGO_SCALE);
      font = pango_context_load_font (ctx, fd);
      for (const char *p = family; can && *p != '\0'; p = g_utf8_next_char (p))
        can = g_unichar_isspace (g_utf8_get_char (p)) || (font != NULL && pango_font_has_char (font, g_utf8_get_char (p)));
      g_clear_object (&font);
      pango_font_description_free (fd);
      known = GINT_TO_POINTER (can);
      g_hash_table_insert (self->own_font, g_strdup (family), known);
    }
  return GPOINTER_TO_INT (known) ? family : NULL;
}

static void
setup_row (GtkSignalListItemFactory *factory, GtkListItem *item)
{
  GtkWidget *label = gtk_label_new (NULL);

  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_list_item_set_child (item, label);
}

/* A row of the list: the family's name in its own font (REQ-0017-14) */
static void
bind_row (GtkSignalListItemFactory *factory, GtkListItem *item, EaselFontCard *self)
{
  GtkLabel *label = GTK_LABEL (gtk_list_item_get_child (item));
  const char *name = gtk_string_object_get_string (gtk_list_item_get_item (item));
  const char *font = easel_font_card_row_font (self, name);
  PangoAttrList *attrs = pango_attr_list_new ();

  gtk_label_set_text (label, name);
  if (font != NULL)
    pango_attr_list_insert (attrs, pango_attr_family_new (font));
  gtk_label_set_attributes (label, attrs);
  pango_attr_list_unref (attrs);
}

/* The popover: a search field over the list of the installed families, made at the first opening (R5: the rows' fonts are loaded as
   they are bound). */
static void
build_popover (EaselFontCard *self)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6), *scroller = gtk_scrolled_window_new ();
  GtkListItemFactory *rows = gtk_signal_list_item_factory_new ();
  GtkEventController *keys = gtk_event_controller_key_new ();

  self->search_entry = GTK_SEARCH_ENTRY (gtk_search_entry_new ());
  gtk_search_entry_set_search_delay (self->search_entry, 0);
  g_signal_connect_swapped (self->search_entry, "search-changed", G_CALLBACK (on_search_changed), self);
  g_signal_connect_swapped (keys, "key-pressed", G_CALLBACK (on_search_key), self);
  gtk_event_controller_set_propagation_phase (keys, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (GTK_WIDGET (self->search_entry), keys);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->search_entry));
  g_signal_connect (rows, "setup", G_CALLBACK (setup_row), NULL);
  g_signal_connect (rows, "bind", G_CALLBACK (bind_row), self);
  self->list = GTK_LIST_VIEW (gtk_list_view_new (GTK_SELECTION_MODEL (g_object_ref (self->highlight)), rows));
  gtk_list_view_set_single_click_activate (self->list, TRUE);
  g_signal_connect (self->list, "activate", G_CALLBACK (on_row_activated), self);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroller), GTK_WIDGET (self->list));
  self->scroll_pending = GTK_INVALID_LIST_POSITION;
  g_signal_connect_swapped (self->list, "map", G_CALLBACK (on_list_mapped), self);
  g_signal_connect_swapped (self->list, "unmap", G_CALLBACK (on_list_unmapped), self);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scroller), 300);
  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (scroller), 240);
  gtk_box_append (GTK_BOX (box), scroller);
  /* typing anywhere in the popover goes to the search (REQ-0017-14) */
  gtk_search_entry_set_key_capture_widget (self->search_entry, box);
  self->popover = GTK_POPOVER (gtk_popover_new ());
  gtk_popover_set_child (self->popover, box);
  g_signal_connect_swapped (self->popover, "closed", G_CALLBACK (on_popover_closed), self);
  gtk_menu_button_set_popover (self->family, GTK_WIDGET (self->popover));
}

/* GTK asks for the popup once, at the first opening: it is built then */
static void
create_popup (GtkMenuButton *button, gpointer data)
{
  build_popover (data);
}

/* Each opening: the search empty, the current family scrolled to and highlighted (none when the selection's differ), the typing in
   the search (REQ-0017-14). */
static void
on_family_opening (EaselFontCard *self)
{
  guint at = GTK_INVALID_LIST_POSITION, n;

  if (self->popover == NULL)
    build_popover (self);
  gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
  gtk_string_filter_set_search (self->search, NULL);
  n = g_list_model_get_n_items (G_LIST_MODEL (self->listed));
  for (guint i = 0; !(self->shown_mixed & EASEL_TEXT_FAMILY) && self->shown.family != NULL && i < n; i++)
    if (g_strcmp0 (gtk_string_list_get_string (self->families, i), self->shown.family) == 0)
      at = i;
  highlight (self, at);
  gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
}

static void
on_family_active (EaselFontCard *self)
{
  if (gtk_menu_button_get_active (self->family))
    on_family_opening (self);
}

static int
compare_names (gconstpointer a, gconstpointer b)
{
  return g_utf8_collate (*(const char *const *) a, *(const char *const *) b);
}

static void
easel_font_card_init (EaselFontCard *self)
{
  PangoFontFamily **families = NULL;
  int n = 0;
  g_autoptr (GPtrArray) names = g_ptr_array_new ();
  GtkExpression *expression = gtk_property_expression_new (GTK_TYPE_STRING_OBJECT, NULL, "string");
  GtkEventController *keys = gtk_event_controller_key_new ();

  self->own_font = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (self), 6);
  gtk_widget_add_css_class (GTK_WIDGET (self), "card");
  gtk_widget_add_css_class (GTK_WIDGET (self), "toolbar");
  gtk_widget_add_css_class (GTK_WIDGET (self), "font-card");                 /* opaque in both themes (style.css) */

  /* The family: a button, "Font family", that opens the popover with the search and the list */
  pango_font_map_list_families (pango_cairo_font_map_get_default (), &families, &n);
  for (int i = 0; i < n; i++)
    if (!easel_text_render_is_generic (pango_font_family_get_name (families[i])))
      g_ptr_array_add (names, (gpointer) pango_font_family_get_name (families[i]));   /* no generic name in the list (REQ-0017-14) */
  g_ptr_array_sort (names, compare_names);
  g_ptr_array_add (names, NULL);
  self->families = gtk_string_list_new ((const char *const *) names->pdata);
  g_free (families);
  self->search = gtk_string_filter_new (expression);
  gtk_string_filter_set_match_mode (self->search, GTK_STRING_FILTER_MATCH_MODE_SUBSTRING);
  gtk_string_filter_set_ignore_case (self->search, TRUE);
  self->listed = gtk_filter_list_model_new (G_LIST_MODEL (g_object_ref (self->families)), GTK_FILTER (g_object_ref (self->search)));
  self->highlight = gtk_single_selection_new (G_LIST_MODEL (g_object_ref (self->listed)));
  gtk_single_selection_set_autoselect (self->highlight, FALSE);
  gtk_single_selection_set_can_unselect (self->highlight, TRUE);
  gtk_single_selection_set_selected (self->highlight, GTK_INVALID_LIST_POSITION);
  self->family = GTK_MENU_BUTTON (gtk_menu_button_new ());
  self->family_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_width_chars (self->family_label, 12);
  gtk_label_set_max_width_chars (self->family_label, 18);
  gtk_label_set_ellipsize (self->family_label, PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (self->family_label, 0);
  gtk_menu_button_set_child (self->family, GTK_WIDGET (self->family_label));
  gtk_menu_button_set_always_show_arrow (self->family, TRUE);
  /* The card's controls take no focus (JOD, 2026-10-27: the mouse, and from the keyboard the box's shortcuts): the button opens the
     popover and the keys stay with the box */
  gtk_widget_set_focusable (GTK_WIDGET (self->family), FALSE);
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->family)); c != NULL; c = gtk_widget_get_next_sibling (c))
    gtk_widget_set_focusable (c, FALSE);
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->family), "Font family (Ctrl+Shift+F)");   /* each control and its key (REQ-0017-14) */
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->family), GTK_ACCESSIBLE_PROPERTY_LABEL, "Font family", -1);
  gtk_menu_button_set_create_popup_func (self->family, create_popup, self, NULL);
  g_signal_connect_swapped (self->family, "notify::active", G_CALLBACK (on_family_active), self);
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->family));

  /* The size: 1 to 500, step 1, digits only, a number outside clamped when applied (REQ-0017-25) */
  self->size = GTK_SPIN_BUTTON (gtk_spin_button_new_with_range (1, 500, 1));
  gtk_spin_button_set_numeric (self->size, TRUE);
  gtk_spin_button_set_update_policy (self->size, GTK_UPDATE_ALWAYS);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self->size), FALSE);    /* − and + take no focus; a click in the text still types there */
  gtk_spin_button_set_value (self->size, 12);
  self->shown_size = 12;
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->size), "Font size (Ctrl+] larger, Ctrl+[ smaller)");
  g_signal_connect (self->size, "output", G_CALLBACK (on_size_output), self);
  g_signal_connect (self->size, "input", G_CALLBACK (on_size_input), self);
  g_signal_connect_swapped (self->size, "change-value", G_CALLBACK (on_size_key_step), self);
  g_signal_connect_data (self->size, "change-value", G_CALLBACK (on_size_key_stepped), self, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  {
    /* one step a notch (GTK sums a touchpad's scroll into notches), before the spin's own wheel */
    GtkEventController *wheel = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL
                                                                 | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);

    gtk_event_controller_set_propagation_phase (wheel, GTK_PHASE_CAPTURE);
    g_signal_connect (wheel, "scroll", G_CALLBACK (on_size_wheel), self);
    gtk_widget_add_controller (GTK_WIDGET (self->size), wheel);
  }
  g_signal_connect_swapped (self->size, "changed", G_CALLBACK (on_size_text_changed), self);
  g_signal_connect_swapped (self->size, "value-changed", G_CALLBACK (on_size_changed), self);
  g_signal_connect_swapped (self->size, "activate", G_CALLBACK (on_size_activate), self);
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->size));

  /* Bold, Italic, Underline, Strikethrough: pressing one leaves the focus in the box */
  for (guint i = 0; i < G_N_ELEMENTS (toggles); i++)
    {
      GtkWidget *button = gtk_toggle_button_new ();

      gtk_button_set_icon_name (GTK_BUTTON (button), toggles[i].icon);
      gtk_widget_set_tooltip_text (button, toggles[i].tip);
      gtk_widget_set_focusable (button, FALSE);
      gtk_widget_set_focus_on_click (button, FALSE);
      gtk_widget_add_css_class (button, "flat");
      g_signal_connect (button, "toggled", G_CALLBACK (on_toggled), self);
      self->toggles[i] = GTK_TOGGLE_BUTTON (button);
      gtk_box_append (GTK_BOX (self), button);
    }

  g_signal_connect_swapped (keys, "key-pressed", G_CALLBACK (on_key_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), keys);
}

static void
easel_font_card_dispose (GObject *object)
{
  EaselFontCard *self = EASEL_FONT_CARD (object);

  g_clear_object (&self->listed);
  g_clear_object (&self->search);
  g_clear_object (&self->highlight);
  g_clear_object (&self->families);
  g_clear_pointer (&self->own_font, g_hash_table_unref);
  self->canvas = NULL;
  G_OBJECT_CLASS (easel_font_card_parent_class)->dispose (object);
}

static void
easel_font_card_class_init (EaselFontCardClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = easel_font_card_dispose;
}

void
easel_font_card_set_canvas (EaselFontCard *self, EaselCanvas *canvas)
{
  self->canvas = canvas;
}

void
easel_font_card_show_style (EaselFontCard *self, const EaselTextStyle *style, unsigned mixed)
{
  self->showing = TRUE;
  self->shown = *style;
  self->shown_mixed = mixed;
  /* the family by its name, also one not installed; empty when the selection's differ (REQ-0017-14) */
  gtk_label_set_text (self->family_label, (mixed & EASEL_TEXT_FAMILY) || style->family == NULL ? ""
                                          : easel_text_render_resolve_family (style->family));   /* a generic name as its family */
  self->shown_size = (mixed & EASEL_TEXT_SIZE) ? 0 : style->size_pt;
  self->size_blank = self->shown_size == 0;
  if (self->shown_size > 0)
    {
      g_autofree char *text = g_strdup_printf ("%d", self->shown_size);

      gtk_spin_button_set_value (self->size, self->shown_size);
      gtk_editable_set_text (GTK_EDITABLE (self->size), text);      /* also after an empty field, the value being the same */
    }
  else
    {
      /* sizes that differ: the spin's value in the middle of its range, so that every step from it is whole (REQ-0017-25) */
      gtk_spin_button_set_value (self->size, 250);
      self->step_base = gtk_spin_button_get_value (self->size);
      gtk_editable_set_text (GTK_EDITABLE (self->size), "");
    }
  for (guint i = 0; i < G_N_ELEMENTS (toggles); i++)
    gtk_toggle_button_set_active (self->toggles[i], (style->flags & toggles[i].flag) && !(mixed & toggles[i].field));
  self->showing = FALSE;
}

void
easel_font_card_cancel_field (EaselFontCard *self)
{
  easel_font_card_show_style (self, &self->shown, self->shown_mixed);
  back_to_box (self);
}

gboolean
easel_font_card_has_search (EaselFontCard *self)
{
  return self->search_entry != NULL;
}

void
easel_font_card_open_family (EaselFontCard *self)
{
  if (self->popover == NULL)
    build_popover (self);
  gtk_menu_button_popup (self->family);
  on_family_opening (self);                  /* also where the session does not show the popover (R5) */
}

gboolean
easel_font_card_family_is_open (EaselFontCard *self)
{
  return self->popover != NULL && gtk_widget_get_visible (GTK_WIDGET (self->popover));
}

void
easel_font_card_set_search (EaselFontCard *self, const char *text)
{
  if (self->search_entry != NULL)
    {
      g_signal_handlers_block_by_func (self->search_entry, on_search_changed, self);
      gtk_editable_set_text (GTK_EDITABLE (self->search_entry), text);
      g_signal_handlers_unblock_by_func (self->search_entry, on_search_changed, self);
    }
  search (self, text);                       /* as the field's search-changed does */
}

GListModel *
easel_font_card_get_listed (EaselFontCard *self)
{
  return G_LIST_MODEL (self->listed);
}

const char *
easel_font_card_get_highlighted (EaselFontCard *self)
{
  GtkStringObject *item = gtk_single_selection_get_selected_item (self->highlight);

  return item != NULL ? gtk_string_object_get_string (item) : NULL;
}

gboolean
easel_font_card_choose_family (EaselFontCard *self, const char *family)
{
  if (easel_text_render_find_family (family) == NULL)
    return FALSE;
  pick_family (self, easel_text_render_find_family (family));
  return TRUE;
}

void
easel_font_card_cancel_family (EaselFontCard *self)
{
  if (self->popover != NULL)
    gtk_popover_popdown (self->popover);
  gtk_string_filter_set_search (self->search, NULL);
  back_to_box (self);
}

GtkWidget *
easel_font_card_get_family_button (EaselFontCard *self)
{
  return GTK_WIDGET (self->family);
}

const char *
easel_font_card_get_family_text (EaselFontCard *self)
{
  return gtk_label_get_text (self->family_label);
}

GtkSpinButton *
easel_font_card_get_size (EaselFontCard *self)
{
  return self->size;
}

GtkToggleButton *
easel_font_card_get_toggle (EaselFontCard *self, guint8 flag)
{
  for (guint i = 0; i < G_N_ELEMENTS (toggles); i++)
    if (toggles[i].flag == flag)
      return self->toggles[i];
  return NULL;
}
