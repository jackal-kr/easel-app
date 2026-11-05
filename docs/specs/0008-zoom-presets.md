# 0008 Zoom Presets

Status: implemented
Depends on: 0001, 0004, 0007

## Why
The Magnifier's options and the zoom commands show two different lists, and the View menu cannot jump to a zoom level at
all, only step. JOD's screencast of 2026-10-18 showed the gap: reaching 8x meant the Magnifier or many steps. One list of
zoom presets, shown in the Magnifier's options and in a View > Zoom submenu, with a check mark on the current one, makes
the same levels reachable and visible in both places.

## Requirements
- REQ-0008-01 The app shall have one list of zoom presets, in percent and in this order: 100, 200, 400, 600, 800, 1600 (1600: JOD, 2026-10-27). 10, 25, 50 and 300 % are zoom steps (Zoom In and Zoom Out, Ctrl+wheel, the pinch: REQ-0001-16, REQ-0004-04, REQ-0004-08) and Fit to Window can give any whole percent (REQ-0004-07), but none of these are presets. (Amended 2026-10-18 by JOD after M1 step 1; the first version also had 10, 25 and 50 %.)
- REQ-0008-02 The Magnifier's options (REQ-0007-02) shall show the presets as six buttons labelled "1x", "2x", "4x", "6x", "8x" and "16x", in two columns row by row (1x 2x / 4x 6x / 8x 16x), with the tooltips "100 %", "200 %", "400 %", "600 %", "800 %" and "1600 %"; this replaces the eight buttons of REQ-0007-02. Choosing one still only selects the Magnifier's level (REQ-0007-02).
- REQ-0008-03 The View menu shall hold, between the zoom row's group and Normal Size, a submenu "Zoom" that holds the presets only, one item each, in the order of REQ-0008-01, labelled "100 %", "200 %", "400 %", "600 %", "800 %" and "1600 %". Normal Size, Large Size and Fit to Window stay where they are in the menu, with their shortcuts (REQ-0004-05, REQ-0004-07). The label `100%` of the zoom row stays a label and does not open the list (this amends REQ-0004-01, the list of the View menu's items).
- REQ-0008-04 When the user chooses a preset in the Zoom submenu, the app shall set the zoom to it, with any tool active, keeping the point at the center of the canvas area still as far as the view's edges allow (REQ-0004-06). It does nothing where REQ-0004-03 ignores the View actions (before the picture exists, while a dialog is open, during a stroke or a resize drag). It does not change the level selected in the Magnifier's options.
- REQ-0008-05 The Zoom submenu shall show a check mark on the item whose preset equals the current zoom, and no check mark when the zoom is not a preset (at 10, 25, 50 or 300 %, or a Fit to Window zoom such as 42 %). The mark follows every change of zoom, however it came: the submenu, the zoom row, Normal and Large Size, Fit to Window, the wheel, the pinch, the Magnifier.
- REQ-0008-06 Zoom In and Zoom Out (REQ-0004-04) shall keep their own step list, 10, 25, 50, 100, 200, 300, 400, 600, 800 and 1600 %; every preset is a step, so that the steps reach every preset.

- REQ-0008-07 The toolbox shall have the same width for every tool, whether the tool has options or not: choosing another tool never changes it. At the default text size the width is 87 pixels, that of the two-column grid of tool buttons (reference measurements from JOD, 2026-10-18). To keep every tool's options inside it, the Brush, Airbrush and Eraser presets shall be compact: the same shapes, sizes, tooltips, order and two columns as REQ-0006-04, REQ-0006-07 and REQ-0006-10, with less padding. The Magnifier's five level buttons shall be laid out as the tool buttons are: two columns lined up under the two tool columns, each button as wide as a tool button, with the same gaps, so that the group is centered in the toolbox; their labels are at the normal text size (amended 2026-10-18 by JOD after M1 step 1). At a larger text size the width is that of the widest content the toolbox then holds (the labels grow, the tool buttons do not), still the same for every tool, and no label is cut (this amends the look of the presets of 0006 and the buttons of REQ-0007-02 only).

## Scenarios
- AS-1 (REQ-0008-01, REQ-0008-02, REQ-0008-03) Given the app just started, When the user opens View > Zoom, Then the items are 100 %, 200 %, 400 %, 600 %, 800 % and 1600 %, and the Magnifier's options, when the Magnifier is chosen, show 1x 2x / 4x 6x / 8x 16x with the tooltips 100 % to 1600 %: the same six levels | Verified by: automated (ui)
- AS-2 (REQ-0008-04) Given a 3000 × 2000 picture at 100 % with Pencil active and a canvas area of 1390 × 853, When the user chooses 800 % in View > Zoom, Then the zoom is 800 % and the picture point that was at the center of the canvas area is still there; When the user chooses 100 %, Then the zoom is 100 %; the choice in the Magnifier's options is unchanged, and no pixel, undo step or modified mark changed | Verified by: automated (ui)
- AS-3 (REQ-0008-05) Given 100 %, Then 100 % is checked and only it; When the user presses Ctrl+PageDown (400 %), Then 400 % is checked; When the user zooms in once from 400 %, Then 600 % is checked; When the user zooms out from 400 % (to 300 %), Then no item is checked; When the user zooms out to 50 % and to 25 % with Zoom Out, Then no item is checked; Given Fit to Window gave 42 %, Then no item is checked; When the user then presses the Magnifier on the picture with 4x chosen, Then 400 % is checked | Verified by: automated (ui)
- AS-4 (REQ-0008-04) Given the color chooser open, a stroke in progress or a window whose picture does not exist yet, When the user chooses a preset, Then nothing changes | Verified by: automated (ui)
- AS-5 (REQ-0008-06) From 100 %, Zoom In gives 200, 300, 400, 600, 800, 1600 %: every preset in order, 300 % the only step between them that is not a preset; 10, 25 and 50 % are steps below the first preset; zooming out from 400 % gives 300 %, then 200 % | Verified by: automated (core)
- AS-6 (REQ-0008-07) Given the app at the default text size, When the user chooses each tool in turn (Pan, Pencil, Brush, Airbrush, Eraser, Magnifier, Pick Color, Fill), Then the toolbox is 87 pixels wide each time, and every preset of the Brush, Airbrush and Eraser and the eight level buttons of the Magnifier are fully shown; Given the text size at 150 % and at 200 %, Then the toolbox is the same width for every tool at each size, wider than 87 pixels only as far as its widest content needs, and no label is cut; With the Magnifier at the default text size, the left edges of 1x, 4x and 8x line up with the Pan button's left edge (within 1 pixel), the left edges of 2x and 6x with the Magnifier tool button's, each level button is as wide as a tool button, and the label is at the normal text size | Verified by: automated (ui, `as6_toolbox_width_is_the_same_for_every_tool`, which measures the toolbox after each choice at three text sizes) + manual M1 (the look of the compact presets and buttons)

## Edge cases
- A zoom that is not a preset (42 %, 300 %): no check mark; choosing any preset still works.
- Choosing the preset that is already the zoom changes nothing (no scroll, no redraw needed).
- The submenu is open while the zoom changes by the wheel or a shortcut: not possible with the menu's popover modal, so no rule.
- At 1600 % Zoom In is insensitive (REQ-0004-02) and the 1600 % item is still shown, checked.

## Out of scope
- A Custom Zoom… entry or any list the user edits (Preferences).
- Keyboard shortcuts for single presets.
- A margin around a small picture and the workspace color (the view polish feature).
- Changing the Magnifier's levels or the zoom steps (only the check that every preset is a step is new).

## Open questions
None. Settled by JOD on 2026-10-18: the items are labelled with the percent ("100 %" … "800 %" since the five-preset amendment of the same day); the submenu holds the presets only, and Normal Size, Large Size and Fit to Window stay where they are; the `100%` label of the zoom row does not open the list. The submenu's place among the View menu's items (after the zoom row, before Normal Size) was the draft's choice and is confirmed by the acceptance of the spec.

## Plan
### Approach
One list of presets lives in the core view code; everything else is built from it. The Magnifier's options already read
`easel_magnifier_levels`: that array becomes the preset list (one definition, in `view.c`, with `easel_magnifier_levels`
and the level count as aliases of it), so the lens cannot drift from the menu. The View > Zoom submenu is built at start-up
from the same list, in code, and inserted into the View menu's second section before Normal Size; its items are one
stateful action, `win.zoom-to`, whose parameter is the percent and whose state is the current zoom when it is a preset and
0 (no item matches) when it is not, so GTK shows the check mark on the matching item by itself. Choosing an item calls the
existing `easel_canvas_set_zoom`, the path every zoom command already uses (center kept still, REQ-0004-06; ignored while
`view_actions_allowed` is false, REQ-0004-03), so no zoom math is new. `sync_zoom_ui` already runs at every zoom change and
sets the action's state, so the mark follows the wheel, the pinch, the Magnifier, Fit and the shortcuts without a copy.
The toolbox width comes from the layout, not from measuring (T006, JOD's option B): the option groups are the pages of a
stack whose width is that of its widest page and whose height is that of the shown page, with an empty page for the tools
without options; the Brush, Airbrush and Eraser groups are made compact (a `compact` style class: small padding, a 4 px
margin and a 2 px gap) so that each fits the 86 px of the tool grid at the default text size, and the Magnifier's levels
take the tool grid's own margins and gaps, two equal columns and a `level` style class (narrow side padding, the label at
the normal size), so that they line up under the tool buttons (the lens amendment of REQ-0008-07). The options widget stays
shown; for a tool without options the empty page shows and the line above the options is hidden, so nothing of the options
is visible and they take no height (REQ-0006-02 is unchanged for the user). No threads, no settings, no new dependencies; the app's first CSS (a small
style sheet in the gresource, `style.css`, which AdwApplication loads by itself from the app's resource path) carries the
compact and level looks.

### Files
- `src/core/view.{c,h}`: `easel_view_presets[]`, `EASEL_VIEW_N_PRESETS`, `easel_view_preset_index`; `src/core/magnifier.{c,h}`:
  the levels become aliases of the presets. Tests: `tests/core/test-view.c` (or the file the zoom steps are tested in).
- `src/ui/window.c`: the `zoom-to` and `magnifier-level` actions, the submenu built from the presets, the state in
  `sync_zoom_ui`. `src/ui/tool-options.c`: the options stack with its empty page, compact preset buttons and grids, the
  lens buttons from the presets on `win.magnifier-level`. `src/ui/style.css` (new, loaded by AdwApplication),
  `easel.gresource.xml`. (Corrected at the check: the Plan first named a widest-width function and a style sheet loaded by
  `application.c`; neither was built, since option B made them unneeded.) Tests: `tests/ui/test-window.c` (a 0008 block), mutants in `tests/mutants.txt`.

### Key interfaces
- `extern const int easel_view_presets[EASEL_VIEW_N_PRESETS]` = {100, 200, 400, 600, 800};
  `int easel_view_preset_index (int pct)`: the index, or -1 when pct is not a preset (10, 25, 50, 42, 300, 0).
- Action `win.magnifier-level`, parameter and state type `i` (the selected level in percent): the lens buttons use it through
  `action-name` and `action-target`, so the selected level is one action state and not read back from the buttons; it does
  not zoom (REQ-0007-02).
- Action `win.zoom-to`, parameter and state type `i`; the menu item label is "%d %%".
- `easel_tool_options_show_tool` switches the stack's page (and hides the line for the empty page) instead of hiding the
  widget. The tests find the toolbox from the Pan button and read the Magnifier's level from the action state; no test-only
  accessor is built (the Plan first named `easel_window_get_toolbox_width`, and `easel_tool_options_get_magnifier_level`
  was removed at the check).

### How each rule is kept
- **REQ-0008-01 and -06:** one array; a core test that every preset below 800 is a step and that stepping in from 100
  visits every preset (300 % is a step and not a preset: `easel_view_preset_index (300) == -1`, nor are 10, 25, 50).
- **REQ-0008-02:** the lens buttons are built in a loop over the presets (labels "1x" … "8x", tooltips "100 %" … "800 %") and bound to `win.magnifier-level`.
- **REQ-0008-03 and -04:** the submenu from the presets; the action calls `easel_canvas_set_zoom`; the Magnifier's selected
  level is not touched because nothing in the path reads or writes it.
- **REQ-0008-05:** `sync_zoom_ui` sets `zoom-to`'s state to the zoom if `easel_view_preset_index` finds it, else 0.
- **REQ-0008-07:** compact style, a shared margin and gap, and a stack whose width is its widest page.

### What grows
Nothing per use: five submenu items and five lens buttons built once, one style sheet, one integer of state.

### Measurements
`docs/engineering.md` budgets one thing this touches: start to a usable window under 0.5 s. A task runs `perf-0001` after
the change (the submenu, the style sheet and the options stack are built at start-up) and the number goes here.
Measured 2026-10-18 after T007 (`perf-0001`, this machine): start, main to the first frame of a 1024 × 768 picture,
201.6 ms, within the 0.5 s budget; opening 1920 × 1080 to the first frame 37.6 ms, 8000 × 8000 78.7 ms; a pencil motion
to the painted frame, median 16.9 / 16.5 ms. No earlier start number is recorded in the specs to compare with.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | core `as1_presets_list_and_index`; ui `as1_zoom_submenu_and_lens_levels` (the five items and the five lens buttons, same levels, in order, labels and tooltips) |
| 2 | ui `as2_zoom_to_preset` (the zoom, the center point kept, the Magnifier's selected level, no pixel, step or modified mark) |
| 3 | ui `as3_zoom_check_follows_zoom` (100, 400 by Ctrl+PageDown, 600 by Zoom In, 300 and Fit's 42 % give none, the Magnifier gives 400) |
| 4 | ui `as4_zoom_to_ignored_when_blocked` (chooser open, a stroke held, no picture yet) |
| 5 | core `as5_every_preset_is_a_step` |
| 6 | ui `as6_toolbox_width_is_the_same_for_every_tool` (the width after each of the eight tools at 100, 150 and 200 % text; 87 px at 100 %; every preset and lens button fully on screen) + manual M1 |

### Constitution check
Layering holds (the list and its index are libc-only core; GTK builds the menu and the buttons from them); no threads or
caches (P5); no new setting (P6 is not touched); every REQ has a scenario and every scenario a test or a manual check (S3).

### Risks
- **GTK behavior, each checked by a probe before the code relies on it (T002, T006):** (a) a stateful action with an integer
  target in a GMenu submenu of a popover shows the check mark on the matching item and none when the state matches no item;
  (b) `g_menu_insert_submenu` into the second section of the View menu built by the template works (the section is a
  GMenu); (c) the width of a hidden group can be had by showing it for the measuring, in one synchronous pass, and the
  toolbox panel keeps that minimum; (d) the compact style makes a preset button 38 px (content 28, as BUTTON_SIZE is now)
  and the lens grid 86 px at the default text size, as a trial with a style sheet gave (lens grid 86, 114 and 143 px at 100,
  150 and 200 % text).
- **Probe T002 (2026-10-18), risks (a) and (b) hold:** the View menu's model and its second section are `GMenu`s, and
  `g_menu_insert_submenu` at position 0 of that section puts "Zoom" first in it, before Normal Size. The popover builds one
  item per preset with the radio role; with the action's state 400 only the "400 %" item is active, and with 42 none is.
  Not shown headless: how the theme draws an active radio item (a check mark or a dot); M1 looks, and if it is not a check
  mark the spec's wording (REQ-0008-05) is the question, not the code.
- **Probe T006 (2026-10-18), risk (d) holds and risk (c) fails as written.** (d): with a trial compact style (padding 2 px
  3 px, the lens label at 82 %, grids with 4 px margins and a 2 px gap) and each group shown by choosing its tool, the
  preset button is 34 px, the Brush group 78 px and the lens group 80 px, and the toolbox stays 87 px for Pan, the Brush
  and the Magnifier at the default text size; the lens group is 108 px at 150 % text and 137 px at 200 % (the tool grid
  stays 86). (c): a hidden group measures 0, and shown for the instant it measures its natural width (Brush 112, lens 154
  with the theme's style), and the toolbox panel keeps a minimum width set on it (160 px held with Pan); but a style change
  made while a group is hidden is not in that measurement (the compact Brush group measured 106, the old padding, until it
  had been shown in a frame, then 78). So the widest width cannot be had from hidden groups at start-up or after a text
  size change without showing them; the fallback is JOD's choice before T007.
- **Probe T006, option B (JOD's choice, 2026-10-18), holds on (a) to (d).** The option groups as pages of a stack whose width
  is the widest page and whose height is the shown page's, with an empty page for tools without options, and the compact
  style set while the groups were not shown. (a) The toolbox is 87 px for the empty page, the Brush, the Magnifier, the
  Airbrush and the Eraser at 100 % text, 109 px for all at 150 % and 138 px for all at 200 %. (b) The text size changed
  while the window was open: the width followed at once, and back at 100 % it was 87 px again. (c) The height follows the
  shown page: 0 for the empty page, Brush 225, Magnifier 101 / 129 / 157 (100 / 150 / 200 %), Airbrush 81, Eraser 162.
  (d) With the empty page, the options take 0 px of height, their line is hidden and not drawn, and they start 6 px below
  the tool grid, which is the grid's own bottom margin, so no gap is added. The plan's approach changes accordingly: the
  options widget stays shown and its pages switch; for a tool without options the empty page shows and the line is hidden. If (c) fails, the fallback is a fixed minimum width of 87 px at the default text size and the
  widest-content rule only for the lens (the text sizes then differ between tools; the spec would need an amendment).
- **The preset drawings must still read at 28 px:** the biggest footprint is the 24-pixel spray disc, drawn one logical
  pixel per picture pixel, and the content size stays 28, so only padding, margins and gaps shrink; M1 looks at them.
- **The 87 px in the test depends on the theme's button padding:** the test measures the toolbox with Pan chosen first and
  asserts the other tools equal to it, and asserts 87 at 100 % text on the test's theme (Adwaita).
- **The first style sheet of the app:** the theme's own rules still apply; the sheet only sets padding, margins, a font
  size on the compact buttons and nothing about color.

## Tasks
Groups end with a full `meson test`; M1 is the manual check (the look and the menu cannot be judged headless).
Each ticked task gets a `Result:` line under it.

A. The list
- [x] T001 The preset list and its index in the core, the Magnifier's levels as its alias (REQ-0008-01, REQ-0008-06; AS-1, AS-5) - done when core `as1_presets_list_and_index` and `as5_every_preset_is_a_step` pass, each fails first, the Magnifier's core tests of 0007 pass unchanged, and mutants (300 % made a preset, a preset dropped from the steps, the index off by one) are in `tests/mutants.txt` and killed
  Result: `as1_presets_list_and_index` failed first at `test-view.c:126: CHECK failed: easel_view_preset_index (expected[i]) == i` (the index returned -1); `as5_every_preset_is_a_step` failed first only by not compiling (`easel_view_presets` undeclared), and once the list existed it passed at once, since the zoom steps already held every preset: characterization for its behavior part. The Magnifier's levels are now an alias of the list (its two 0007 mutants moved to `view.c`); 7 of 7 mutants of REQ-0008-01, -06 and REQ-0007-02 killed
  Result (after the five-preset amendment, 2026-10-18): the list is now 100, 200, 400, 600 and 800 %; `as1_presets_list_and_index` (five values, 10, 25 and 50 not presets) failed first at `EASEL_VIEW_N_PRESETS == 5` and `as3_magnifier_zoom_rule` at its five-level check, then both passed; `as5_every_preset_is_a_step` now walks from 100 %. `EASEL_MAGNIFIER_DEFAULT` is index 1 (2x). Mutants: the 1x and 300 % ones rewritten for the new array, the 1/10x one deleted, "10 % is still a preset" added; all killed

B. The View > Zoom submenu
- [x] T002 Probe: a stateful integer action in a submenu inserted into the template's View menu shows the check mark on the matching item and none otherwise (Plan risks a, b) - done when the probe's result is written in the Plan's risks
  Result: probe, no code kept: risks (a) and (b) hold (items with the radio role, the matching one active, none at 42; the submenu first in the second section); the drawing of the mark is left to M1
- [x] T003 The submenu and `win.zoom-to` (REQ-0008-03, REQ-0008-04; AS-1, AS-2, AS-4) - done when ui `as1_zoom_submenu_and_lens_levels` (the menu part), `as2_zoom_to_preset` and `as4_zoom_to_ignored_when_blocked` pass, each fails first, and the 0004 menu tests still pass
  Result: each failed first: `as1_zoom_submenu_and_lens_levels` at `'sub' should not be NULL` (no Zoom submenu), `as2_zoom_to_preset` at `easel_window_get_zoom (win) == 800: (100 == 800)`, `as4_zoom_to_ignored_when_blocked` at `g_action_group_has_action (... "zoom-to")` should be TRUE; then they pass. The 0004 test `as1_menus` was updated for the amended REQ-0004-01 (the Zoom submenu first in its group, with a pointer note in 0004); 2 of 2 new mutants (REQ-0008-03, -04) killed by assertions
  Result (after the five-preset amendment, 2026-10-18): the submenu has five items, 100 % … 800 %; `as1_zoom_submenu_and_lens_levels` failed first at `g_menu_model_get_n_items (sub) == 5: (8 == 5)`, then passed; `win.zoom-to` ignores a value that is not a preset (25 % is asserted to change nothing)
- [x] T004 The check mark follows the zoom (REQ-0008-05; AS-3) - done when ui `as3_zoom_check_follows_zoom` passes and fails first, and a mutant (the state not updated on a zoom change) is killed
  Result: `as3_zoom_check_follows_zoom` failed first at `zoom_checked (win) == 100: (0 == 100)` (the state never set); `sync_zoom_ui` now sets it and the test passes; 2 of 2 mutants of REQ-0008-05 killed by assertions (the mark not updated; a mark kept at 300 %)

C. The toolbox
- [x] T005 The lens built from the presets (REQ-0008-02; AS-1) - done when the lens part of `as1_zoom_submenu_and_lens_levels` passes and the 0007 options tests pass unchanged
  Result: characterization: the lens part of `as1_zoom_submenu_and_lens_levels` passed at once, because T001 already made the Magnifier's levels an alias of the preset list, so the buttons were built from it; the mutant that leaves out the last preset is killed by that test (`'b' should not be NULL`), and the 0007 options tests pass unchanged
  Result (after the five-preset amendment, 2026-10-18): the lens shows five buttons, 1x 2x / 4x 6x / 8x, on their own stateful action `win.magnifier-level` (the selected level; select only, no zoom) instead of a toggle group; the 0007 tests were amended with the spec: `as2_magnifier_level_kept` failed first at its layout check (`265 == 242`, three rows expected) and `as3_magnifier_click` at `find_by_tooltip (..., "10 %") should be NULL`, then both passed; `as5_box_follows_pointer` passed at once once its reduction case was removed (characterization); the REQ-0007-02 "starts at 4x" and REQ-0007-03 "level never reaches the canvas" mutants were rewritten for the action and are killed
- [x] T006 Probe: a hidden option group measured while shown for the instant, and the compact style's sizes at 100, 150 and 200 % text (Plan risks c, d) - done when the numbers are in the Plan's risks, and if (c) fails, JOD has chosen the fallback before T007
  Result: probe, no code kept: measuring hidden groups failed (a style change made while hidden is not measured: Brush 106 against 78); JOD chose option B, and its probe held on all four points (87 / 109 / 138 px for every tool at 100 / 150 / 200 % text, following a text-size change at once, the height of the shown page, 0 px and no line for the empty page); numbers in the Plan's risks
- [x] T007 Compact presets and lens, and one toolbox width for every tool (REQ-0008-07; AS-6) - done when ui `as6_toolbox_width_is_the_same_for_every_tool` passes and fails first, the 0006 and 0007 options tests pass, and manual M1 (the compact presets and lens buttons, the View > Zoom submenu and its check mark, light and dark theme) passes
  Result: rewording first, behavior-neutral: the five "options not mapped" assertions of 0006 `as1_options_panel` (3) and 0007 `as1_new_tools_toolbox_hints` (2) became `assert_no_options_shown` (no option button on screen, the options 0 px high) and both tests passed on the code before T007. Then `as6_toolbox_width_is_the_same_for_every_tool` failed first at `gtk_widget_get_width (toolbox_panel (win)) == width: (113 == 87)` (the Brush); with the stack and the compact style it passes: 87 / 109 / 138 px for every tool at 100 / 150 / 200 % text. 3 of 3 mutants of REQ-0008-07 killed by assertions (no empty page; the line left on it; a stack as wide as its page); the third first died on a test crash (`gtk-xft-dpi` read as -1 under meson), fixed in the test. Full `meson test` 27 of 27. Manual M1: pending
  Result (after the five-preset amendment, 2026-10-18): `as6_toolbox_width_is_the_same_for_every_tool` with the five-button lens passed at once (characterization: fewer, narrower buttons than before). Full `meson test` 27 of 27; `tools/mutate.sh --lint`: 148 mutants, 0 stale. Manual M1: restarted at step 1 on the five-button lens
  Result: manual M1 step 1 (JOD, 2026-10-18): the toolbox width is the same for every tool and the Brush, Airbrush and Eraser presets are fine; the five lens buttons are too small and sit to the left: they should be centered under the two tool columns and better visible, as in JOD's reference screenshot (spec gap, below)
  Result (lens amendment, 2026-10-18): REQ-0008-07 and AS-6 amended (the levels laid out as the tool buttons, normal-size labels); `as6_toolbox_width_is_the_same_for_every_tool` failed first at `assert_lens_like_tool_buttons: r.origin.x == pan.origin.x: (4 == 6)`, then passed (87 px at 100 % and 150 % text, 93 px at 200 %, the same for every tool); 2 new mutants (a narrower gap; smaller labels) killed by assertions; full `meson test` 27 of 27, `mutate.sh --lint` 150 mutants, 0 stale
  Result: manual M1 step 1 again (JOD, 2026-10-18): all is ok: the lens 1x 2x / 4x 6x / 8x centered under the tool columns at normal size, the toolbox the same width for every tool
  Result: manual M1 steps 2 to 4 (JOD, 2026-10-18): "M1 is ok", reported as a whole after the submenu step (the Zoom submenu, its mark, the zoom it gives; the light and dark themes); JOD did not say how the theme draws the mark

D. The numbers
- [x] T008 Start-up time after the change (engineering.md: start to usable window under 0.5 s) - done when `perf-0001` has run and its number is in the Plan's Measurements
  Result: measurement, no test: `perf-0001` start to the first frame 201.6 ms against the 0.5 s budget; numbers in the Plan's Measurements

Change of 2026-10-28 (JOD, 2026-10-27): the zoom reaches 1600 % (REQ-0001-16, REQ-0004-02, -07, -08, REQ-0007-02, -03, REQ-0008-01, -02,
-03, -06; 0004's AS-3 and AS-31, 0007's AS-1, AS-2 and AS-19, 0008's AS-1 and AS-5). The Eraser's cursor at 1600 % was probed first (0006's
Plan, 2026-10-28: no cap).
- [x] T009 1600 % as a level, a preset, a step and a Magnifier level, Fit to Window still capped at 800 % - done when core `test-view`
  (the levels, the presets, Zoom In from 800 % and none from 1600 %, the Fit cap at 800 %, the pinch's stop at 1600 %) and
  `test-magnifier` (the six levels; 16x from 100 % gives 1600 %, at 1600 % back to 100 %; 8x at 800 % back to 100 %) and ui tests of the
  View > Zoom submenu (six items), the Magnifier's six buttons (1x 2x / 4x 6x / 8x 16x, the tooltips), Zoom In insensitive at 1600 %, the
  wheel's last notch, AS-31's grid up to 1600 % and 0007's AS-19 are written first and red, then green; mutants; the toolbox width of
  REQ-0008-07 kept with the sixth button
  Result (2026-10-28): core `test-view` red first at `easel_view_zoom_step (100, 21) == 1600` and `test-magnifier` at the sixth level, then
  green with 1600 in the levels and the presets and Fit to Window capped by its own `EASEL_VIEW_FIT_MAX` (800). The ui tests that still
  said 800 % was the end failed with the new levels and were brought to the new wording (0001's `as13_zoom_keeps_point_under_pointer`,
  0004's `as3_zoom_buttons`, 0008's `as1_zoom_submenu_and_lens_levels`, 0016's `as13_canvas_menu`), with the six levels in
  `as2_magnifier_level_kept`, the lens layout (16x under the Magnifier's button) and 0008's toolbox width; `as19_magnifier_16x` (0007's
  AS-19) is new. AS-31 (`as10_grid_covers_the_view_after_zoom_and_pan`, up to 1600 %) failed once on its own helper: after the zoom the
  view stood at the picture's right edge, where the last column's far side is the picture's edge and has no line (REQ-0004-12), so the
  check now looks at the cell before it. The menus and the lens follow the lists in the code, so no ui code changed. Mutants: the levels
  stopping at 800 %, 1600 % not a preset, Fit to Window up to 1600 %, and three re-anchored; `mutate.sh` for the twelve changed REQ ids:
  27 of 27 killed. Full isolated suite 51 of 51. perf-0004's grid case now zooms to 800 % directly (its Zoom In steps would have reached
  1600 %).
- [x] T010 perf at 1600 % (the 8000 x 8000 picture, the real session, the pointer parked, three runs, after JOD's "ready") - done when the
  frame time of a scroll and of a pencil stroke and the peak memory at 1600 % are in the Plan and compared with 800 % (drawn by the visible
  tiles, it should not grow), and the gate: the full isolated `meson test`, `mutate.sh` for the changed REQ ids, `--lint`, the lints
  Result (2026-10-28, JOD's "ready", the real session, the pointer parked, no other Easel window; load 0.22 to 0.46; three runs of the new
  `perf-0008`, each 800 %, 1600 %, then 800 % again on an 8000 x 8000 picture, the view in the middle):
  | Zoom | A scroll (a tenth of the page) to the frame: median / worst | A pencil motion: median / worst | A frame with the 1-pixel grid | Resident, peak |
  |---|---|---|---|---|
  | 800 % (first) | 16.4 to 16.5 / 20.7 to 21.9 ms | 16.6 to 16.7 / 64.4 to 73.4 ms | 16.6 to 16.7 / 18.7 to 19.5 ms | 363 MB, 363 MB |
  | 1600 % | 16.6 / 18.3 to 19.3 ms | 16.6 to 16.7 / 19.2 to 19.5 ms | 16.7 to 16.8 / 18.9 to 21.8 ms | 363 MB, 363 MB |
  | 800 % (again) | 16.5 to 16.6 / 18.4 to 19.4 ms | 16.5 to 16.7 / 19.0 to 20.3 ms | 16.6 to 16.8 / 17.9 to 20.7 ms | 363 MB, 363 MB |
  1600 % costs what 800 % costs: one frame (16.6 ms is the screen's) for a scroll, a pencil motion and a frame with the grid, and no more
  memory. The slow first pencil motion (64 to 73 ms) comes only in the first block of each run, at 800 %, never at 1600 % nor in the
  second 800 % block: the first stroke of the process, not the zoom. The gate: full isolated `meson test` 51 of 51, mutants above,
  `--lint` and the spec lints below.
  perf-0004 with the grid at 8000 × 8000, 2026-11-02, Vulkan, 3 runs each interleaved with `e5bbe7e`, load 0.63 to 0.75 (pointer position and
  other windows were not checked; indicative): at 800 %, cell 1 16.6 to 16.7 / 18.1 to 19.1 ms against 16.6 to 16.8 / 18.9 to 19.5 ms, and
  axonometric cell 4 16.6 to 16.8 / 18.9 to 30.8 ms against 16.6 / 18.3 to 19.1 ms; at 1600 %, on HEAD alone, 16.6 to 16.7 / 18.9 to 19.4 ms and
  16.6 to 16.7 / 18.2 to 19.1 ms. Peak memory 605 to 606 MB against 605 MB. No slow case comes from the zoom. A median at the display's 16.6 ms
  cannot show a cost under one frame.

## Check
- Date and commit: 2026-10-18, `5bd6583` plus the uncommitted work of 0008
- Tests: `meson test` 27 of 27 pass (the cases of this feature: 7 ui `/0008/...`, core `as1_presets_list_and_index` and `as5_every_preset_is_a_step`, and the 0006/0007 tests reworded or amended with it)
- Mutants: 150 of 150 killed (693 s); 3 killed only by a crash, none of them 0008's (finding 1)
- Warnings: 0. Lint, `lint --names 0008` and status clean (8/8 tasks)
- Budget: start to the first frame 201.6 ms (`perf-0001`, budget 0.5 s)
- Results of the tasks: T001 red then green, behavior part of `as5_every_preset_is_a_step` characterization; T002 probe (the radio items hold); T003 red then green; T004 red then green; T005 characterization (the alias of T001 already built the lens from the list), then red at the five-preset amendment; T006 probe (hidden measuring failed, option B held); T007 the rewording behavior-neutral, AS-6 red then green, characterization at the five-preset amendment, red then green at the lens amendment; M1 ok (step 1 twice, steps 2 to 4 as a whole); T008 measured
- Findings:
  1. Medium, outside 0008: three mutants are killed only by a crash, `Adwaita-FATAL-CRITICAL: adw_alert_dialog_get_heading: assertion 'ADW_IS_ALERT_DIALOG (self)' failed` (REQ-0004-26 "a refused undo or redo shows no error", REQ-0005-15 "a start-up file that cannot be opened shows no error", REQ-0001-42 "an existing file is replaced without the question when .png was added"): the tests read the heading of a dialog that is not there. The 0007 check said "none killed only by a crash", but its search did not look for FATAL-CRITICAL, so it missed them. Fixed: `tools/mutate.sh` now reports such a kill as `crashed` ("killed by crash (test gap)": no assertion of ours, and a FATAL or CRITICAL message, "Aborted" or a signal) and fails the run; it flagged all three before the tests were touched; the three tests now assert that the dialog exists (`g_assert_nonnull`) before reading its heading, and the three mutants are killed by that assertion.
  2. Low: Plan drift. The Plan still says "a smaller label for the lens" (replaced by the lens amendment), "a function that returns the widest natural width" and "`application.c` (loads the sheet)" (the options are a stack; AdwApplication loads `style.css` by itself), and names `easel_window_get_toolbox_width`, which was not built (the test finds the toolbox from the Pan button). Fixed: the Plan's approach, files and interfaces now say what was built (the lens laid out as the tool buttons, the options stack, `style.css` loaded by AdwApplication, no test-only accessor).
  3. Low: no pointer notes in 0006 for what REQ-0008-07 changes there (the look of the presets of REQ-0006-04, -07, -10; "no options" shown as an empty page under REQ-0006-02). Fixed: pointer notes on REQ-0006-02, -04, -07 and -10.
  4. Low: `easel_tool_options_get_magnifier_level` is now used only by tests and reads the buttons, not the `win.magnifier-level` state the canvas uses; it cannot drift today (GTK sets the buttons from that state). Fixed: the tests read the action state (`magnifier_level`); the accessor and its read-back are deleted; the mutant that quoted it is replaced by "the Magnifier's choice is not kept" on the action, killed.
  5. Low: characterization runs (T001 in part, T005, T007 at the five-preset amendment), and two red runs that were only compile failures (part of T001, part of T003). Left as recorded.
  6. Low: M1 steps 2 to 4 were reported as a whole ("M1 is ok"); how the theme draws the mark was not said. Left as recorded. Settled: JOD checked M1 again after the fixes on 2026-10-18 ("all checked and works").
  7. Carried, not 0008's: the airbrush flake of 0006, with the pointer hypothesis of the learning log; the full runs of this check passed.
- After the fixes: `meson test` 27 of 27; mutants of REQ-0001-42, -0004-26, -0005-15, -0007-02 and -0007-03 all killed by assertions; `tools/mutate.sh --lint` 150 mutants, 0 stale; 0 warnings.
- Verdict: 0008 is built as specified; findings 1 to 4 fixed, 5 and 7 left as recorded, 6 settled by JOD's M1 check after the fixes.
