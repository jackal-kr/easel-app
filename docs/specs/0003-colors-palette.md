# 0003 Colors and palette

Status: implemented
Depends on: 0001, 0002

## Why
Drawing in black and one background color is not a paint program. The user needs XP's palette: 28 colors one click
away, left click for the foreground, right click for the background, and a way to change any palette color that the
app remembers.

## Requirements
Look and input rules come from `docs/design.md` (Palette editing; Pointer: primary = foreground, secondary = background).

### Palette bar and indicator
- REQ-0003-01 While the window is open, the app shall show a bar below the canvas, across the window's full width: at its left a color indicator, then a swap button, then 28 palette swatches of 20x20 logical pixels with 2 logical pixels between neighbors, in two rows of 14. Swatches are numbered 1 to 28: row 1 left to right is 1 to 14, row 2 is 15 to 28.
- REQ-0003-02 The indicator shall show two overlapping squares: the foreground square in front at the top left, the background square behind it at the bottom right, each showing its color within the next frame drawn after the color changes.
- REQ-0003-03 The window shall not get narrower than the bar needs, so no swatch is ever clipped.
- REQ-0003-04 The default palette shall be the classic XP Paint colors, all opaque, by swatch number:

| Swatch | Row | Column | Hex | RGB |
|---|---|---|---|---|
| 1 | 1 | 1 | #000000 | 0, 0, 0 |
| 2 | 1 | 2 | #808080 | 128, 128, 128 |
| 3 | 1 | 3 | #800000 | 128, 0, 0 |
| 4 | 1 | 4 | #808000 | 128, 128, 0 |
| 5 | 1 | 5 | #008000 | 0, 128, 0 |
| 6 | 1 | 6 | #008080 | 0, 128, 128 |
| 7 | 1 | 7 | #000080 | 0, 0, 128 |
| 8 | 1 | 8 | #800080 | 128, 0, 128 |
| 9 | 1 | 9 | #808040 | 128, 128, 64 |
| 10 | 1 | 10 | #004040 | 0, 64, 64 |
| 11 | 1 | 11 | #0080FF | 0, 128, 255 |
| 12 | 1 | 12 | #004080 | 0, 64, 128 |
| 13 | 1 | 13 | #8000FF | 128, 0, 255 |
| 14 | 1 | 14 | #804000 | 128, 64, 0 |
| 15 | 2 | 1 | #FFFFFF | 255, 255, 255 |
| 16 | 2 | 2 | #C0C0C0 | 192, 192, 192 |
| 17 | 2 | 3 | #FF0000 | 255, 0, 0 |
| 18 | 2 | 4 | #FFFF00 | 255, 255, 0 |
| 19 | 2 | 5 | #00FF00 | 0, 255, 0 |
| 20 | 2 | 6 | #00FFFF | 0, 255, 255 |
| 21 | 2 | 7 | #0000FF | 0, 0, 255 |
| 22 | 2 | 8 | #FF00FF | 255, 0, 255 |
| 23 | 2 | 9 | #FFFF80 | 255, 255, 128 |
| 24 | 2 | 10 | #00FF80 | 0, 255, 128 |
| 25 | 2 | 11 | #80FFFF | 128, 255, 255 |
| 26 | 2 | 12 | #8080FF | 128, 128, 255 |
| 27 | 2 | 13 | #FF0080 | 255, 0, 128 |
| 28 | 2 | 14 | #FF8040 | 255, 128, 64 |

- REQ-0003-05 A swatch or indicator square whose color has transparency shall show it over a checkerboard, as the canvas does (REQ-0001-11).
- REQ-0003-18 The bar shall have a separator line above it, 8 logical pixels of margin left and right, 6 above and below, and 8 between the indicator, the swap button and the swatch grid. The indicator shall be 46x42 logical pixels: the foreground square 28x28 with its top left at (2, 2), the background square 28x28 with its top left at (16, 12), each with a 1-pixel frame.
- REQ-0003-19 When the user clicks the swap button, which shows the symbol ⇄ and the tooltip "Swap foreground and background (Ctrl+Shift+X)", the app shall swap the foreground and background colors as REQ-0003-07 does; the button does nothing while a dialog, the quit prompt or the color chooser is open.

### Choosing colors
- REQ-0003-06 When the user primary-clicks a palette swatch, the app shall make its color the foreground color and give that swatch keyboard focus; when the user secondary-clicks it, the background color, with the same focus. A middle click does nothing.
- REQ-0003-07 When the user presses Ctrl+Shift+X, the app shall swap the foreground and background colors. The shortcut works wherever keyboard focus is in the window and does nothing while a dialog, the quit prompt or the color chooser is open (as REQ-0002-11).
- REQ-0003-08 A pencil stroke shall use the foreground or background color that is current when it starts, to its end (REQ-0001-19, REQ-0001-21). A color with transparency sets pixels to exactly that value, without blending.
- REQ-0003-09 Choosing, swapping or editing colors shall not change the picture, add an undo step or change the modified mark (adds to REQ-0002-02).
- REQ-0003-10 The foreground and background colors shall not be remembered between runs: every start uses REQ-0001-08 (black, and the picture's background).

### Editing colors
- REQ-0003-11 When the user double-clicks a palette swatch with the primary button (both clicks on the same swatch), or presses F2 while a swatch has keyboard focus, the app shall open the system color chooser, with transparency, set to that swatch's color. The first click of the double-click has already made the color the foreground (REQ-0003-06).
- REQ-0003-12 When the user confirms a color in that chooser, the app shall make it the swatch's color and the foreground color; when the user cancels, the swatch keeps its color and the foreground stays as the first click set it.
- REQ-0003-13 When the user clicks a square of the indicator, the app shall open the chooser set to that color; confirming makes it the new foreground (front square) or background (back square); no swatch changes. Cancel changes nothing.
- REQ-0003-14 The chooser shall be modal: while it is open the main window takes no drawing, swatch, indicator, undo, redo or swap input. If the user quits while it is open, the chooser closes as Cancel and quitting goes on as REQ-0001-34/35.

### Remembering the palette
- REQ-0003-15 When a swatch's color is confirmed (REQ-0003-12), the app shall write the whole palette at once to `~/.config/easel/palette.ini`, a file the app owns (the settings file stays untouched, REQ-0002-19). Section `[palette]`, keys `color1` to `color28` (numbering of REQ-0003-01), values as REQ-0001-03 (`#RRGGBB`, `#RRGGBBAA` or `transparent`). The write replaces the file in one step, so it is never half-written. The file does not exist until the first confirmed edit.
- REQ-0003-16 If writing the palette file fails, then the swatch still changes for this run, silently.
- REQ-0003-17 At start-up the app shall read the palette file with the syntax of REQ-0001-40. A missing file, a missing or invalid entry, or an unparseable file gives the default color (REQ-0003-04) for each affected swatch, silently; unknown keys and sections are ignored. The next confirmed edit rewrites the file whole.

## Scenarios
- AS-1 (REQ-0003-01, REQ-0003-02, REQ-0003-04) Given a fresh start with no palette file, When the window opens, Then a bar below the canvas shows the indicator and swatches 1 to 28 with the colors of REQ-0003-04 in order, and the indicator shows black in front of the picture's background color | Verified by: automated (ui) + manual M1
- AS-2 (REQ-0003-03) Given the window open, When the user makes it as narrow as possible, Then all 28 swatches and the indicator stay fully visible | Verified by: manual M2
- AS-3 (REQ-0003-06) Given a fresh start, When the user primary-clicks swatch 17 (#FF0000), Then the foreground is #FF0000, the front square is red and swatch 17 has keyboard focus; When the user secondary-clicks swatch 21 (#0000FF), Then the background is #0000FF; When the user middle-clicks swatch 1, Then nothing changes | Verified by: automated (ui)
- AS-4 (REQ-0003-07) Given foreground #FF0000 and background #0000FF, When the user presses Ctrl+Shift+X, Then the foreground is #0000FF and the background #FF0000; Given the quit prompt is open, When Ctrl+Shift+X is pressed, Then nothing changes | Verified by: automated (ui)
- AS-5 (REQ-0003-08) Given foreground #FF0000 and background #0000FF, When the user primary-drags and then secondary-drags with the Pencil, Then the first stroke is #FF0000 and the second #0000FF; Given a primary stroke in progress, When the user presses Ctrl+Shift+X, Then the rest of that stroke stays #FF0000 and the next primary stroke is #0000FF | Verified by: automated (ui)
- AS-6 (REQ-0003-05, REQ-0003-08) Given `background=transparent` in the settings, When the window opens, Then the indicator's back square shows a checkerboard; When the user presses Ctrl+Shift+X and draws, Then the stroke's pixels are exactly #00000000 | Verified by: automated (ui) + manual M3
- AS-7 (REQ-0003-09) Given an unmodified picture with no undo steps, When the user picks colors, swaps them and edits a swatch, Then the title has no modified mark and Undo stays insensitive | Verified by: automated (ui)
- AS-8 (REQ-0003-10) Given the user picked #FF0000 as foreground and quit, When the app starts again, Then the foreground is black | Verified by: manual M4
- AS-9 (REQ-0003-11, REQ-0003-12) Given the default palette, When the user double-clicks swatch 17 (#FF0000) and confirms #123456, Then swatch 17 and the foreground are #123456; When the user double-clicks swatch 1 and cancels, Then swatch 1 is still #000000 and the foreground is #000000 (from the first click) | Verified by: automated (ui) + manual M5
- AS-10 (REQ-0003-11) Given swatch 17 clicked (focused), When the user presses F2 and confirms #00FF0080, Then swatch 17 shows half-transparent green over a checkerboard | Verified by: automated (ui) + manual M6
- AS-11 (REQ-0003-11, REQ-0003-06) Given the default palette, When the user clicks swatch 17 and quickly clicks swatch 18, Then no chooser opens and the foreground is #FFFF00; When the user secondary-double-clicks swatch 21, Then no chooser opens and the background is #0000FF | Verified by: automated (ui)
- AS-12 (REQ-0003-13) Given foreground #000000, When the user clicks the indicator's front square and confirms #123456, Then the foreground is #123456 and every swatch keeps its color | Verified by: automated (ui) + manual M7
- AS-13 (REQ-0003-14) Given the chooser open, When the user presses Ctrl+Z, Ctrl+Shift+X or clicks a swatch, Then nothing changes; When the user closes the window, Then the chooser closes as Cancel and, if the picture is modified, the quit prompt appears | Verified by: manual M8
- AS-14 (REQ-0003-15) Given no palette file, When the user confirms #123456 for swatch 3, Then `palette.ini` exists with `[palette]` and `color1` to `color28`, `color3=#123456` and the other values the defaults; the settings file is byte-for-byte unchanged | Verified by: automated (io)
- AS-15 (REQ-0003-16) Given the config folder is read-only, When the user confirms #123456 for swatch 3, Then swatch 3 shows #123456, no message appears, and no file is written | Verified by: automated (io + ui)
- AS-16 (REQ-0003-17) Given `palette.ini` with `color3 = #123456`, `color5=abc`, `color40=#FFFFFF`, `# note` and a `[other]` section, When the app starts, Then swatch 3 is #123456, swatch 5 its default, the rest their defaults, and no message is shown; Given a palette file of binary garbage, Then every swatch has its default color | Verified by: automated (core/io)
- AS-17 (REQ-0003-14, REQ-0002-11) Given a picture with one undoable stroke and the color chooser open, When the user presses Ctrl+Z, then Ctrl+Y, Then the stroke is still there, Undo stays sensitive and Redo stays insensitive | Verified by: automated (ui)
- AS-18 (REQ-0003-01, REQ-0003-18, REQ-0003-19) Given a fresh start, When the window opens, Then the bar shows, from the left, the 46x42 indicator, the swap button and the swatch grid (swatch n at x = 22 * (column - 1), y = 22 * (row - 1) inside the grid, each 20x20); Given foreground #FF0000 and background #0000FF, When the user clicks the swap button, Then the foreground is #0000FF and the background #FF0000; Given the color chooser open, When the swap button is clicked, Then nothing changes | Verified by: automated (ui) + manual M9 (the bar looks right at normal and doubled screen scale)

## Edge cases
- Text the user types (names, values): the palette file's values and syntax, REQ-0003-15 and REQ-0003-17.
- A rule that changes the user's input after they confirmed it: does not apply.
- The same action again while its dialog or prompt is already open: the chooser is modal, so a second one cannot open (REQ-0003-14).
- A third quick click on a swatch is a new single click; it neither opens a second chooser nor changes anything beyond REQ-0003-06.
- Confirming a swatch's current color rewrites the file with the same values.

## Out of scope
Keyboard navigation of the palette (Tab, arrows, Enter, Space) and screen-reader names (Accessibility); pick color from the
picture (0007); the Preferences control that resets the palette (until then, deleting `palette.ini` resets it);
remembering the fore/background colors between runs; an "Edit Colors" menu entry (editing happens in place, see
REQ-0003-11..13); the third (Ctrl) color slot (product non-goal).

## Open questions
(none)

## Behavior reference
- `docs/design.md` Palette editing: double-click, F2 or the indicator opens the system color chooser; remembered between runs; Preferences reset (Preferences); no separate Edit Colors entry (settled here).
- `docs/reference/new-behaviors.md`: Ctrl+Shift+X swaps the colors; palette keyboard access (Accessibility).
- XP Paint: 28 colors in two rows of 14 at the bottom; left click foreground, right click background; overlapping squares at the left of the palette; double-click opens Edit Colors and the chosen color becomes the foreground.

## Plan

### Approach
- **Core:** a new `palette` module holds 28 colors (`uint32_t` straight alpha, ADR-0002), the defaults table, and parsing
  and formatting of the palette text.
  - The settings and palette files share one syntax (REQ-0001-40). So the line handling now in `settings.c` moves to a
    small `ini` module (it walks `[section]` and `key=value` lines and reports an unparseable file), and the color
    parser moves with it.
  - `settings.c` keeps its rules on top. Its behavior is unchanged, and the existing 0001 and 0002 settings tests guard
    the move.
- **io:** `palette-file` reads `~/.config/easel/palette.ini` (a missing file gives the defaults and is not created). It
  writes the whole file with the same atomic write as PNG saves (`g_file_set_contents_full`, consistent mode); a failed
  write returns false and is logged at debug level only.
- **ui:**
  - An `EaselPaletteBar` sits in the window's bottom bar (`AdwToolbarView` bottom). It holds an `EaselColorIndicator`,
    a flat swap button (⇄) and a 2x14 grid of `EaselSwatch` widgets, with the dimensions of REQ-0003-18.
  - Each swatch is its own focusable widget, so focus, the focus ring and later screen-reader names (Accessibility) come from
    GTK. Each has a click gesture for primary, secondary and double click, and an `F2` key binding.
  - The window owns the palette and the two colors and passes them to the canvas.
  - The chooser is GTK's color dialog, modal and with alpha. Because a compositor may not block input to the parent, the
    window also keeps a "chooser open" flag; the canvas, swatches, indicator, undo, redo and swap all check it. Quitting
    while the chooser is open cancels it first.
- **Swap:** `win.swap-colors`, Ctrl+Shift+X, ignored while a dialog or the chooser is open (REQ-0002-11 rule).

Transparency on screen only: the checkerboard behind transparent colors (swatches, indicator, canvas) is drawn by
the widgets and never written into the picture; a transparent pencil color sets the pixel to exactly its value
(REQ-0003-08, tested in core).

Data that grows with use: none. The palette is a fixed 28 colors; the file is rewritten whole, about 600 bytes.

### Files
- **New:**
  - `src/core/ini.{c,h}`, `src/core/palette.{c,h}`, `src/io/palette-file.{c,h}`;
  - `src/ui/palette-bar.{c,h}` (bar, swatch and indicator);
  - `tests/core/test-palette.c`, `tests/io/test-palette-file.c`.
- **Changed:**
  - `src/core/settings.c` (uses `ini`), `src/ui/window.{c,h}`, `src/ui/window.ui`, `src/ui/canvas.{c,h}` (input
    blocked while the chooser is open), `src/ui/application.c` (Ctrl+Shift+X);
  - both `meson.build` files, `tests/ui/test-window.c`.

### Key interfaces
- `bool easel_ini_parse (const char *text, size_t len, EaselIniFunc func, void *data)`: calls `func (section, key,
  value)` for each key line; returns false for an unparseable file (REQ-0001-40).
  `bool easel_ini_parse_color (span, uint32_t *out)`.
- `typedef struct { uint32_t colors[28]; } EaselPalette`; `easel_palette_defaults`;
  `easel_palette_parse (text, len, EaselPalette *)`; `char *easel_palette_format (const EaselPalette *)`. The writer uses
  `#RRGGBB` when a color is opaque and `#RRGGBBAA` otherwise.
- `bool easel_palette_file_load (const char *path, EaselPalette *)`; `bool easel_palette_file_save (path, const EaselPalette *)`.
- Window and test entry points:
  - `easel_window_get_palette_bar`;
  - `easel_palette_bar_press (bar, int swatch, guint button, int n_press)` (the gesture feeds it);
  - `easel_palette_bar_press_indicator (bar, gboolean front)`;
  - `easel_window_finish_color_choice (win, const GdkRGBA *rgba_or_NULL)`, which the chooser's callback also calls.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | core `as1_default_palette` (the 28 values), ui `as1_palette_bar`, ui `as1_swatch_layout` + manual M1 |
| 2 | manual M2 |
| 3 | ui `as3_click_picks_colors` |
| 4 | ui `as4_swap_colors` |
| 5 | ui `as5_strokes_use_current_colors` |
| 6 | core `as6_transparent_stroke_exact`, ui `as6_transparent_background_swap` + manual M3 |
| 7 | ui `as7_colors_are_not_edits` |
| 8 | manual M4 |
| 9 | ui `as9_double_click_edits_swatch` + manual M5 (the real chooser) |
| 10 | ui `as10_f2_edits_focused_swatch` + manual M6 |
| 11 | ui `as11_click_sequences` |
| 12 | ui `as12_indicator_edits_color` + manual M7 |
| 13 | manual M8 |
| 14 | io `as14_writes_whole_palette` |
| 15 | io `as15_write_failure_silent`, io `as15_existing_file_kept_when_write_fails`, ui `as15_swatch_changes_when_write_fails` (failure paths, automated) |
| 16 | core `as16_palette_syntax`, io `as16_garbage_file_defaults`, io `as16_mixed_entries_fall_back` |
| 17 | ui `as17_undo_ignored_while_chooser_open` |
| 18 | ui `as18_bar_geometry_and_swap_button` + manual M9 |

### Constitution check
- P1: GNOME widgets and the system color chooser.
- P6: the palette is remembered in a configuration file.
- Layering: parsing in core (libc only), files in io, widgets in ui.
- Colors are not edits, so the 0002 rule of one undo step per edit is untouched.
- No performance budget in `docs/engineering.md` covers the palette, so there is no measurement task.

### Risks
- **The chooser in tests.** The ui tests drive the window's color-choice entry point instead of GTK's dialog window,
  whose internals may change. The real chooser (opening, confirming, cancelling, alpha) is covered by manual checks M5
  to M7.
- **Modality.** Whether the compositor blocks the parent window under a modal dialog varies, so the "chooser open" flag
  is what guarantees REQ-0003-14; AS-17 tests it.
- **Double-click timing** comes from the GTK settings; the tests pass `n_press` through the bar's entry point.
- **Moving the parser** touches `settings.c`. The existing settings tests (as2, as5, as7, as18, as36) must stay green
  unchanged.

## Tasks
- [x] T001 Move the settings line and color parser into a shared `ini` module (REQ-0001-40, REQ-0002-18, REQ-0003-17) - done when the existing settings tests from 0001 and 0002 (core test-settings, io test-settings-file) pass with their test files unchanged, and the build is warning-free
- [x] T002 Core palette: defaults, parse, format (REQ-0003-04, REQ-0003-15, REQ-0003-17; AS-1, AS-16) - done when core `as1_default_palette` (all 28 values by number) and `as16_palette_syntax` pass, and formatting then parsing gives the same palette, transparent colors included
- [x] T003 Palette file: read, atomic write, silent failure (REQ-0003-15, REQ-0003-16, REQ-0003-17; AS-14, AS-15, AS-16) - done when io `as14_writes_whole_palette`, `as15_write_failure_silent`, `as16_garbage_file_defaults` pass, and `as16_mixed_entries_fall_back` passes: a palette.ini with one valid entry, one garbage entry and one missing entry gives the file's value for the valid swatch and the defaults only for the garbage and missing ones
- [x] T004 Transparent pencil colors write exact pixels (REQ-0003-08, REQ-0003-05; AS-6) - done when core `as6_transparent_stroke_exact` passes: strokes in #00000000 and #FF000080 over opaque pixels leave exactly those values, with no blending and no checkerboard in the picture
- [x] T005 Palette bar: indicator, 28 swatches, layout, minimum width (REQ-0003-01, REQ-0003-02, REQ-0003-03, REQ-0003-04, REQ-0003-05; AS-1, AS-2) - done when ui `as1_palette_bar` passes, ui `as1_swatch_layout` passes (each swatch 20x20 logical pixels with 2 between neighbors, two rows of 14, swatch n at row 1 column n for n <= 14 and row 2 column n - 14 otherwise), and manual M1, M2 pass
- [x] T006 Choosing colors: clicks, focus, middle click, swap, strokes, not edits (REQ-0003-06, REQ-0003-07, REQ-0003-08, REQ-0003-09, REQ-0003-10; AS-3, AS-4, AS-5, AS-6, AS-7, AS-8) - done when ui `as3`, `as4`, `as5`, `as6_transparent_background_swap` and `as7` pass, and manual M3, M4 pass
- [x] T007 Editing colors: double-click, F2, indicator, modal chooser, quit while open (REQ-0003-11, REQ-0003-12, REQ-0003-13, REQ-0003-14; AS-9, AS-10, AS-11, AS-12, AS-13, AS-17) - done when ui `as9`, `as10`, `as11`, `as12` and `as17` pass, and manual M5 to M8 (the real chooser) pass
- [x] T008 Remember the palette: load at start, write on each confirmed edit (REQ-0003-15, REQ-0003-16, REQ-0003-17; AS-14, AS-15) - done when ui `as15_swatch_changes_when_write_fails` passes and a confirmed edit in the window leaves palette.ini with that value, which the next window shows
- [x] T009 Bar dimensions and swap button (REQ-0003-18, REQ-0003-19; AS-18) - done when ui `as18_bar_geometry_and_swap_button` passes (indicator 46x42 with the two squares at their positions, swatch pitch 22, swap button swaps, and does nothing while the chooser is open) and manual M9 passes
- [x] T010 Close the gaps /sdd-check 0003 found (REQ-0003-09, REQ-0003-15) - done when io `as15_existing_file_kept_when_write_fails` passes and fails when the palette is written in place, and ui `as9` shows a swatch edit leaves no hidden undo step
