# 0020 Preferences

Status: implemented
Reviewed: spec-reviewer, 2026-11-03 (12 findings merged; the changes shown to JOD old/new, not reviewed again)
Depends on: 0001 (settings file and its syntax), 0002 (undo budget), 0003 (palette), 0004 (panels), 0005 (JPEG quality, Save), 0007 (Magnifier level), 0009 (workspace color), 0010 (line width), 0014 (selection tint), 0017 (text style), 0018 (how a command and its key are gated)

## Why
Everything a person can set in Easel is a line in a hand-edited file today. Preferences puts those settings in one GNOME dialog, applied at once, and leaves the file as the single store, so that someone who edits it by hand loses nothing.

## Requirements
### Opening it
- REQ-0020-01 The primary menu (REQ-0005-01) shall hold "Preferences" (Ctrl+,) directly before Quit, in Quit's section. The item and the key do nothing before the picture exists, while a dialog, the quit prompt or the color chooser is open, while any mouse button is held in the canvas area, or while the focus is in a text field of the font card (as REQ-0018-01). Opening it changes nothing: a pending shape, an open text box and a selection stay as they are, and may be seen changing as the settings apply. Ctrl+Q, Ctrl+W or a close request while it is open shall close it, with any chooser of its own, committing a typed value, and then quit as REQ-0001-34 and REQ-0001-35 say (JOD, 2026-11-03).
- REQ-0020-02 It shall be a modal GNOME preferences dialog titled "Preferences" with three pages, Appearance, New Picture and Drawing, which opens on Appearance every time. It has no OK or Cancel: a change applies at once (REQ-0020-08). Esc and the close button close it. Each row shows the value last chosen in this run or, if none, the value read at start-up.

### The rows
- REQ-0020-03 Page Appearance:
  - Group Style: "Style", a choice of System, Light and Dark, key `[view] style` (`system`, `light`, `dark`; default `system`). Light and Dark override the desktop's setting for the whole app; System follows it (REQ-0009-06).
  - Group Workspace: "Workspace color" and "Workspace color, dark" (REQ-0009-04), each a color swatch that opens the system color chooser with no alpha, and a button "Use Default" that removes the key, insensitive while the key is not set. An unset swatch shows what that theme would use without it (REQ-0009-06): light #E7E7E7; dark `workspace-color` if set, else #2C2C2C. Written as `#RRGGBB`, upper case.
  - Group Selection: "Tint strength", a number field in percent from 0 to 50, key `[view] tint` (default 12; REQ-0014-17). 0 draws no tint. It covers Select, Free-Form Select and the text box.
  - Group Panels: "Tool Box", "Color Box" and "Status Bar", switches, keys `[view] toolbox`, `colorbox`, `statusbar` (REQ-0004-27). A switch sets how the panel starts and shows or hides it now, and the View menu's check item follows. The View menu's items still change only the current run and do not move the switch or the key. Written `on` or `off`.
  - Group Palette: a button "Reset Palette…" (REQ-0020-07).
- REQ-0020-04 Page New Picture:
  - "Automatic size", a switch: on means `width=auto` and `height=auto` (REQ-0001-02). While it is on, Width and Height below are insensitive and show the size a new picture would get now. An `auto`, missing or invalid value in either key counts as on (REQ-0001-05).
  - "Width" and "Height", number fields in pixels from 1 to 10000, keys `[new-picture] width`, `height`. Turning the switch off writes the shown size to both keys, and turning it on writes `auto` to both, each in one write: both or neither.
  - "Canvas color", a swatch that opens the system color chooser with alpha, key `[new-picture] canvas-color` (REQ-0001-03), written `#RRGGBB` when opaque, else `#RRGGBBAA`, upper case. It shows a valid `canvas-color`, else a valid `background`, else #FFFFFF; `transparent` shows as alpha 0.
  - These apply to the next New and the next start; the open picture does not change.
- REQ-0020-05 Page Drawing:
  - Group Tools, each a choice naming the tool's presets in words ("Circle, 4 pixels"), in section `[drawing]`: "Brush", key `brush` (`circle-7`, `circle-4`, `circle-1`, `square-8`, `square-5`, `square-2`, `slash-8`, `slash-5`, `slash-2`, `backslash-8`, `backslash-5`, `backslash-2`; default `circle-4`; REQ-0006-04); "Airbrush", key `airbrush` (`9`, `16`, `24`; default 16; REQ-0006-07); "Eraser size", key `eraser` (`4`, `6`, `8`, `10`; default 8; REQ-0006-10); "Eraser mode", key `eraser-mode` (`opaque`, `transparent`; default `opaque`; REQ-0006-10); "Line and shape width", key `line-width` (1 to 5 pixels; default 1), for every tool of REQ-0010-03; "Fill style", key `fill-style` (`outline`, `outline-and-fill`, `fill`; default `outline`; REQ-0010-03); "Magnifier level", key `magnifier` (`1`, `2`, `4`, `6`, `8`, `16`; default 2; REQ-0007-02). They apply at the next start, and each row's subtitle says so: "Takes effect the next time Easel starts."; the session's choices stay (REQ-0006-03, REQ-0010-03, REQ-0007-02).
  - Group Text: "Font", a list of the installed families with a search, with "Automatic" first (`auto`, REQ-0017-24); "Size", a number field in points from 1 to 500; "Bold", "Italic", "Underline" and "Strikethrough", switches written `true` or `false`, the family as the list names it: section `[text]`. They apply to the first box of the next run, as REQ-0017-24 says, and the group's description says "Takes effect the next time Easel starts."
  - Group History: "Undo memory", a number field in megabytes from 16 to 8192, key `[history] memory` (REQ-0002-18), with the subtitle "Takes effect the next time Easel starts."
  - Group Saving: "JPEG quality", a number field from 1 to 100, key `[save] jpeg-quality` (REQ-0005-14). It applies to the next JPEG saved, not only after a restart.
- REQ-0020-06 Each number field takes whole numbers and has a minus and a plus button. Its value changes by 1 with the buttons, the Up and Down keys and the wheel while the field has the focus (JOD, 2026-11-03: without the focus the wheel scrolls the page, so that scrolling never changes a setting), by 10 with Page Up and Page Down, and by typing; each step is one change, and after any burst of them the file holds the last value on one line and the app keeps up; a change past its limit stops at the limit, a typed number past its limit becomes the limit, and text that is not a whole number goes back to the value it had (REQ-0004-21). A typed value takes effect when the focus leaves the field, at Enter, or when the dialog closes. A number field shows its unit as Stretch and Skew does, and a typed value may have it after it (REQ-0019-03): `%`, `px`, `MB`, `pt`; JPEG quality has none. A choice that the file holds in no list above (`line-width=7`) shows the default, and the file is not rewritten until that row changes.
- REQ-0020-07 "Reset Palette…" shall ask with a dialog, heading "Reset the Palette?", body "All 28 colors go back to the classic ones. This cannot be undone." and the buttons Cancel and "Reset". Cancel is the default and Esc is Cancel. Reset gives the swatches the classic 28 colors of REQ-0003-04 at once and removes the palette file, and no palette file is no failure; the foreground and background colors stay (REQ-0003-10). If the file cannot be removed, the swatches still change for this run, silently (as REQ-0003-16).

### Applying and the file
- REQ-0020-08 A change shall apply at once and be written to the settings file (REQ-0001-01) at once, one change at a time. A picture, its pixels, its undo steps and its modified mark are never changed by Preferences.
- REQ-0020-09 Writing shall change only that key's line, in this form: `key=value`, the rest of the file byte for byte: comments, blank lines, the order, unknown sections and keys. When the key repeats, the last line is rewritten (the one that counts, REQ-0001-40) and the others stay. A key that is missing goes directly after the last key line of the section's last occurrence, or after its header if it has none; a section that is missing is added at the end of the file, after a blank line, with none in an empty file. A removed key's lines all go. If the last line has no line ending, one is added before an appended line, and no other line changes. A line keeps its line ending, and a new line copies the file's. A symbolic link stays and its target is written, and the file's permissions stay. A write replaces the whole file in one step, so that a failure never leaves it half written, and it is made against the file as it is on disk at that moment, so that a hand edit made while the app runs is kept; the app does not reread the other values until it starts again. An older `background` line stays; `canvas-color` is added or changed and wins over it (REQ-0001-03).
- REQ-0020-10 If the file cannot be read or written (a read-only file counts, also in a writable folder), is a folder, or holds a line that makes it unparseable (REQ-0001-05), the file shall be left exactly as it is, the change shall still apply for this run where it applies in this run (Appearance, New Picture, JPEG quality), and an error dialog shall show once for each opening of Preferences, heading "Could Not Save the Preferences" and body "The change applies until Easel closes, but settings.ini could not be written." A typed value committed as the dialog closes shows the error over the window. A missing file, or a missing folder, is created as REQ-0001-04 says, with the change in it; if that fails, the error shows. A setting that applies only at a later start then has no effect, and it still shows the value chosen for the rest of the run (REQ-0020-02).
- REQ-0020-11 A settings file the app creates (REQ-0001-04) shall also hold `style=system` and `tint=12` in `[view]`, and `[drawing]` with `brush=circle-4`, `airbrush=16`, `eraser=8`, `eraser-mode=opaque`, `line-width=1`, `fill-style=outline` and `magnifier=2`. An existing file is written only by a change made here. A missing or invalid value of a new key means its default, silently (REQ-0001-05). In `[view] style` the value's case is ignored (REQ-0001-40).

### Save
- REQ-0020-12 Save of an opened file stays silent, with no setting (JOD, 2026-11-03; REQ-0005-13).

## Scenarios
- AS-1 (REQ-0020-01) Given a picture, When "Preferences" is chosen or Ctrl+, pressed, Then the dialog opens on Appearance; Given a stroke held, the quit prompt, the color chooser open, or the focus in the font card's size field, When Ctrl+,, Then no dialog opens; Given a waiting curve, an open text box or a floating selection, When it opens and closes, Then they are as they were and no step is added; Given it open with a typed value, When Ctrl+Q, Then it closes, the value is written, and the app quits or asks as usual | Verified by: automated (ui) + manual M1 (the real key, also on a Ukrainian layout)
- AS-2 (REQ-0020-02) Given a setting changed and the dialog closed by Esc, When it opens again, Then it is on Appearance and shows the new value | Verified by: automated (ui)
- AS-3 (REQ-0020-03, REQ-0009-06) Given Style Dark on a light desktop, Then the app is dark at once and the workspace takes `workspace-color-dark` or #2C2C2C; When System, Then it follows the desktop; Given a workspace color chosen, Then the workspace shows it at once and the key is written in upper case; When Use Default, Then the key is gone, the button insensitive, and the color of REQ-0009-06 shows; Given Style Light, When the desktop goes dark, Then nothing changes | Verified by: automated (ui) + manual M1
- AS-4 (REQ-0020-03, REQ-0014-17) Given an accent of #3584E4 and a yellow picture and Tint strength 40, Then a marked, a floating and a free-form selection and a text box show #AECE5B, within 1 in each channel, over the yellow; at 0 exactly yellow; the picture's pixels are unchanged at every value | Verified by: automated (ui, a rendered window)
- AS-5 (REQ-0020-03, REQ-0004-27) Given Tool Box off in Preferences, Then the toolbox hides now and the key is `off`; and the View menu's item is unchecked; When the View menu shows it, Then it shows, the switch and the key are still off; a restart starts with it hidden | Verified by: automated (ui)
- AS-6 (REQ-0020-04) Given a canvas area of 1100 × 800 and Automatic size on, Then Width and Height are insensitive and show 1024 × 768; at 1000 × 700 they show 800 × 600; `width=0` with `height=300` counts as on; When the switch is turned off, Then `width=1024` and `height=768` are written; When Width is 300 and New chosen, Then the new picture is 300 wide; the open picture did not change before it; When it is turned on, Then both keys are `auto`; Given a transparent Canvas color, Then the key is `#00000000` | Verified by: automated (ui) + automated (core)
- AS-7 (REQ-0020-05) Given a Brush of `square-5`, an Airbrush of 24, an Eraser of 10 in Transparent mode, Line and shape width 3, Fill style `fill`, Magnifier level 8x and a size 24 bold font set, Then each row's subtitle says it takes effect at the next start, each session choice is unchanged and the file holds them; When the app starts again, Then every tool starts at its chosen preset, a Rectangle with the same width and fill, and the first box in that style; the font row's search finds a family and Automatic is first; Given Undo memory 1024, Then the budget in this run is unchanged and the next start uses it; Given JPEG quality 50, When a JPEG is saved, Then it is written at quality 50 in this run | Verified by: automated (ui) + automated (io)
- AS-8 (REQ-0020-06) Given each number field at its limits, When its minus or plus button, Up, Down, Page Up, Page Down or the wheel is used, or "99999", "-5", "abc", "" and "1.5" are typed, Then the value stops at or becomes the limit, or goes back for the text; a typed value counts when the field is left, at Enter, and at Esc on the dialog; "40 %" and "40%" are 40 and "40 px" goes back in Tint strength; 100 quick steps leave one line with the last value; Given `line-width=7`, Then the row shows 1 and the file still holds 7 | Verified by: automated (ui) + manual M1 (the wheel with and without the focus)
- AS-9 (REQ-0020-07) Given a changed palette, When Reset Palette… and Cancel or Esc, Then nothing changes; Reset with no palette file shows no error; When Reset, Then the 28 classic colors show, the palette file is gone, and the foreground and background stay; Given a palette file that cannot be removed, Then the swatches still change, with no dialog | Verified by: automated (ui)
- AS-10 (REQ-0020-08) Given a modified picture with undo steps and every row changed, Then its pixels, its steps and the modified mark are as they were | Verified by: automated (ui)
- AS-11 (REQ-0020-09) Given a file with comments, blank lines, unknown keys, `grid = on` with spaces, and `size` twice in `[text]`, When Size changes, Then only the last `size` line differs and it reads `size=14`; When a missing key and a missing section change, Then each is added as the requirement says; When a removed key repeats, Then all its lines go; Given a hand edit made while the app runs, When another row changes, Then the hand edit is still there; Given `background=#FFFF00` and no `canvas-color`, When Canvas color changes, Then `canvas-color` is added and `background` stays; Given a repeated `[text]`, a section ending in comments, a last line with no line ending, CRLF endings, an empty file and a symbolic link, Then each is written as the requirement says | Verified by: automated (core)
- AS-12 (REQ-0020-10) Given a settings folder that cannot be written, a read-only file, or a file that is a folder, When a tint and a Style change, Then both apply, the error dialog shows once, the file is byte for byte as it was, and the rows still show them when opened again; Given a file with an unparseable line, Then the same; Given no file and no folder, When a row changes, Then both are created with the defaults and that change; a write that fails midway leaves the old file whole; a typed value committed by closing shows the error over the window | Verified by: automated (io) + automated (ui)
- AS-13 (REQ-0020-11) Given no settings file, When the app starts, Then the file holds the new keys with their defaults; Given an existing file without them, When the app starts, Then it is byte for byte unchanged and the defaults hold; `style=DARK` is dark, `tint=51`, `tint=abc` and `line-width=0` mean their defaults | Verified by: automated (core)
- AS-14 (REQ-0020-12) Given a picture opened from a JPEG and modified, When Save, Then it is written with no question | Verified by: automated (ui, the existing test of REQ-0005-13)

## Edge cases
- The dialog over a window resize, a zoom or a scroll: not reachable by the mouse while it is modal; a window resize changes no row, except the size shown while Automatic size is on, which is read when the dialog opens, when the switch changes and when a panel is shown or hidden.
- A second Ctrl+, while it is open does nothing.
- A color chooser opened from a row: Esc closes the chooser, and Preferences stays open; nothing changes until a color is confirmed. Esc in the font row's popover closes only the popover (REQ-0017-14).
- An empty family or a family that is not installed in the file: REQ-0017-24 holds, and the row shows Automatic until it changes.
- A value that a hand edit made invalid shows its default in the row, and the file keeps that value until that row changes.
- A second copy of the app cannot exist (REQ-0001-07), so nothing else writes the file except a hand edit, which REQ-0020-09 keeps.

## Out of scope
- Frame Time's row (Window Extras adds it) and the Language list (Languages adds it); the smoothing defaults (Phase 2); the grid's start-up keys, which stay in the file; the standard new-picture sizes, which stay constants (JOD, 2026-11-03); default foreground colors (REQ-0003-10); importing or exporting settings.

## Open questions

## Plan
**Approach.** The file's rules are pure core: an editor that takes the file's text, a section, a key and a value and gives the new text. io reads, edits and replaces the file. The dialog holds the rows and calls one window function per kind of change; the settings struct in the window stays the one place every later read comes from, so New, Save and the next box already follow it.

**Files.** New: `src/ui/preferences-dialog.{c,h}`, `src/ui/unit-row.{c,h}` (the number field with its unit, moved out of `stretch-skew-dialog.c`), `tests/core/test-settings-edit.c`, `tests/io/test-settings-change.c`. Changed: `src/core/settings.{c,h}`, `src/io/settings-file.{c,h}`, `src/io/palette-file.{c,h}`, `src/ui/window.{c,ui}`, `src/ui/application.c`, `src/ui/canvas.{c,h}` (tint), `src/ui/tool-options.{c,h}` (start presets), `src/ui/font-card.{c,h}` (the family list, shared), `src/ui/stretch-skew-dialog.c`, `tests/ui/test-window.c`, `tests/meson.build`, `tests/mutants.txt`.

**Key interfaces.**
- Core: `EaselSettings` gains `style`, `tint`, `brush`, `airbrush`, `eraser`, `eraser_transparent`, `line_width`, `fill_style`, `magnifier`, parsed and defaulted as the other keys. `bool easel_settings_edit (const char *text, size_t len, const char *section, const char *key, const char *value, char **out, size_t *out_len)`: REQ-0020-09, a NULL value removes the key. `bool easel_settings_parseable (const char *text, size_t len)`.
- Io: `bool easel_settings_file_change (const char *path, const EaselSettingChange *changes, size_t n)`: reads the file as it is now, refuses an unparseable or unwritable one, applies all `n` edits to the text, then one replace: both keys of Automatic size or neither.
- Ui: `easel_preferences_dialog_present (EaselWindow *win)`. `void easel_window_apply_setting (...)`, one per row kind: updates `self->settings`, applies what applies now, writes, and shows the error once per opening. `easel_canvas_set_tint (canvas, percent)`. `easel_tool_options_set_start (...)`: the start presets, read once at start-up. `GtkWidget *easel_unit_row_new (...)`.

**The one function.** Every change goes through `easel_settings_file_change`, and every quit through the window's existing close handling, which first closes Preferences, committing a typed value. What opening does: nothing is put down, so the action does not call `put_down_for_action`.

**Readers of what this extends.**
- `dialog_is_open` and `view_actions_allowed` now also see Preferences: every action gated by them stays off while it is open (REQ-0020-01). The quit paths read `dialog_is_open` to refuse: REQ-0001-34, -35 and REQ-0005-02 are the ones that change, for this dialog only; T007 lists each.
- `append_selection_tint`'s 0.12f becomes the setting; readers of the tint in tests: `tinted ()` in `tests/ui/test-window.c` and 0014's and 0017's cases, which keep the default of 12. The mutants quoting that line (`the tint is a fixed blue`, `the tint is 28 % as before`) are brought to the new text.
- `self->settings`: `width`, `height`, `background` (New, Attributes' Default), `jpeg_quality`, the text style. They now change during a run; REQ-0005-03, REQ-0004-21 and REQ-0005-14 have their pointer notes.
- `apply_workspace_color`, called once at start and on a theme change, now also on a change of either key or of Style.
- A created file's text: `easel_settings_default_text`, and the tests that read it (AS-4 of 0001, AS-14 of 0004).

**What grows.** Nothing is kept: each change reads and replaces a file of a few hundred bytes.

**Mutants** (the rule of `docs/engineering.md`: `tools/mutate.sh --core` once, at the end). The core editor, the parser and the new defaults get theirs, against the core tests; the file's rules guard against losing a hand edit, so these are not optional. See the note below on the mutant JOD named.

**The mutant of REQ-0014-17 (JOD, 2026-11-03).** To re-add it I looked for it, and the record does not match its description: the only REQ-0014-17 mutant dropped as equivalent is the dashes' timer (0014's Check, line 404), dropped because a 1/12 s timer finds a new step at almost every firing. It has nothing to do with colors, and no setting revives it. No mutant of the tint or of the workspace colors was ever dropped: a search of every version of `tests/mutants.txt` finds 4 removed in all, none of them of REQ-0014-17 or the workspace. The nearest is REQ-0017-05's swap of the tint and the highlight, which looked equivalent because both are the accent; it was kept, killed by `as27_highlight_over_tint_exact`, at 12 % only. **Proposed:** that test also runs at another tint strength, now that the strength can change, plus a new mutant, "the tint strength from the settings is ignored", both killed in T003. JOD to confirm or name the mutant he means.

**Scenario map.**
| AS | Test |
|---|---|
| 1 | ui `as1_preferences_opens`, `as1_quit_closes_it`, manual M1 |
| 2 | ui `as2_opens_on_appearance` |
| 3 | ui `as3_style_and_workspace`, manual M1 |
| 4 | ui `as4_tint_strength`, `as27_highlight_over_tint_exact` |
| 5 | ui `as5_panel_switches` |
| 6 | ui `as6_new_picture_rows`, core `as6_size_keys` |
| 7 | ui `as7_drawing_rows`, io `as7_values_on_disk`, core `as7_new_keys` |
| 8 | ui `as8_number_fields`, manual M1 |
| 9 | ui `as9_reset_palette` |
| 10 | ui `as10_picture_untouched` |
| 11 | core `as11_edit_*`, one per file shape |
| 12 | io `as12_*`, ui `as12_error_once` |
| 13 | core `as13_new_keys_and_defaults` |
| 14 | ui the existing test of REQ-0005-13 |

**Constitution.** Core stays libc only with bounds checked; this completes P6 for what is built; no ADR (JOD). No picture pixel is ever written (AS-10).

**Risks, each checked before the code relies on it.**
- R1 `g_file_set_contents_full` on a symbolic link and on permissions: the spec wants the link kept and the target written, with its mode. Probe in T002, `io as12_symlink_and_mode`. Fallback: resolve the link and pass the mode ourselves.
- R2 A read-only file in a writable folder: a rename would succeed. T002 tests it, with an access check before the replace.
- R3 The wheel with and without the focus on an `AdwSpinRow` inside a scrolling page: a probe in the first ui task, in a real frame, before the row is built. Fallback: a scroll controller in the capture phase.
- R4 `AdwComboRow` drawing a preset as the options panel does: a probe in T005. Fallback: the same preset button grid in a row.
- R5 The family list of the font card lives inside the card. T006 moves it behind a function both use; 0017's popover tests are its guard, so that task runs `ui-0017`.
- R6 An error dialog over a modal Preferences, and when typed values commit at close: T007 tests where it shows.
- R7 100 quick writes: T002 measures them. A write is a read, an edit and a rename with no fsync; if that lags a held plus button, the row writes on release and at close, and the spec's "each step" gets JOD's look.
- R8 Where the keys go after it closes: manual in M1, since the isolated session does not return the focus as the desktop does.

## Tasks
- [x] T001 Core: the new keys and defaults, and the editor of the file's text (REQ-0020-09, -11; AS-7, AS-11, AS-13) - done when the core tests are red first, then green on every file shape
  Result: `as11_edit_changes_one_line` and the others failed first by build (`implicit declaration of easel_settings_edit`, no member `style`). Green on every file shape, valgrind clean; a failed allocation is tested for real (`--wrap`).
- [x] T002 Io: a change to the file in one step, with R1, R2 and R7 checked (REQ-0020-09, -10; AS-7, AS-12) - done when the io tests pass, the link, the mode and a read-only file included, and 100 quick writes are measured
  Result: `as7_values_on_disk` and the others failed first by build (`easel_settings_file_change` undeclared). Green, valgrind clean. R1: the link stays and its target is written, mode 0640 kept (`realpath`, then the replace with the mode given). R2: a read-only file in a writable folder is refused by an access check first. R7: 100 changes take 250 ms on the real ext4 disk (2.5 ms each, 1 ms on tmpfs), far under a held button's repeat, so each step writes and the spec stands.
- [x] T003 The tint strength in the canvas; `as27_highlight_over_tint_exact` at a second strength and the mutant "tint strength from the settings is ignored", killed (REQ-0020-03; AS-4) - done when `as4_tint_strength` and that mutant are done
  Result: `as4_tint_strength` failed first by build (`easel_canvas_set_tint` undeclared), then on the behavior: `marked at 40: rendered #FFF2E814, want #FFD3B345`, the strength reset by `set_image`, where the default had first gone; it would have reset at every New. `as27_highlight_over_tint_exact` passes at 12 and 40; the mutant "tint strength from the settings is ignored" is killed by `as4_tint_strength`. Three quoted mutants brought to the new text; "the tint is 28 % as before" renamed "the tint is a fixed strength, not the setting".
- [x] T004 The dialog: item, key, pages, the number field with its unit, the wheel by R3's probe (REQ-0020-01, -02, -06; AS-1, AS-2, AS-8) - done when ui-0020 passes
  Result: `as1_preferences_opens` and the others of T004 to T009 were written first, red by build (`easel_window_get_primary_button` and `easel_swatch` missing, no action). R3: the spin row's scroll controller reads as off; `easel_unit_row_wheel_with_focus` turns it on while the field has the focus, for M1 to confirm.
- [x] T005 Page Drawing, Tools: the rows by R4's probe, and the start presets (REQ-0020-05; AS-7) - done when `as7_drawing_rows` passes
  Result: Green with T004's tests. R4: `AdwComboRow` takes a factory, but words are clearer than 12 small pictures in a list and cost far less, so the rows name the presets (the wording of REQ-0020-05 changed, shown old/new).
- [x] T006 Page Drawing: Text by R5, Undo memory, JPEG quality (REQ-0020-05; AS-7) - done when `as7_drawing_rows` and `ui-0017` pass
  Result: Green with T004's tests. R5: the font card's list is tied to the card; an `AdwComboRow` with its search does the job (the wording of REQ-0020-05 changed, shown old/new), so 0017 is untouched.
- [x] T007 Page Appearance: Style, workspace, panels, Reset Palette (REQ-0020-03, -07; AS-3, AS-5, AS-9) - done when their cases pass
  Result: Green with T004's tests.
- [x] T008 Page New Picture (REQ-0020-04; AS-6) - done when its cases pass
  Result: Green with T004's tests.
- [x] T009 The error dialog, and quitting with it open, by R6 (REQ-0020-01, -10; AS-1, AS-12) - done when their cases pass
  Result: Green with T004's tests. R6: an alert stacks over Preferences. The window's close request closes it first, and its `closed` handler commits a typed value.
- [x] T010 The picture is never touched, and Save stays silent; manual M1: the look of the pages, the wheel with and without focus, the keys after it closes, Ctrl+, on a Ukrainian layout, Style with the real desktop (REQ-0020-08, -12; AS-1, AS-3, AS-8, AS-10, AS-14) - done when JOD has seen it
  Result: M1: JOD, 2026-11-03, "M1 is ok". AS-10 by `as10_picture_untouched`; AS-14 by the existing test of REQ-0005-13.
- [x] T011 The mutants of the file rules, and `tools/mutate.sh --core` once, in the background (REQ-0020-09, -10, -11) - done when its summary line is here and each survivor is one line for Check
  Result: 20 mutants of the file rules, killed; two more written and removed as equivalent, a comment that looks like a key (`#size` never equals `size`) and the mode (GLib keeps an existing file's mode). Summary line in Check.

## Known gaps
In `docs/phase2-plan.md`: the cases of AS-1, AS-3, AS-7 and AS-9 listed there, which the code handles and no test of its own observes, and the Font row's look.

## Check
- Date and commit: 2026-11-03, on `b57eefe` with all of 0020 in the working tree. Versions as the last full run: GTK 4.24.1, Pango 1.58.0, Cairo 1.18.4, GNOME Shell 51.0; none changed since.
- Tests: `meson test` through `tools/isolated.sh`: 60 of 60 (414 s); `--scale 2 ui-0025-scale2 ui-0019-scale2` 2 of 2; the cases of this feature: 12 in `/0020`.
- Mutants, `tools/mutate.sh --core`: 397 mutants: 396 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 0 errors, 1 accepted in 152 s. Named runs of REQ-0020-04, -09, -10, which hold `src/io`'s: all killed. Two written and removed as equivalent (T011). The mutant JOD asked for, "tint strength from the settings is ignored", is killed by `as4_tint_strength`.
- Warnings: 0 after a forced rebuild. `spec.sh lint`, `--names 0020`, `mutate.sh --lint`: clean.
- Scenarios: as in the map; each task's Result is in Tasks above. C marks: none; T004 to T009's tests were written first and red by build as one group, so no single red run per task.
- Findings:
  - F1 M1 (the look of the pages, the wheel with and without focus, the keys after it closes, the Ukrainian layout): JOD, 2026-11-03, "M1 is ok". Settled.
  - F2 REQ-0019-03's wheel in Stretch and Skew: a probe read that row's only scroll controller as off, and it was asked at M1 (its step 3); JOD reported M1 ok with no problem named. Settled on that word; no headless test observes it.
  - F3 `/0005/as1_file_menu` still expected Quit alone in its section, and stopped `ui-0005`: updated, with a pointer note in REQ-0005-01. The full suite caught it; the per-task filter does not run other features' tests.
  - F4 The tint's default in `set_image` reset the strength at every New: found red in T003. A start-up crash from setting presets before the panel existed: found in T004.
  - F5 Clauses with no test of their own, the code read and handling them: see Known gaps. Left open: no user-visible bug.
  - F6 Decisions shown to JOD: REQ-0020-05's tool rows in words and its Font row; a write per step (R7).
