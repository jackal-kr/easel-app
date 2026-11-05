# 0021 Window Extras

Status: implemented
Reviewed: spec-reviewer, 2026-11-03 (12 findings merged; the changes shown to JOD old/new, not reviewed again)
Depends on: 0004 (View menu, status bar), 0013 (Esc), 0016 (the canvas menu), 0017 (the font card's look and its Esc), 0018 (how a command and its key are gated), 0020 (Preferences), 0025 (the pixel contract)

## Why
The last pieces of the window: a list of every key, a Help page, credits, a small actual-size view while working zoomed in, a right-click menu in Pan, and a frame time for looking at speed. Print is its own feature (JOD, 2026-11-03).

## Requirements
### The primary menu
- REQ-0021-01 The primary menu shall end in three sections, a separator between each (JOD, 2026-11-03): Preferences (REQ-0020-01); then "Keyboard Shortcuts" (Ctrl+?, Ctrl+F1), "Help" (F1), "About Easel"; then Quit. Keyboard Shortcuts, Help and About Easel and their keys do nothing before the picture exists, while a dialog, the quit prompt, the color chooser or a menu is open, while any mouse button is held in the canvas area, or while the focus is in a text field of the font card (as REQ-0018-01). Opening them changes nothing: a pending shape, an open text box and a selection stay as they are. Quit keeps its own rules (REQ-0001-34, -35, -39). Ctrl+? follows REQ-0018-01's rule for a layout that is not Latin, and Ctrl+F1 is there for a keyboard with no key for `?`.

### Keyboard Shortcuts
- REQ-0021-02 (Pointer note, Print, 2026-11-03: Ctrl+P joins File, REQ-0026-02.) "Keyboard Shortcuts" shall open a modal GNOME shortcuts dialog listing every key the app has, each with what it does, in the groups File, Edit, Image, View, Tools, Colors and Text. A key a menu shows and a key no menu shows (Esc, the arrows, Numpad + and −, F2, Tab, Ctrl+Shift+X, the text box's) are listed alike, and a key with two meanings is listed for each. Esc closes it.
- REQ-0021-03 Ctrl+Q, Ctrl+W or a close request while Keyboard Shortcuts, About Easel or Help's error dialog is open shall close it, then go on as REQ-0001-34 and REQ-0001-35 say.

### Help
- REQ-0021-04 (Pointer note, Packaging and release: the page gains a part "Saving", REQ-0024-12.) (Pointer note, Languages, 2026-11-04: the page opens in the interface's language, REQ-0023-09.) (Pointer note, Print, 2026-11-03: the page names Ctrl+P and describes Print and its Picture tab, REQ-0026-02.) "Help" shall open one page in the default browser, "Easel Help", installed with the app: what Pan mode is, each tool in the toolbox's order with what it does and its options, selections, text, the Image and View menus, Preferences, undo, and every key of REQ-0021-02. The app goes on as it was; the page does not need the app. A key repeat does nothing, and neither does a request while one is still starting. If the browser cannot be started or the page is missing, an error dialog shall show, heading "Could Not Open Help", body "The help page could not be opened." A failure reported later waits until no button is held and no dialog is open, and is dropped once a quit has begun.

### About
- REQ-0021-05 "About Easel" shall open a GNOME about dialog with the name Easel, the version, the license GPL-3.0-or-later, and an Acknowledgements section with "Microsoft Paint in Windows XP" and "JS Paint by Isaiah Odhner" (`docs/product.md`). Esc closes it.

### Thumbnail
- REQ-0021-06 The View menu shall hold a check item "Show Thumbnail", directly after Grid…, with no key (XP has none). It is sensitive only while the zoom is above 100 % (XP). At 100 % and below the card is not shown, and the item stays checked, so that the card shows again when the zoom passes 100 %. The item starts off at every start.
- REQ-0021-07 While shown, the thumbnail shall be a floating card over the canvas area, in the look of the font card (REQ-0017-14), a rounded card with 6 logical pixels of padding around a square picture area of 160 × 120 logical pixels that starts on a whole device pixel; the card's top right corner lies 12 logical pixels from the canvas area's top right, and it keeps that corner through a window resize. The picture area shows the picture at 100 %, one picture pixel to a device pixel (P7.2). Its first pixel is the picture pixel whose drawn square (REQ-0025-02) holds the view's top-left device pixel, or row or column 0 while the margin is at that edge. Past the picture's end it shows the workspace color, and it shows transparent pixels as the canvas's checkerboard (REQ-0001-11). It shows the picture and what is pending at the pixels its put-down writes (REQ-0025-03), also where they lie outside the view, and none of the overlays of REQ-0025-05, the hover outline, the text highlight, the resize outline or the picture's shadow. During a resize drag it shows the picture as it is. It follows every scroll, every change of the picture and every frame of a drag.
- REQ-0021-08 Esc goes first to an open dialog, prompt, chooser or menu; then to the family popover or the size field (REQ-0017-14); then, with any button held in the canvas area, to that drag's own rule, or to nothing, and it never closes the card; then to a waiting curve, polygon or gradient, a selection (marked or floating) and an open box. Only then, while the card is shown, it closes the card and unchecks the item. A failed put-down has taken the Esc, and the card stays. A checked item with the card hidden is not touched by Esc. A key repeat of Esc does not close the card.
- REQ-0021-09 (Pointer note, Accessibility, 2026-11-03: a keyboard press reaches the picture under the card, REQ-0022-10.) With no drag going on, the card wins over the picture and over every handle and marker (JOD, 2026-11-03): what lies under it cannot be pressed until it is scrolled away or the card is closed. The font card lies over the thumbnail and wins. A press on the thumbnail, with any button, applies a focused size field as a press elsewhere does (REQ-0017-14) and does nothing more; moving out and releasing does nothing, and it does not count as a button held in the canvas area. The pointer over it shows the default cursor, no position and no hover outline (REQ-0006-16). The wheel, Shift+wheel, Ctrl+wheel and the pinch act as over the canvas at that point.
- REQ-0021-10 While a drag begun on the canvas goes on, the card takes no input: the position and the drag's cursor stay, and a release or the other button acts at the picture pixel under the pointer (REQ-0001-23, REQ-0010-08). A press on the card with no button held first ends a drag whose release was lost, as last shown (REQ-0013-18, REQ-0014-14), and does nothing more.
- REQ-0021-11 With a canvas area under 196 × 156 logical pixels the card is not shown (172 + 2 × 12 by 132 + 2 × 12), the item stays checked, and it shows again when the area is large enough.

### Frame time
- REQ-0021-12 (Pointer note, Preferences buttons, 2026-11-04: group Panels is on page Interface, REQ-0027-01; the row shows at once and writes at OK or Apply, REQ-0027-05, -07; without this note it is on page Appearance and writes at once.) Preferences' Appearance page, group Panels, shall hold a switch "Frame time", key `[view] frame-time` (`on`, `off`; default `off`; syntax as REQ-0001-40), applied at once. While on, the status bar shows, directly left of the pointer position (REQ-0004-09), in a part of fixed width so that the hint does not shift, the CPU time taken to build the canvas's last painted frame (JOD, 2026-11-03: nothing of the GPU or the compositor, and not the thumbnail), as `6.2 ms`, one decimal, the point as the locale writes it. The text changes at most once per 250 ms, and the last frame's value shows within 250 ms. Changing the text does not redraw the canvas or count as a measured frame. It keeps its value while nothing is painted. It shows `– ms` before the first measured frame.

### Pan's menu
- REQ-0021-13 When the user presses the secondary button with Pan in the canvas area while no button is held, the app shall open a context menu at the pointer, a GNOME popover menu as the canvas menu is (REQ-0016-14), with its sections: Paste, Paste From…, Select All; Zoom In, Zoom Out, Normal Size; Clear Image. Each item is sensitive and acts as its counterpart elsewhere (REQ-0016-14, REQ-0018-02). It does nothing before the picture exists (REQ-0001-39). Pan has no handles (REQ-0004-16): the menu opens at those places too. Select All switches to the Select tool at once. Paste and Paste From… switch to it only when the pasted picture lands (`docs/reference/new-behaviors.md`); on a cancel, a failure or a clipboard timeout (REQ-0016-15) Pan stays. Zoom and Clear Image keep Pan. Opening and closing the menu changes nothing.

### How it meets P7
- REQ-0021-15 A double click of the primary button on the empty space of the header bar, the palette bar or the status bar, on a disabled button there, or on a status bar's text, shall do nothing (JOD, 2026-11-04: a window that changes size by a stray click on a disabled Undo is a surprise). A double click on the window's title in the header bar keeps what the desktop gives it, by default maximizing. A drag on those places still moves the window, the other buttons there keep what the desktop gives them, and a press on a working control is the control's.
- REQ-0021-14 The card is a panel over the view, as the font card is, not an overlay of P7.5: it never writes a picture pixel, and the view under it is drawn as without it. It shows exact colors (P7.1), one device pixel a picture pixel (P7.2), and a preview or float by the pixels its put-down writes (P7.3). It depends only on the picture, what is pending, the view's position and the workspace color (P7.4).

## Scenarios
- AS-1 (REQ-0021-01) Given a picture, Then the primary menu ends in the sections Preferences | Keyboard Shortcuts, Help, About Easel | Quit, with their keys; Given a stroke held, the quit prompt, the color chooser, a menu, or the focus in the font card's size field, When Ctrl+?, Ctrl+F1, F1 or the items are used, Then nothing opens; Given a waiting curve or a floating selection, When each opens and closes, Then they stay and no step is added | Verified by: automated (ui) + manual M1 (the real keys, also on a Ukrainian layout)
- AS-2 (REQ-0021-02) Given the app, When Keyboard Shortcuts opens, Then every key the app has is listed in its group, each meaning once, and every listed key is one the app has; Esc closes it | Verified by: automated (ui) + manual M1 (its look)
- AS-3 (REQ-0021-03) Given Keyboard Shortcuts, About Easel or the Help error open, When Ctrl+Q, Then it closes and the quit goes on; with a modified picture the prompt shows | Verified by: automated (ui)
- AS-4 (REQ-0021-04) Given the app, When Help is chosen, Then the page opens in the browser; the page names every tool of the toolbox and every key of AS-2; held F1 opens one; Given a page that does not exist, Then the error dialog shows and the picture is as it was; Given a failure reported during a stroke, Then the dialog waits for its release; during a quit it is dropped | Verified by: automated (ui) + manual M1 (it opens)
- AS-5 (REQ-0021-05) Given About Easel, Then it shows the name, the version of the build, GPL-3.0-or-later and both acknowledgements; Esc closes it | Verified by: automated (ui)
- AS-6 (REQ-0021-06) Given 100 %, Then Show Thumbnail is insensitive; Given 400 %, When it is checked, Then the card shows; When the zoom goes to 100 %, Then it hides and the item stays checked; When 200 %, Then it shows; a restart starts it off | Verified by: automated (ui)
- AS-7 (REQ-0021-07, REQ-0021-14) Given a picture with every pixel different at 800 %, scrolled, Then each device pixel of the picture area is the picture pixel at its place from the first pixel, at 100 %, exactly; at scroll 0 the first pixel is (0, 0); at 129 % at scroll s, the first column is the pixel whose drawn square holds it, checked at s = 1 and s = 2; Given a pencil stroke held, a floating selection and an open text box, Then the area shows their pixels and none of the overlays; past the picture's end the workspace color; transparent pixels as the checkerboard; the view under the card is drawn as without it; the picture's pixels are unchanged; on a screen of scale 2 too | Verified by: automated (ui, a rendered window, also on a session of scale 2)
- AS-8 (REQ-0021-08) Given the card shown and no other state, When Esc, Then it closes and the item unchecks; Given a pending curve, a floating or marked selection, an open box, a pencil stroke held or a pan held, When Esc, Then the card stays; Given a hidden card, When Esc, Then the item stays checked; Given held Esc on a floating selection, Then the selection is put down and the card stays | Verified by: automated (ui)
- AS-9 (REQ-0021-09) Given a resize handle or the picture under the card, When a press with each button falls on the card, Then nothing is drawn, resized or opened; a press on it then a release on the canvas does nothing; When the wheel is over it, Then the view scrolls; Given the font card over it, Then the font card takes the press; Given a focused size field, When a press falls on it, Then the field is applied | Verified by: automated (ui) + manual M2 (the real mouse)
- AS-10 (REQ-0021-10) Given a drag begun on the canvas, When it passes over the card, Then it goes on, with its position and cursor; When the release falls on the card, Then the drag ends at the pixel there; When the other button is pressed over the card, Then the drag is cancelled; Given a lost release, When a press falls on the card, Then the drag ends as last shown and nothing more | Verified by: automated (ui) + manual M2
- AS-11 (REQ-0021-11) Given an area of 195 × 156 or 196 × 155, Then it is not shown and the item stays checked; at 196 × 156 it is | Verified by: automated (ui)
- AS-12 (Pointer note, Preferences buttons, 2026-11-04: the key is written at Apply, REQ-0027-07.) (REQ-0021-12) Given Frame time off, Then the status bar has no such part; When on, Then `– ms` until a frame, then a number with one decimal, at most 4 changes a second during a stroke, and the stroke's last value within 250 ms; a drawing delayed by a known time reads at least that; a change of the text adds no canvas frame; with the hint long, the hint does not shift when the value changes; the key is written; `frame-time=maybe` means off | Verified by: automated (ui) + automated (core)
- AS-14 (REQ-0021-15) Given a new picture, Then a double click is taken by the app at a disabled Undo, at the header bar's empty space, at the palette bar's empty space and at the status bar's hint; and left alone at the title, at Open, at an enabled Undo, at the Image menu, at a swatch, at the color indicator and at the swap button | Verified by: automated (ui) + manual M1 (the real double click, and a drag that still moves the window)
- AS-13 (REQ-0021-13) Given Pan, When the secondary button is pressed over the picture, over the margin and where a handle would be, Then the menu opens there with its three sections; When Select All, Then the tool is Select and all is marked; When Paste with a picture on the clipboard, Then the tool is Select and it floats; with nothing to paste the item is insensitive; When Paste From… is cancelled, or the oversize prompt is, Then Pan stays; When Zoom In or Clear Image, Then it acts and Pan stays; a press while the primary is held does nothing; opening and closing adds no step | Verified by: automated (ui)

## Edge cases
- A picture smaller than the card at 1600 %: its few pixels at the area's top-left, the workspace color around them.
- A theme change while the card is shown: the card and the workspace color follow at once.
- The Help page's text is English until Languages; the list of keys is written by hand and checked by AS-4's test.
- On a fractional desktop scale the card's device pixels are as P7.2 leaves them (`docs/phase2-plan.md`): only scales 1 and 2 are tested.

## Out of scope
- Print (its own feature); a thumbnail of the whole picture; moving or resizing the card; a Frame Time item in the View menu (0004 keeps none: JOD, 2026-11-03); translating the Help page (Languages); a Help page per tool or with pictures.

## Open questions

## Plan
**Approach.** Five independent slices, smallest first. The numbers (first pixel of the card, frame time's gate and text) are pure core; the card is a small widget in the canvas overlay beside the font card, whose pixels the canvas composes into a buffer the size of the card; Help opens through one function with a seam for its launcher.

**Finding the Help page.** The page is `data/help/index.html`, installed to `<datadir>/easel/help/index.html`; meson writes that path into `config.h`. `EASEL_HELP_PAGE`, if set, wins: the ui tests set it to the source tree's file in `ui_env`, as `EASEL_TEST_DATA` does for test data. An uninstalled build finds its copy, `build/data/help-index.html`, from its program's folder, however it was started (JOD's M1: an app started by the desktop has no variable from `meson devenv`). Missing page: the app checks that the file exists before asking the launcher, so "page not found" (REQ-0021-04) needs no browser to be seen and has its own test.

**Files.** New: `src/ui/thumbnail-card.{c,h}`, `src/ui/help.{c,h}` (the shortcuts table, its dialog, About, the page's path), `data/help/index.html`, `data/meson.build`, `tests/core/test-window-extras.c`, `src/core/frame-time.{c,h}` (its text and its pace). Changed: `src/core/view.{c,h}` (the card's first pixel), `src/core/settings.{c,h}` (`frame-time`), `src/ui/canvas.{c,h}`, `src/ui/window.{c,ui}`, `src/ui/application.c`, `src/ui/status-bar.{c,h}`, `src/ui/preferences-dialog.c`, `meson.build`, `tests/ui/test-window.c`, `tests/meson.build`, `tests/mutants.txt`.

**Key interfaces.**
- Core: `int easel_view_first_cell (double scroll_logical, double margin, int pct, double scale)`: REQ-0021-07. `bool easel_frame_time_due (int64_t now_us, int64_t last_shown_us)`: at most once per 250 ms; `int easel_frame_time_wait_ms (...)`: until then; `void easel_frame_time_format (double ms, char *out, size_t size)`.
- Ui: `GdkTexture *easel_canvas_compose_view (canvas, x0, y0, w, h)`: the picture, a float and its hole, a shape's or gradient's preview and an open box's pixels, by the code that makes them for the view, with no overlay. Signal `"painted"` of the canvas, emitted once per frame after its snapshot through an idle, which redraws the card: the card follows one frame later, and the canvas is not drawn again. `easel_status_bar_set_frame_time`. `easel_window_set_help_launcher` (test hook).
- The frame's cost is read with the thread's CPU clock around `easel_canvas_snapshot`.

**The one function.** Esc: the window's key handler, last in its existing order, so that everything that takes Esc takes it first (REQ-0021-08). Quit: `close_request_hook`, an emission hook on `close-request`, closes these dialogs and Preferences: libadwaita's handler would stop the request before `easel_window_close_request` runs.

**Readers of what this extends.**
- The status bar's field list: `EASEL_STATUS_N_FIELDS`, with `easel_status_bar_get_text`'s tests.
- The canvas's press and motion: `on_selection`, the handle hit tests and the hover; a press on the card never reaches them, since the card is a widget over the canvas with its own gesture. A drag begun on the canvas keeps its grab, which is how REQ-0021-10 holds: T006 tests it.
- The primary menu's tests: 0005's `as1_file_menu` and 0020's `as1_preferences_opens`; the View menu's: 0004's `as1_*`.
- The created file's text: AS-4 of 0001, AS-14 of 0004, AS-13 of 0020.

**What grows.** A card's buffer: 160 × 120 × scale² × 4 bytes, made for each drawn frame and dropped with the texture; 75 KB at scale 1, 300 KB at scale 2, computed. If it cannot be had the card shows the workspace color for that frame.

**Scenario map.**
| AS | Test |
|---|---|
| 1 | ui `as1_extras_in_the_menu`, `as1_shortcuts_key_by_place`, manual M1 |
| 2 | ui `as2_every_key_listed`, manual M1 |
| 3 | ui `as3_quit_closes_extras` |
| 4 | ui `as4_help`, `as4_help_page_text`, manual M1 |
| 5 | ui `as5_about` |
| 6 | ui `as6_thumbnail_item` |
| 7 | ui `as7_thumbnail_pixels`, `/0021-scale2/as7_at_scale_2`, core `as7_first_cell` |
| 8 | ui `as8_thumbnail_esc` |
| 9 | ui `as9_card_takes_presses`, manual M2 |
| 10 | ui `as10_drag_over_card`, manual M2 |
| 11 | ui `as11_small_area`, core `as11_card_fits` |
| 12 | ui `as12_frame_time`, core `as12_frame_time_text` |
| 13 | ui `as13_pan_menu`, `as13_pan_menu_paste` |

**Constitution.** Core stays libc only; the card is a panel, REQ-0021-14; no ADR. A Help page ships with the app, under the project's license.

**Risks.**
- R1 `GtkUriLauncher` refused a `file://` page here (JOD's M1, a probe: "The application launch failed"; no test can see it, the launcher being the seam); `GtkFileLauncher` asked "Open With" through the portal from a window (M1 again; the probe with no window had hidden that). `g_app_info_launch_default_for_uri_async` opened it, from a window, with no question: it opens the default application for the page, the browser while `text/html` belongs to one. Under Flatpak: checked at packaging.
- R2 `AdwShortcutsDialog` (libadwaita 1.8, present here): T003 builds with it; the minimum libadwaita goes to packaging-release.
- R3 A redraw queued from an idle after the canvas's snapshot must not ask for another canvas frame: T006 counts them.
- R4 AS-2 needs the list of keys in one place: the dialog's table, checked against the accelerators GTK holds and a list of the keys with no menu.
- R5 A press on the card with no button held and a lost release: the card's gesture calls the canvas's own function for that, tested in T007.

## Tasks
- [x] T001 Frame time: the key, its row, its part of the status bar and its measure (REQ-0021-12; AS-12) - done when core and ui tests pass, red first
  Result: `as12_frame_time_text` failed first by build (`frame-time.h: No such file`), `as12_frame_time` by build (`EASEL_STATUS_FRAME_TIME` undeclared); then green. The measure is the thread's CPU clock around the canvas's snapshot. A delay of 30 ms reads at least 30; 40 quick frames change the text at most 5 times; 400 ms with nothing painted adds no canvas frame and no write.
- [x] T002 Pan's menu (REQ-0021-13; AS-13) - done when `as13_pan_menu` passes, red first
  Result: `as13_pan_menu` failed first (`gtk_widget_get_visible (menu)` should be TRUE), then green. Select All and a landed paste already switch to Select, so only the press changed; `as13_canvas_menu` of 0016 still passes.
- [x] T003 The menu items and Keyboard Shortcuts (REQ-0021-01, -02, -03; AS-1, AS-2, AS-3) - done when they pass, red first
  Result: all failed first by build (`help.h: No such file`). A real bug came out: libadwaita's own handler stops a close request while a dialog is open, ahead of any handler of ours, so a quit with Preferences open only closed the dialog (0020's `as1_quit_closes_it` never saw it); that test now also waits for the window to go, red, and an emission hook closes these dialogs first. Ctrl+? by the key's place is `as1_shortcuts_key_by_place`. Esc is the dialog's own binding and goes to M1. Tab has its row under View; Shift and Ctrl as modifiers of a drag are not keys of this list and are in the Help page.
- [x] T004 About Easel (REQ-0021-05; AS-5) - done when it passes, red first
  Result: pulled into T003's run, `as5_about` failed first by build with it. Esc goes to M1.
- [x] T005 The Help page and how it is found and opened (REQ-0021-04; AS-4) - done when `as4_help` and `as4_help_page_text` pass, red first
  Result: `as4_help` failed first by link (`easel_window_set_help_launcher`), `as4_help_page_text` on the page's missing rows. A repeat is a second press of F1 with no release; a release lost as the window goes to the back is cleared. Installed path seen by `meson install` to a private folder.
- [x] T006 The thumbnail: first pixel, item, card, pixels, one frame's lag (REQ-0021-06, -07, -11, -14; AS-6, AS-7, AS-11) - done when they pass, also at scale 2, red first
  Result: core `as7_first_cell` and `as11_card_fits` red by build, the ui cases by build (`easel_window_get_thumbnail_card`). Scale 2 was red on the first run (a blended pixel at the card's (5, 0): `ff004800` for `ff005000`), so the texture goes in at device size, unscaled. A break proves `as7_thumbnail_pixels`: without the float's lay it fails at (4, 4), `ff004004` for `ffffffff`. The window's minimum is wider than 196, so `as11_small_area` sets the area's size where the rule reads it; the core test has the edges. The buffer is 75 KB, or 300 KB at scale 2, as planned; with no memory that frame shows the workspace color (no test: the same `g_try_malloc` path as the view's).
- [x] T007 The thumbnail's Esc, presses and drags (REQ-0021-08, -09, -10; AS-8, AS-9, AS-10) - done when they pass, red first
  Result: red by build (`easel_window_thumbnail_press`). Breaks prove them: without `!repeat` `as8_thumbnail_esc` fails at `assert_card_stays`; without the lost release's end `as10_drag_over_card` fails at `easel_canvas_is_stroking`. The grab during a real drag, Ctrl+wheel and the pinch over the card go to M2: no ui test sends real events. Also `as13_pan_menu_paste`, owed from T002: a paste that lands gives Select, one that times out leaves Pan.
- [x] T008 Manual M1 (the real keys on two layouts, the shortcuts dialog's look, Help opening, About) and M2 (the card with the real mouse: a press, a drag across it, the wheel) - done when JOD has seen them
  Result: JOD, 2026-11-03: M1 steps 1, 2, 3, 5, 6 ok and M2 steps 1 to 8 ok at the first try. M1 step 4 (Help) failed three times, each a fix: the URI launcher refuses a `file://` page; the file launcher asks "Open With" through the portal; an app started by the desktop has no variable from `meson devenv`, so the page was not found. Then JOD: "works now". JOD did not say separately whether a held F1 opens one page or how the page reads.
- [x] T009 The core mutants, and `tools/mutate.sh --core` once, in the background (REQ-0021-07, -12) - done when its summary line is here and each survivor is one line for Check
  Result: `406 mutants: 405 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 0 errors, 1 accepted in 120 s`. Nine new mutants (first pixel, room for the card, frame time's text, pace and key); no survivor.

## Check
- Date and commit: 2026-11-03, uncommitted on `8f4d043`
- Tests: `meson test` 63 of 63 pass after the fixes (62 of 63 before, F1); scale 2: 3 of 3, before the fixes, which were to test and Help text and labels
- Mutants: `tools/mutate.sh --core`: 405 killed, 0 killed by crash (test gap), 0 survived, 1 accepted (120 s); run before the Help fixes, which touch no core code
- Warnings: 0
- Scenarios: AS-1 `as1_extras_in_the_menu`, `as1_shortcuts_key_by_place`, M1; AS-2 `as2_every_key_listed`, M1; AS-3 `as3_quit_closes_extras`; AS-4 `as4_help`, `as4_help_page_text`, M1; AS-5 `as5_about`, M1; AS-6 `as6_thumbnail_item`; AS-7 `as7_thumbnail_pixels`, `as7_at_scale_2`, core `as7_first_cell`; AS-8 `as8_thumbnail_esc`; AS-9 `as9_card_takes_presses`, M2; AS-10 `as10_drag_over_card`, M2; AS-11 `as11_small_area`, core `as11_card_fits`; AS-12 `as12_frame_time`, core `as12_frame_time_text`; AS-13 `as13_pan_menu`, `as13_pan_menu_paste`. No characterization task.
- Results: see each task's `Result:` under Tasks; every task has one.
- Findings:
  - F1 (must fix) `/0013/as10_select_all_edit_menu` still expects 3 sections in the primary menu; the separators made 5. Only the test was out of date. Fixed: it expects 5.
  - F2 (user-visible) The Help page says a Magnifier press zooms in and the secondary button zooms out; REQ-0007-03 sets the chosen level with either button, or 100 % if already there. Fixed.
  - F3 (user-visible) The Help page says Esc cancels any drag; it does not for a stroke (REQ-0021-08, AS-8). Fixed: the page says which drags.
  - F4 (user-visible) Words differ from the app: "primary/secondary colors" for foreground and background, "Fit in Window" for Fit to Window, "Rotate 90° Clockwise" and "Counterclockwise" for Rotate 90° and 270°, no ellipsis on Open…, Save As…, Stretch and Skew… and Attributes…. Fixed; `as2_every_key_listed` now compares each row of a menu's action outside Text with the menu, red first.
  - F5 T008: JOD said "works now", not whether a held F1 opens one page or how the page reads. Asked again after F2 to F4; JOD answered "go", with no separate word on the held key or the page, recorded as that.
- Known gaps (cases with no test of their own, also in `docs/phase2-plan.md`): a waiting gradient on the card; a failed put-down taking Esc; Paste From… cancelled and the oversize prompt in Pan; the quit prompt, color chooser and open menu as gates of the new items; no memory for the card's buffer; a press on the card released on the canvas; the workspace color's theme change on the card; Help under Flatpak, at packaging.
- Survivors: none.
