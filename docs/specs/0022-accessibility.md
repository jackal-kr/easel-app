# 0022 Accessibility

Status: implemented
Reviewed: spec-reviewer, 2026-11-03
Depends on: 0001, 0003 (palette), 0006 and 0007 (tools, options), 0010 to 0017 (shapes, selections, menus, text box), 0018, 0019 (commands), 0021 (keys, Help), 0025 (drawn squares)

## Why
Someone who cannot use a mouse, or cannot see the screen, should still be able to draw and to work the app (`docs/design.md`). Each earlier feature left its keyboard path and its screen-reader names to this one.

How it meets P7: a keyboard press acts on whole picture pixels and by the tool's own rules, so it writes exactly what a mouse would at those pixels, at any zoom (P7.1, P7.3, P7.4, P7.6). The marker is an overlay like those of REQ-0025-05: it never writes a picture pixel and covers only its own two rings (P7.2, P7.5).

## Requirements
### Moving between panels
- REQ-0022-01 The window's panels shall be, in order: toolbox, tool options, canvas, palette, each one stop of Tab and Shift+Tab, the header bar's controls in their own stops before the toolbox and the status bar having none. F6 and Shift+F6 shall move to the next and the previous of these, the header bar counting as one. A hidden or empty panel is skipped. A panel's focus goes to the control it last had; the first time, or if that control is gone, to the chosen tool, the chosen option or swatch 1. No key here changes the tool, a color or the picture. While a button is held (REQ-0022-08), Tab, Shift+Tab, F6 and Shift+F6 do nothing.
- REQ-0022-02 With the focus in the toolbox or the tool options, the arrow keys shall move the focus among the controls, by their rows and columns, stopping at the ends, also while a selection exists (JOD, 2026-11-03), and Space shall choose the focused control, as a click does; Enter too, unless something waits for it (REQ-0011-12, REQ-0012-11, REQ-0013-11), which it then acts on. Choosing a tool leaves the focus in the toolbox. A focused control shows the theme's focus ring. A press on the canvas takes the keyboard to the canvas (`docs/design.md`), so that the arrows then move a selection, whatever was clicked before.
- REQ-0022-03 With the focus in the palette, the arrow keys shall move among the swatches, by rows and columns, stopping at the ends, also while a selection exists; Space shall make the focused swatch the foreground color, Shift+Space the background color, Enter and Shift+Enter the same unless something waits for Enter; F2 edits it (REQ-0003-11). The two color squares and the swap button are one stop each before the swatches: Space opens the chooser or swaps (REQ-0003-13).
- REQ-0022-04 While a text box is open, Tab, Shift+Tab and F6 shall move among the panels, passing the font card, and the keys of REQ-0022-02 and -03 act on the panel with the focus: Space or Enter on a swatch is a color pick for the text (REQ-0017-13). With the focus back on the canvas, keys go to the box (REQ-0017-06). A typed Tab is not text (JOD, 2026-11-03).

### The keyboard pointer
- REQ-0022-05 The app shall show a marker around one picture pixel after Tab or F6 brings the focus to the canvas, or after a key of REQ-0022-06, -07, -09 or -13 with the focus there (JOD, 2026-11-03: a mouse user never sees it). A mouse press or move on the canvas, or the focus leaving, hides it; the mouse puts it at the pixel under it, unless a keyboard press is held. It is one device pixel of black just outside the pixel's drawn square (REQ-0025-02), and one device pixel of white just outside that; below 100 %, where a pixel has no square of its own, around the device pixel that shows it. Only those two rings cover anything. It lies over the picture, the grid, the tint, a preview and the handles, and under the thumbnail and the font card. It starts at the picture pixel at the view's top left (REQ-0021-07), and New and Open put it there again; a transform or a resize keeps its numbers, kept inside the picture.
- REQ-0022-06 With the focus on the canvas and no box open, Space shall act as a press of the primary button at the marker's pixel, Shift+Space of the secondary, Ctrl+Space of the primary with Ctrl (a copy, REQ-0013-09), Ctrl+Shift+Space of the primary with Shift at the press only (a smear, REQ-0014-11). While a press is held, Space with any modifiers releases it, whichever button holds it, the Shift really held counting at that release. A key repeat of these does nothing. Before the press the view scrolls by the least to show the marker.
- REQ-0022-07 A keyboard press shall act on the pixel only: it never takes a picture handle, a selection handle or a box handle or band, whatever the zoom. It takes a waiting gradient's marker when the marker's pixel is that end. It is never part of a double click: a polygon closes by Enter or by the first-corner rule (REQ-0011-07). Fill and the Magnifier act at the one press and hold nothing. In Pan, Space does nothing. A press that opens a menu holds nothing. Airbrush sprays from the press until the release.
- REQ-0022-08 With the focus on the canvas, the arrow keys shall move the marker by 1 picture pixel and Shift with an arrow by 10, stopping at the picture's edges, each key repeat moving again; while a press is held that is a move of the mouse to that pixel. With no press held and a selection there, the arrows move the selection as REQ-0013-10 says, and the marker moves with it, kept inside the picture. Alt with an arrow, and Alt+Shift by 10, move the marker alone while a selection exists. The view scrolls by the least to show the marker's square. In Pan the arrows do the same, so the view follows.
- REQ-0022-09 With the focus on the canvas, the Menu key and Shift+F10 shall open at the marker's pixel the menu a secondary press there opens (REQ-0013-17, REQ-0016-14, REQ-0021-13), at the caret with a box open (REQ-0017-22), and nothing where none opens or while a press is held. The focus returns to the canvas when it closes.
- REQ-0022-10 A keyboard press held counts as a button held in every rule that names one (REQ-0021-08 too), and all commands, keys and options act as during a mouse drag of that tool: Undo by its rule (REQ-0010-08, REQ-0013-18); Esc cancels a keyboard press with any tool, as the other button cancels a mouse drag, since the keyboard has no other button (JOD, 2026-11-04); Save and Quit end it as last shown (REQ-0001-33, REQ-0010-18), the window's focus lost ends it as last shown (REQ-0001-25). After any such end the next Space is a new press. The rules that say the arrows do nothing during a drag are about the mouse (REQ-0013-18, REQ-0014-14, REQ-0017-04). REQ-0007-18 does not apply: a move of the mouse does nothing to a keyboard press or the marker, a primary or secondary mouse press cancels it as the other button does and does nothing more, and a middle press nothing. With a mouse button held, the keys of REQ-0022-06, -08 and -09 do nothing, F7 still answers. A scroll leaves the marker on its picture pixel. A keyboard press reaches the picture also under the thumbnail or the font card (REQ-0021-09).
- REQ-0022-11 While the marker shows, the status bar's position (REQ-0004-09) shall show its pixel, and the footprint outline (REQ-0006-16) and the Magnifier's box (REQ-0007-04) lie there; while a keyboard press is held the hint reads "Space to finish, Esc to cancel".
- REQ-0022-12 Handles shall have a keyboard path by commands: a selection's size through Stretch and Skew (REQ-0019-06), the picture's through Attributes (REQ-0004-21). While a text box is open, Alt with an arrow shall move it by 1 picture pixel as a drag of its band (REQ-0017-09) and Alt+Shift with an arrow move its far corner by 1 as a drag of that corner (REQ-0017-10), one edit each (REQ-0017-11), stopping as a drag would. Not reached by keyboard (accepted, JOD, 2026-11-03): a shape's or gradient's end beyond the picture, a stroke begun outside it, and the Ctrl stretch that leaves a copy (REQ-0015-04).

### What a screen reader hears
- REQ-0022-13 F7, with the focus on the canvas, shall announce the marker's place and color: "X, Y: #RRGGBBAA", the color a put-down would write there (REQ-0025-03), also during a press and with a box open. A key repeat does nothing.
- REQ-0022-14 Every control shall have a name, a role and its state: each tool by its tooltip's name, and whether chosen; each option by its tooltip; each swatch as "Color N, #RRGGBB", or "#RRGGBBAA" when not opaque; the two color squares as "Foreground color, #RRGGBB" and "Background color, #RRGGBB"; the canvas as an image named "Picture, W by H pixels"; an open box as a text entry named "Text box" with its text; the font card's controls by their tooltips; the status bar's fields by what they show; the thumbnail as "Thumbnail".
- REQ-0022-15 The app shall announce, once each and not interrupting what is being spoken, in whole translatable sentences: the tool chosen ("Pencil"); a color set ("Foreground #RRGGBB", "Background #RRGGBB", with AA when not opaque), Pick Color's at the release only; the zoom after it changes ("400 %"); a selection made ("Selection W by H"), put down ("Selection put down") and removed ("Selection removed"); Undo and Redo ("Undone", "Redone"); a keyboard press, release and cancel ("Drawing", "Done", "Cancelled"); a polygon closed ("Polygon closed"); a box opened and closed ("Text box open", "Text box closed"); a picture saved or opened ("Saved NAME", "Opened NAME"). It does not announce each move of the marker, nor anything of the mouse's moves.

### Keys and help
- REQ-0022-16 Every key here shall be in Keyboard Shortcuts (REQ-0021-02), the palette's in Colors, Alt with an arrow in Text and in Tools, F6 in View, the rest in Tools, each meaning once, and on the Help page, which shall have a part "Using only the keyboard" (REQ-0021-04).

## Scenarios
- AS-1 (REQ-0022-01) Given the app at start, When Tab is pressed repeatedly, Then the focus goes through the header bar's controls, toolbox, options, canvas and palette, and Shift+Tab goes back; When F6, Then the next panel; Given Pan, Then the options are skipped; Given the Tool Box hidden, Then it is skipped; When Brush's tip was focused and the tool is then Line, Then the options start at the chosen width; Given Pencil held by Space, When Tab or F6, Then the focus stays | Verified by: automated (ui) + manual M1 (the real keys, the focus ring)
- AS-2 (REQ-0022-02) Given the toolbox focused on Pan, When Right, Then Magnifier is focused and the tool is Pan; When Space, Then the tool is Magnifier and the focus stays; at the ends the arrows stop; Given a selection, Then the arrows still move the focus and it stays still; Given a waiting curve, When Enter, Then it is put down and the focused tool is not chosen; When Space, Then chosen, as REQ-0011-12 says | Verified by: automated (ui)
- AS-3 (REQ-0022-03) Given swatch 1 focused, When Right then Space, Then swatch 2's color is the foreground; When Shift+Space, Then the background; the end stops the arrows; F2 opens the chooser; Space on the swap button swaps | Verified by: automated (ui)
- AS-4 (REQ-0022-04) Given an open box, When Tab or F6 repeatedly, Then the panels get the focus and the font card none; When Space on a swatch, Then the text's color changes as a pick does; When typed letters with the focus back on the canvas, Then they go to the box; a typed Tab adds no text | Verified by: automated (ui)
- AS-5 (REQ-0022-05) Given a mouse user, Then no marker; Given a picture at 800 %, scrolled, When Tab reaches the canvas, Then the view has not moved and the marker's two rings lie just outside the top left pixel's square, a cut-off square too, and every other device pixel is as before; at 129 %; at 100 %; at 50 % and 10 % around one device pixel; at scale 2; When a mouse move, Then it is hidden and takes that pixel; the picture, undo list, modified mark and a saved file are unchanged by it; New puts it back; a smaller picture keeps it inside | Verified by: automated (ui, a rendered window, also at scale 2)
- AS-6 (REQ-0022-06, REQ-0022-08) Given Pencil on white, When Space, Right × 3, Space, Then 4 pixels are drawn in the foreground, one undo step, equal to the mouse's drag; When Shift+Space and the same, Then in the background; Given Brush, Eraser, Line, Curve, Polygon, Rectangle, Rounded Rectangle, Ellipse, Gradient, Select, Free-Form Select and Text, When driven the same way, Then the picture or the selection equals the mouse's; Given Line, When Space, Right × 5, Down × 3, Shift+Space, Then the constrained line; a key repeat does nothing; the edge stops the marker | Verified by: automated (ui)
- AS-7 (REQ-0022-07) Given a picture at 100 % and 800 % with the marker beside a resize handle or a selection handle, When Space, Right, Space, Then the same pixels at both and no resize; Given Polygon, When presses are at any speed at one pixel, Then corners are added and Enter closes; Given Fill, When Space, Then filled and Tab works at once; Given the Magnifier, Then the zoom changes at once; Given Pick Color, When Space, arrows, Space, Then the color follows and the tool returns (REQ-0007-09); Given Airbrush, When Space, a wait, Space, Then sprayed pixels lie in the disc, one step, and spraying stops; Given Pan, When Space, Then nothing; Given Pan, When Shift+Space, Then the menu, Esc, and Space is nothing held | Verified by: automated (ui)
- AS-8 (REQ-0022-08) Given a marked selection, When an arrow, Then it floats and moves and the marker moves with it, also held at the picture's edge inside it; When Ctrl+Space, arrows and Space on it, Then a copy moves; When Ctrl+Shift+Space, Then a smear; When Alt+Right, Then only the marker moves, and Space off it starts as a press outside does; Given the marker at the view's right edge, When Right, Then the view scrolls by the least, also while a stroke is held, and in Pan | Verified by: automated (ui, a rendered window)
- AS-9 (REQ-0022-09) Given Pan, When the Menu key, Then Pan's menu opens at the marker, and the focus returns on close; Given a selection under it, Then the selection's; Given an open box, Then its menu at the caret; Given a press held, Then nothing | Verified by: automated (ui)
- AS-10 (REQ-0022-10) Given a shape, or a Pencil, Brush, Airbrush or Eraser stroke, held by Space, When Esc, Then it is cancelled, no step is added, and the next Space a new press; When Undo, Then cancelled; When the window loses focus, or Save, or Quit, Then it ends as last shown; When Ctrl+plus, Then no zoom; When a mouse move, Then nothing; When a mouse press, Then cancelled and that press does nothing more; Given a mouse drag, When Space, Then nothing; Given a held stroke and a scroll, Then the marker stays on its pixel; Given the marker under the thumbnail, When Space, Then it draws; Given a held press and a card shown, When Esc, Then the card stays | Verified by: automated (ui)
- AS-11 (REQ-0022-11) Given the marker showing, Then the position is its pixel, the Brush's outline and the Magnifier's box lie there; When held, Then the hint reads as written; When the marker hides, Then all follow the mouse | Verified by: automated (ui)
- AS-12 (REQ-0022-12) Given an open box, When Alt+Right, Then it moves by 1, one edit; When Alt+Shift+Down, Then it grows by 1; at its limits it stops; Given a selection, When Stretch and Skew, Then the size changes | Verified by: automated (ui)
- AS-13 (REQ-0022-13) Given the marker on a pixel of #FF0000FF at (3, 4), When F7, Then "3, 4: #FF0000FF"; Given a float over it, Then the float's color; a held press still answers; a repeat says nothing more | Verified by: automated (ui)
- AS-14 (REQ-0022-14) Given the window, Then every control has a name and role, swatches and color squares as written, a chosen tool says so, and a swatch's name follows its color | Verified by: automated (ui) + manual M1 (Orca reads them)
- AS-15 (REQ-0022-15) Given each event listed, Then one announcement of the text written; Given a marker moved or a mouse stroke, Then none; Given Pick Color dragged, Then one at the release | Verified by: automated (ui) + manual M1 (Orca)
- AS-16 (REQ-0022-16) Given Keyboard Shortcuts and the Help page, Then each key is there and the part "Using only the keyboard" | Verified by: automated (ui) + manual M1 (every new key reaches the app on this desktop with its input methods: Ctrl+Space and Alt+Shift can be taken)

## Edge cases
- A one-pixel picture: the marker lies around it, and the arrows stop at once.
- A square wider than the view at 1600 %: the least scroll shows its top left corner part.
- The marker under the thumbnail or the font card is covered; scrolling does not go around them, and a press still acts.
- Below 100 % a step of one picture pixel may not move the marker on the screen; F7 and the position tell.
- A fractional desktop scale: the marker lies as the drawn edges do, with no test (as 0025, `docs/phase2-plan.md`).
- Stretch and Skew anchors a selection at its own rule; opposite handles are reached with arrow moves after it, which the Help page says.

## Out of scope
- Letter keys for tools (JOD, 2026-11-03: Tab and arrows); speaking each marker step; a high-contrast theme of our own (the desktop's applies); a keyboard way to drag the handles themselves (commands stand in, REQ-0022-12); what a pointer does outside the picture (REQ-0022-12); braille beyond what the screen reader gives.

## Open questions

## Plan
**Approach.** Two parts, each task testable alone (JOD, 2026-11-03). Part A: focus order, names, roles and states, announcements. Part B: the keyboard pointer. The window's key handler owns the panels' keys and the keyboard pointer's keys; the canvas owns the marker and the keyboard press, which enters the same press, move and release as the mouse, with a flag that turns off the hit tests of handles and the counting of clicks.

**Files.** New: `tests/ui` cases `/0022`. Changed: `src/ui/window.{c,ui}`, `src/ui/canvas.{c,h}`, `src/ui/palette-bar.{c,h}`, `src/ui/tool-options.c`, `src/ui/status-bar.c`, `src/ui/font-card.c`, `src/ui/help.c`, `data/help/index.html`, `tests/ui/test-window.c`, `tests/meson.build`, `tests/mutants.txt`.

**Key interfaces.**
- Window: `panel_stops` (the list Tab and F6 walk), `panel_move_focus (panel, direction)` (the nearest focusable control by bounds, stopping at the ends), `easel_window_announce (self, text)` with `easel_window_take_announcements` for tests.
- Palette: two focusable children of the color indicator, one for each square, that take no pointer, so that each has a focus stop and a name.
- Canvas: `easel_canvas_marker_show`, `_marker_move (dx, dy)`, `_key_press (button, ctrl, shift)`, `_key_release`, `_is_key_held`, `_pixel_as_put_down (x, y)` (what `easel_canvas_compose_view` makes for one pixel).

**The one function.** The panels' and pointer's keys: `on_key_pressed`, before REQ-0013-10's arrows and the box's keys. A keyboard press: `easel_canvas_key_press`. A press by mouse during a keyboard hold: `on_event`.

**A text box and the panels.** Keys go to the box unless the focus came to a panel by Tab or F6 (`panel_focus_by_key`): a click on a swatch with a box open must leave typing as it is (REQ-0017-06).

**Readers of what this extends.**
- REQ-0013-10's arrows: `on_key_pressed`, 0013's AS-5 test.
- `update_cursor` and the hover: mouse only; the marker's overlay draws the footprint at its place.
- `easel_canvas_handle_at`, `easel_canvas_selection_handle_at`, `text_hit`, `marker_hit`: return nothing during a keyboard press.
- `easel_canvas_buttons_released` (REQ-0007-18): skipped during a keyboard hold.
- Tests that count controls or focus: `ui-0003`, `ui-0006`, `ui-0007`, `ui-0013`, `ui-0017` run with each task of Part A.

**What grows.** Nothing kept: the marker is drawn from two numbers; the footprint's image is made per frame while shown.

**Scenario map.**
| AS | Test |
|---|---|
| 1 | ui `as1_panels`, manual M1 |
| 2 | ui `as2_toolbox_keys` |
| 3 | ui `as3_palette_keys` |
| 4 | ui `as4_box_and_panels` |
| 5 | ui `as5_marker`, `as5_at_scale_2` |
| 6 | ui `as6_keyboard_draws` |
| 7 | ui `as7_press_rules` |
| 8 | ui `as8_selection_and_marker` |
| 9 | ui `as9_menu_key` |
| 10 | ui `as10_hold_rules` |
| 11 | ui `as11_follows_marker` |
| 12 | ui `as12_box_keys` |
| 13 | ui `as13_read_pixel` |
| 14 | ui `as14_names`, manual M1 |
| 15 | ui `as15_announcements`, manual M1 |
| 16 | ui `as16_keys_listed`, manual M1 |

**Constitution.** P7 as Why says; no ADR; core unchanged except what a core mutant needs.

**Risks.**
- R1 `gtk_test_accessible_check_property` reads a name in a test, and `gtk_accessible_announce` is in GTK 4.14: T001's probe, M1 with Orca.
- R2 An open box as text to a screen reader: a small unseen child with role text box and its value; M1 with Orca says whether it reads, and a gap goes to `docs/phase2-plan.md` if not.
- R3 Ctrl+Space and Alt+Shift may be taken by an input method: M1 reports them to JOD.
- R4 Scale 2 for the marker: `as5_at_scale_2`.

## Tasks
Part A
- [x] T001 Names, roles and states (REQ-0022-14; AS-14) - done when `as14_names` passes, red first
  Result: `as14_names` failed first by build (`easel_palette_bar_get_square`), then at the first read: GTK makes no accessible context under `GTK_A11Y=none`, so `ui-0022` runs under `test` (R1). The color squares got unseen children for a stop and a name each.
- [x] T002 Announcements (REQ-0022-15; AS-15) - done when `as15_announcements` passes, red first, for what Part A can already raise
  Result: `as15_announcements` failed first by build (`easel_window_take_announcements`). GTK's test backend crashes on an announcement (GTK 4.24), so a real announcement is skipped under it, and the log is what tests read; M1 hears the real one.
- [x] T003 Panels: Tab, F6, stops, the first and last focus (REQ-0022-01; AS-1) - done when `as1_panels` passes, red first
  Result: `as1_panels` failed first (`guard < 20`, Tab walked the whole window). The title buttons are not stops: they refuse a grab.
- [x] T004 Toolbox and options keys, and 0013's change shown old/new (REQ-0022-02; AS-2) - done when `as2_toolbox_keys` passes, red first
  Result: `as2_toolbox_keys` failed first (the arrows moved nothing). A press on the canvas now takes the keyboard there, or a tool clicked and a selection drawn would have left the arrows in the toolbox; 0013's AS-5, `ui-0013` and `ui-0014` showed this.
- [x] T005 Palette keys, squares and swap (REQ-0022-03; AS-3) - done when `as3_palette_keys` passes, red first
  Result: `as3_palette_keys` failed first at the swatch. A button's `activate` waits on a timer, so Space emits `clicked`.
- [x] T006 A box and the panels (REQ-0022-04; AS-4) - done when `as4_box_and_panels` passes, red first
  Result: `as4_box_and_panels` failed first at the swatch; a typed key after a click on a swatch still goes to the box, a focus by Tab takes the panel's keys.
Part B
- [x] T007 The marker (REQ-0022-05; AS-5) - done when `as5_marker` and `as5_at_scale_2` pass, red first
  Result: `as5_marker` failed first by build (`easel_canvas_marker_show`). Rings exact at 800, 129, 100, 50 and 10 %, every other device pixel as before; `as5_at_scale_2` green too.
- [x] T008 Space, the arrows, the press's rules (REQ-0022-06, -07, -08; AS-6, AS-7, AS-8) - done when they pass, red first
  Result: the three failed first by build (`easel_canvas_is_key_held`). The mouse in `as6_keyboard_draws` had to take the keys' way pixel by pixel for the freehand tools. Breaks are in `tests/mutants.txt` for the phase gate.
- [x] T009 The hold's rules, the menu key (REQ-0022-09, -10; AS-9, AS-10) - done when they pass, red first
  Result: both failed first by build (`easel_canvas_key_hold_event`). The menu of a box opens at the caret.
- [x] T010 What follows the marker, the box's keys, F7 (REQ-0022-11, -12, -13; AS-11, AS-12, AS-13) - done when they pass, red first; the announcements of Part B are added here
  Result: failed first by build (`easel_canvas_get_magnifier_box`), `as13_read_pixel` at its expectations. The hover of the position and the Magnifier goes through the same pointer function, marked as a keyboard press; the footprint is drawn as an overlay, since the mouse has it as a cursor.
- [x] T011 Keyboard Shortcuts and the Help page (REQ-0022-16; AS-16) - done when `as16_keys_listed` passes, red first
  Result: `as16_keys_listed` failed first (`View: no row for F6 <Shift>F6`).
- [x] T012 Manual M1 (Orca, every new key, what does not reach the app, the focus after each dialog) and M2 (drawing a picture with no mouse) - done when JOD has seen them
  Result: JOD, 2026-11-04. M1: Tab, F6, panels, keys and focus after dialogs ok; every new key reached the app, Ctrl+Space and Alt+Shift+arrows included. A focused swatch showed no ring: fixed with a ring inside it, `as3_palette_keys` red first. Orca could not be run (not a priority, JOD), so the names were read only by the tests' backend and no announcement has been heard, and whether a box's text is read is unknown. M2: steps 1 to 7 ok, except that Esc did not cancel a keyboard stroke of Pencil: JOD agreed that Esc cancels any keyboard press, `as10_hold_rules` red first; JOD has not tried that change by hand.
- [x] T013 `tools/mutate.sh --core` once, in the background - done when its summary line is here
  Result: `417 mutants: 416 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 0 errors, 1 accepted in 173 s`. No core rule changed in this spec; three ui mutants are added for the phase gate.

## Check
- Date and commit: 2026-11-04, uncommitted on `3443557`
- Tests: `meson test` 67 of 67 pass, before the fixes of F1 and F2, after which `ui-0003` and `ui-0022` pass; scale 2: 4 of 4
- Mutants: `tools/mutate.sh --core`: 416 killed, 0 killed by crash (test gap), 0 survived, 1 accepted (173 s); no core file changed in this spec, and the four ui mutants added run at the phase gate
- Warnings: 0
- Scenarios: AS-1 `as1_panels`, M1; AS-2 `as2_toolbox_keys`; AS-3 `as3_palette_keys`; AS-4 `as4_box_and_panels`; AS-5 `as5_marker`, `as5_at_scale_2`; AS-6 `as6_keyboard_draws`, M2; AS-7 `as7_press_rules`, M2; AS-8 `as8_selection_and_marker`, M2; AS-9 `as9_menu_key`; AS-10 `as10_hold_rules`, M2; AS-11 `as11_follows_marker`, M2; AS-12 `as12_box_keys`, M2; AS-13 `as13_read_pixel`; AS-14 `as14_names`; AS-15 `as15_announcements`; AS-16 `as16_keys_listed`. No characterization task.
- Results: see each task's `Result:` under Tasks; every task has one.
- Findings:
  - F1 (user-visible) `panel_key` has no key repeat rule: Space held on the swap button swaps again at each repeat, so the colors ended up by chance. Fixed: a held key chooses once, `as3_palette_keys` red first.
  - F2 (drift) REQ-0022-05 shows the marker after F7 and the Menu key too; the code shows it only for Space and the arrows, so F7 could speak a place nothing showed. Fixed: `as13_read_pixel` and `as9_menu_key` red first.
  - F3 (drift) `docs/specs/INDEX.md`'s row of 0022 had no implemented date. Fixed.
- Known gaps (no test of their own, also in `docs/phase2-plan.md`): Orca (as recorded); "Opened NAME"; a gradient's marker taken on its own pixel by a keyboard press; a key before any placing of the marker, when a mouse press outside the picture gave the canvas the focus (the marker starts at pixel 0, 0, not at the view's corner); a selection move refused by memory, where the marker still moves; the status bar's fields' names, which are their labels.
- Survivors: none.
