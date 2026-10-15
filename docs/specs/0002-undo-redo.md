# 0002 Undo and redo

Status: implemented
Depends on: 0001

## Why
Mistakes must cost nothing: every edit can be taken back and put back again, as far back as memory allows, not three
steps as in XP (constitution P3). The modified mark must tell the truth: undoing back to what is on disk means there is
nothing to save.

## Requirements
Look and input rules come from `docs/design.md` (header bar Undo and Redo, one user-visible edit = one undo step).
In this feature the only edit is a pencil stroke (0001); later features add their edits as steps under the same rules.

### History
- REQ-0002-01 When an edit that changes at least one pixel is completed, the app shall add exactly one undo step for it.
- REQ-0002-02 A stroke that changes no pixel, a cancelled stroke (REQ-0001-23), panning, scrolling, zooming, a tool change and saving shall add no undo step. (0003 adds choosing, swapping and editing colors: REQ-0003-09.)
- REQ-0002-03 When the user chooses Undo, the app shall restore the picture to how it was before the most recent step that has not been undone, and that step becomes redoable.
- REQ-0002-04 When the user chooses Redo, the app shall reapply the most recently undone step, and it becomes undoable again.
- REQ-0002-05 When an edit is completed after one or more undos, the app shall discard every redoable step.
- REQ-0002-06 While there is nothing to undo, the Undo button shall be insensitive and the Undo shortcut shall do nothing.
- REQ-0002-07 While there is nothing to redo, the Redo button shall be insensitive and the Redo shortcuts shall do nothing.
- REQ-0002-08 Undo and redo shall change only the picture: the zoom, the active tool and the colors stay as they are, and the scroll position stays as it is as far as the picture's size allows.
- REQ-0002-09 If Undo is chosen during a stroke, then the app shall cancel the stroke as in REQ-0001-23 (nothing is added, no redo step appears); further Undo or Redo is ignored until the mouse button is released, or the window loses focus (as REQ-0001-25), in case the release never arrives.

### Controls
- REQ-0002-10 The header bar shall show Undo and Redo buttons right after Save (order per `docs/design.md`: Open, Save, Undo, Redo), with the standard undo and redo icons and the tooltips "Undo" and "Redo".
- REQ-0002-11 Undo shall be Ctrl+Z; Redo shall be Ctrl+Y and Ctrl+Shift+Z. The shortcuts work wherever keyboard focus is in the window, and do nothing while the quit prompt or a save, replace or error dialog is open. (0003 adds the color chooser to these dialogs: REQ-0003-14.)

### Saved state
This replaces the "modified" rule of 0001 (REQ-0001-13, REQ-0001-26): the modified mark now follows the history.
- REQ-0002-12 Each state of the picture in the history is identified by the edit that produced it (the start counts as one state), not by how many steps deep it is. The picture shall count as modified exactly when its current state differs from the saved state: the state current at the last successful save, or the start for a never-saved picture.
- REQ-0002-13 Undoing or redoing back to the saved state shall clear the modified mark, so quitting there closes at once (REQ-0001-34).
- REQ-0002-14 If the saved state can no longer be reached (its step was discarded by REQ-0002-05 or dropped by REQ-0002-16), then the picture shall stay modified until the next successful save, even if its pixels happen to equal the saved file.
- REQ-0002-15 A failed save (REQ-0001-31) shall not move the saved state. A save requested during a stroke ends the stroke first (REQ-0001-33), so the saved state is the one after that stroke.

### Memory budget
- REQ-0002-16 The app shall keep the memory the history stores to undo and redo (not the picture itself, not other app memory) within the budget; when a new step would exceed it, the app shall drop the oldest steps, oldest first, until it fits, silently. A step shall need at most 16 bytes per pixel it changed plus 64 bytes for the step itself. So with the default 512 MB budget and
  small strokes (each changing at most 50,000 pixels, at most 0.8 MB each), at least the 640 most recent strokes stay
  undoable; this guarantee assumes every stroke is that small. A single edit bigger than the whole budget follows REQ-0002-17.
- REQ-0002-17 If one step alone is larger than the budget, then the app shall keep it as the only step, so the last edit can always be undone.
- REQ-0002-18 The budget shall be read at start-up from the settings file (syntax as REQ-0001-40): section `[history]`, key `memory`, a whole number of megabytes from 16 to 8192, default 512. A missing, invalid or out-of-range value means 512, silently. A change takes effect at the next start.
- REQ-0002-19 A settings file the app creates (REQ-0001-04) shall also hold `[history]` with `memory=512`; an existing file without it is never rewritten.

## Scenarios
- AS-1 (REQ-0002-01, REQ-0002-03, REQ-0002-11) Given a white picture, When the user draws stroke A, then stroke B, and presses Ctrl+Z, Then the picture shows A only; When the user presses Ctrl+Z again, Then the picture is white | Verified by: automated (core) + automated (ui, shortcuts)
- AS-2 (REQ-0002-04, REQ-0002-11) Given strokes A and B were both undone, When the user presses Ctrl+Y, Then A is back; When the user presses Ctrl+Shift+Z, Then B is back | Verified by: automated (core) + automated (ui, shortcuts)
- AS-3 (REQ-0002-03, REQ-0002-04, REQ-0002-10) Given stroke A, When the user clicks the header-bar Undo button, Then A disappears; When the user clicks Redo, Then A is back | Verified by: automated (ui)
- AS-4 (REQ-0002-05) Given A and B were drawn and B was undone, When the user draws C, Then Redo is insensitive and the picture shows A and C | Verified by: automated (core) + automated (ui)
- AS-5 (REQ-0002-02) Given a picture with stroke A only, When the user pans, scrolls with the wheel, zooms, switches Pan to Pencil and back, primary-clicks a pixel of A, cancels a stroke with the other button, and saves, Then one Undo removes A and Undo is then insensitive | Verified by: automated (ui)
- AS-6 (REQ-0002-06, REQ-0002-07) Given a fresh start, When the window opens, Then Undo and Redo are insensitive and Ctrl+Z and Ctrl+Y change nothing; When the user draws one stroke, Then Undo becomes sensitive and Redo stays insensitive | Verified by: automated (ui) + manual M1
- AS-7 (REQ-0002-08) Given Pan active at 400 % scrolled to the middle, after a Pencil stroke, When the user presses Ctrl+Z, Then the stroke disappears and the zoom, scroll position and tool are unchanged | Verified by: manual M2
- AS-8 (REQ-0002-09) Given a stroke in progress, When the user presses Ctrl+Z before releasing, Then the stroke vanishes, Redo stays insensitive, and a second Ctrl+Z before releasing changes nothing; When the window loses focus before the release, Then Undo and Redo work again | Verified by: automated (ui) + manual M4
- AS-9 (REQ-0002-11) Given keyboard focus on a toolbox button, When the user presses Ctrl+Z, Then the last stroke is undone; Given the quit prompt is open, When the user presses Ctrl+Z, Then nothing changes | Verified by: automated (ui)
- AS-10 (REQ-0002-12, REQ-0002-13) Given a picture saved as a.png, When the user draws a stroke, Then the title shows the modified mark; When the user presses Ctrl+Z, Then the mark is gone and Ctrl+Q closes at once; When the user presses Ctrl+Y instead, Then the mark is back | Verified by: automated (ui)
- AS-11 (REQ-0002-12, REQ-0002-13) Given a fresh untitled picture, When the user draws a stroke and undoes it, Then the title has no modified mark | Verified by: automated (ui)
- AS-12 (REQ-0002-14) Given a.png saved after stroke A, When the user undoes A and draws the identical stroke A again, Then the picture stays modified | Verified by: automated (core)
- AS-13 (REQ-0002-14, REQ-0002-16) Given a picture saved after its 5th stroke and later strokes that make the history drop strokes 1 to 5, When the user undoes as far as possible, Then the title still shows the modified mark; Given a never-saved picture (saved state = the start) and the same drops of the oldest strokes, When the user undoes as far as possible, Then the title shows the modified mark too, because the start is gone | Verified by: automated (core)
- AS-14 (REQ-0002-15) Given a modified picture, When saving into a read-only folder fails and the user presses Ctrl+Z and then Ctrl+Y, Then the title still shows the modified mark | Verified by: automated (ui)
- AS-15 (REQ-0002-15) Given a stroke in progress on a picture saved as a.png, When the user presses Ctrl+S before releasing, Then the title has no mark; When the user then presses Ctrl+Z, Then the stroke is undone and the mark is back | Verified by: automated (ui)
- AS-16 (REQ-0002-16) Given a 16 MB budget, a 1024x768 picture and small strokes that each change exactly 50,000 different pixels (so each needs at most 0.8 MB and at least 20 fit), When the user draws 200 such strokes, Then the stored history is at most 16 MB, at least the 20 most recent strokes can be undone, and Undo becomes insensitive before the oldest stroke is undone. (The guarantee assumes small strokes; a single edit bigger than the budget is AS-17.) | Verified by: automated (core)
- AS-17 (REQ-0002-17) Given a 16 MB budget and earlier strokes, When one edit needs more than 16 MB to undo, Then it is the only step: one Undo removes it, then Undo is insensitive | Verified by: automated (core)
- AS-18 (REQ-0002-18) Given `[history]` holding `memory=abc`, then `memory=8`, then `memory=99999`, then no key at all, When the app starts each time, Then the budget is 512; with `memory=64` it is 64 | Verified by: automated (core)
- AS-19 (REQ-0002-19) Given no settings file, When the app starts, Then the created file also holds `[history]` and `memory=512`; Given an existing file without `[history]`, Then the file is byte-for-byte unchanged | Verified by: automated (io)

## Edge cases
- Holding Ctrl+Z: key repeat undoes one step per repeat until nothing is left (REQ-0002-06).
- Undo right after a save: the picture becomes modified (REQ-0002-12).
- The same action again while its dialog or prompt is already open: Undo and Redo do nothing (REQ-0002-11).
- A huge single edit: kept alone, over the budget (REQ-0002-17).
- Undo during a stroke: cancels it (REQ-0002-09).

## Out of scope
History list or panel; undo of view changes (zoom, scroll); clearing history on New or Open (0005); the Preferences
control for the budget (0013); steps for edits that later features add (each feature adds its own).
Deferred idea, not planned: keep old undo steps on disk instead of dropping them. Revisit only if the 512 MB default
proves too small in practice.

## Open questions
(none)

## Behavior reference
- `docs/reference/new-behaviors.md`: no fixed step limit, only a memory budget; Redo is Ctrl+Y or Ctrl+Shift+Z; default budget 512 MB, minimum and maximum set here.
- `docs/design.md`: Undo and Redo in the header bar; one user-visible edit = exactly one undo step; pending state commits before undo history.
- XP Paint: Ctrl+Z undoes, Ctrl+Y redoes, three levels only; a new edit after undo discards the redo steps.

## Plan

### Approach
A new core module, `history`, holds the steps:
- **What a step stores:** an array of `{uint32 index, uint32 old, uint32 new}` (12 bytes per pixel) plus a fixed header,
  whose size the budget counts as 64 bytes. Pictures are at most 10000x10000, so a pixel index fits in 32 bits.
- **How a stroke becomes a step:** the stroke already logs each changed pixel's old value for cancel (0001). When a stroke
  ends, its log becomes a step, with the stroke color as the new value.
- **Undo and redo** write the old or new values back through `easel_image_set`, so the canvas's dirty-area update shows
  them like any edit.
- **States:** each step gets a unique, increasing id. A state is the id of the last applied step, and 0 is the start.
  The saved state is such an id; when its step is discarded or dropped it becomes "none", so the picture stays modified
  (REQ-0002-14). Modified means `current != saved`.
- **Budget:** the sum over steps of 12 bytes per pixel plus 64 bytes. When a new step would exceed the budget, the oldest
  steps are dropped first. A lone step that is bigger than the budget is kept (REQ-0002-17).

The window owns the history:
- `win.undo` and `win.redo` actions, enabled or disabled from the history (REQ-0002-06 and -07); app accelerators Ctrl+Z,
  Ctrl+Y and Ctrl+Shift+Z.
- Header-bar buttons with the `edit-undo-symbolic` and `edit-redo-symbolic` icons.
- The title follows `easel_history_is_modified`.
- Undo and redo are ignored while a dialog is visible or a save dialog is open (REQ-0002-11), and while a mouse button is
  held. Undo during a stroke cancels it (REQ-0002-09).

Settings: core parses `[history] memory` into the budget, and the default text gains `[history]` / `memory=512`.

### Memory and copying rules
- **Changed pixels only.** A step stores one 12-byte entry per pixel whose value changed, never a bounding box or row
  copy. A stroke logs a pixel only when its value actually changes, so pixels a stroke crosses twice, or that already had
  the stroke's color, cost nothing. A 3,000-pixel diagonal costs 36 KB + 64 B, whatever its box.
- **No full-picture copy in the history.** Starting a stroke copies nothing; undo and redo write back only the step's
  pixels.
- **Display copy (known, from 0001).** Each screen update still copies the whole picture into a new texture
  (`src/ui/canvas.c`). It is not stored and not counted in the budget, but an undo triggers one. JOD's decision
  (2026-10-15): keep it in 0002 and measure it; the fix stays with the deferred 8000x8000 work (0004).

### Measurements (T008, 2026-10-15)
Benchmark `perf-0002` (`meson test -C build --benchmark --verbose`) times one undo and one redo of a stroke of exactly
50,000 changed pixels (50 zigzag rows of 1,000), from the action to the next painted frame. Default GTK renderer, AMD
Ryzen AI 7 PRO 350 with Radeon 860M, 60 Hz display (a refresh is 16.7 ms). Two runs, 20 undo/redo pairs each; the
larger median is shown.

| Case | Action itself | On screen (median / worst) | Verdict |
|---|---|---|---|
| Undo, 1920x1080 | 0.9 ms | 16.7 ms / 17.9 ms | budget met: the next refresh |
| Redo, 1920x1080 | 0.8 ms | 16.9 ms / 18.6 ms | budget met: the next refresh |
| Undo, 8000x8000 (information) | 54 ms | 94 ms / 104 ms | lags: about 11 per second |
| Redo, 8000x8000 (information) | 54 ms | 96 ms / 99 ms | lags: about 11 per second |

The 8000x8000 time is the whole-picture copy to the screen texture (about 51 ms, the same as a stroke in 0001), not
the history. Measured on the core alone (/sdd-check, 2026-10-15), for the same 50,000-pixel stroke on 8000x8000:
starting a stroke 0.0009 ms, ending it into the history 0.0009 ms, undo 0.083 ms mean (0.124 ms worst), redo 0.082 ms;
peak memory of the whole run 252 MB, of which the picture itself is 250 MB, so the history copies no picture.

640 strokes at the default budget (REQ-0002-16) is arithmetic from the 16 MB test, not tested. The fix stays with the deferred 8000x8000 work (0004), as
decided. One run of `perf-0001` stalled once (a frame never arrived, probably the window was not visible while it ran);
the rerun was normal. Run the benchmarks with the window visible.

### Files
- New: `src/core/history.{c,h}`, `tests/core/test-history.c`, `tests/perf/perf-0002.c` (a benchmark, like perf-0001).
- Changed: `src/core/stroke.{c,h}` (a stroke can end into a history), `src/core/settings.{c,h}`, `src/ui/canvas.{c,h}`
  (cancel a stroke; is a button held), `src/ui/window.{c,h}`, `src/ui/window.ui`, `src/ui/application.c` (accelerators),
  `tests/core/test-settings.c`, `tests/io/test-settings-file.c`, `tests/ui/test-window.c`.

### Key interfaces (core)
- `EaselHistory *easel_history_new (size_t budget_bytes)`; `easel_history_free`.
- `bool easel_history_undo (EaselHistory *h, EaselImage *img)`;
  `bool easel_history_redo (EaselHistory *h, EaselImage *img)`; `easel_history_can_undo` and `easel_history_can_redo`.
- `void easel_history_mark_saved (EaselHistory *h)`; `bool easel_history_is_modified (const EaselHistory *h)`;
  `size_t easel_history_bytes (const EaselHistory *h)`.
- `size_t easel_stroke_end (EaselStroke *s, EaselHistory *h)`: when h is not NULL and pixels changed, the stroke becomes
  one step (the 0001 callers pass NULL).
- `EaselSettings.undo_budget_mb` (16..8192, default 512).

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | core `as1_undo_restores`, ui `as1_undo_shortcuts` (accelerators registered and the action undoes) |
| 2 | core `as2_redo_reapplies`, ui `as1_undo_shortcuts` (both Redo accelerators) |
| 3 | ui `as3_header_buttons` |
| 4 | core `as4_new_edit_discards_redo`, ui `as4_redo_insensitive_after_edit` |
| 5 | ui `as5_non_edits_add_no_step` |
| 6 | ui `as6_initial_sensitivity` + manual M1 |
| 7 | manual M2 |
| 8 | ui `as8_undo_during_stroke`, ui `as8_focus_loss_unblocks_undo` + manual M4 |
| 9 | ui `as9_shortcuts_focus_and_dialogs` (the action activated from a focused toolbox button; ignored while the quit prompt is open) |
| 10 | ui `as10_modified_follows_history` |
| 11 | ui `as11_undo_to_start_unmodified` |
| 12 | core `as12_rediscarded_saved_state` |
| 13 | core `as13_dropped_saved_state`, core `as13_dropped_start_state` |
| 14 | ui `as14_failed_save_keeps_saved_state` (failure path, automated) |
| 15 | ui `as15_save_during_stroke` |
| 16 | core `as16_budget_drops_oldest` |
| 17 | core `as17_huge_step_kept_alone` |
| 18 | core `as18_budget_setting`, ui `as18_budget_reaches_history` |
| 19 | io `as19_new_file_has_history`, io `as19_existing_file_untouched` |

### Constitution check
- P3: memory budget only, no step limit.
- P6: the budget is a setting.
- Layering: history is libc-only core; the ui calls it.
- One user-visible edit is one step.

### Risks
- **Large edits undo pixel by pixel through the dirty path.** Each undo copies the whole picture to the screen texture,
  the same cost as a stroke (0001 measurements: fine at 1920x1080, laggy at 8000x8000). The undo time budget is for
  1920x1080 only; the measurement task records it.
- **AS-9 "focus on a toolbox button".** GTK 4 cannot synthesize key presses. The test activates `win.undo` from the focused
  button, which is how an accelerator resolves; the real key press is manual check M3.
- **AS-13 vs pixels.** After steps 1..5 are dropped, the oldest reachable state has the saved pixels, but the spec says it
  stays modified (REQ-0002-14). The code follows the spec.

## Tasks
- [x] T001 Core history: steps, undo, redo, discard, state ids, saved state (REQ-0002-01, REQ-0002-03, REQ-0002-04, REQ-0002-05, REQ-0002-12, REQ-0002-13, REQ-0002-14; AS-1, AS-2, AS-4, AS-12) - done when the core tests as1, as2, as4 and as12 pass
- [x] T002 Budget: drop oldest steps, keep one oversized step, bytes accounting (REQ-0002-14, REQ-0002-16, REQ-0002-17; AS-13, AS-16, AS-17) - done when the core tests as13, as16 and as17 pass, including the 16 bytes per pixel + 64 bytes per step bound
- [x] T003 A stroke ends into the history; a no-change stroke adds nothing (REQ-0002-01, REQ-0002-02) - done when a core test shows a stroke of N changed pixels makes one step of N entries, a no-change stroke makes none, and the 0001 stroke tests still pass
- [x] T004 Budget setting and the new settings file section (REQ-0002-18, REQ-0002-19; AS-18, AS-19) - done when the core test as18 and the io tests as19_new_file_has_history and as19_existing_file_untouched pass
- [x] T005 Undo and Redo in the window: actions, header buttons, shortcuts, sensitivity, title follows the history (REQ-0002-02, REQ-0002-03, REQ-0002-04, REQ-0002-06, REQ-0002-07, REQ-0002-08, REQ-0002-10, REQ-0002-11, REQ-0002-12, REQ-0002-13; AS-1, AS-2, AS-3, AS-4, AS-5, AS-6, AS-7, AS-10, AS-11) - done when the ui tests as1, as3, as4, as5, as6, as10 and as11 pass and manual M1, M2 pass
- [x] T006 Undo during a stroke; shortcuts with focus elsewhere and while dialogs are open (REQ-0002-09, REQ-0002-11; AS-8, AS-9) - done when the ui tests as8 and as9 pass and manual M3 (real Ctrl+Z with focus on a toolbox button) passes
- [x] T007 Saved state across saves: failed save, save during a stroke (REQ-0002-14, REQ-0002-15; AS-14, AS-15) - done when the ui tests as14 and as15 pass
- [x] T008 Measure undo and redo against the budget (REQ-0002-03, REQ-0002-04; engineering.md "Undo / redo") - done when the benchmark perf-0002 runs and its numbers are recorded under Plan / Measurements
- [x] T009 Close the gaps /sdd-check 0002 found (REQ-0002-09, REQ-0002-14, REQ-0002-18; AS-8, AS-13, AS-18) - done when ui `as18_budget_reaches_history`, core `as13_dropped_start_state` and ui `as8_focus_loss_unblocks_undo` pass, each of the first two fails under its mutation, and manual M4 passes
