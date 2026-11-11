# 0026 Print

Status: implemented
Reviewed: spec-reviewer, 2026-11-03
Depends on: 0001 (quit, settings), 0005 (file menu, flattening), 0011, 0012, 0013, 0014, 0016, 0017 (what a command puts down), 0018 (keys on other layouts), 0019 (number fields), 0020, 0021 (gates, Keyboard Shortcuts, Help)

## Why
A picture made or touched up here often ends on paper. XP Paint prints from its File menu and sets the page in Page Setup; Easel needs the same, and a choice of how a picture's pixels look on paper, because paper is finer than most pictures (JOD, 2026-11-03).

How it meets P7: P7 binds the picture and the view, and paper is neither. Printing writes no picture pixel (P7.1, P7.5), the view has no effect on the page (P7.2), pending pixels are put down first, so a page holds only written pixels (P7.3), and the same input gives the same page (P7.4, REQ-0026-11). The Pixels choice is the user's and acts only on paper.

## Requirements
### The items
- REQ-0026-01 The primary menu's first section shall read New, Open…, Save, Save As…, "Print…" (Ctrl+P). The item does nothing before the picture exists, while a dialog, the quit prompt or the color chooser is open, while any mouse button is held in the canvas area, or while the focus is in a text field of the font card (as REQ-0021-01); Ctrl+P also does nothing while a menu is open, and the item acts from its own menu. Ctrl+P follows REQ-0018-01's rule for a layout that is not Latin. A key repeat of Ctrl+P does nothing.
- REQ-0026-02 Ctrl+P shall join the File group of Keyboard Shortcuts (REQ-0021-02), and the Help page (REQ-0021-04) shall name Ctrl+P and describe Print and its Picture tab.

### The Picture tab (JOD, 2026-11-03: no Page Setup item, the desktop's print dialog has the rest)
- REQ-0026-03 The desktop's print dialog shall hold a tab "Picture" with Pixels (Sharp | Smooth when shrunk | Smooth) and a switch "Fit on one page". Paper, orientation, scale, copies and range are the dialog's own.
- REQ-0026-04 It shall start with the desktop's default paper, Sharp, and Fit on one page on. The orientation follows the picture, Landscape when it is wider than tall, else Portrait, also after New, Open or a resize, until the user changes the orientation in the print dialog and confirms (JOD, 2026-11-03). What the dialog and the tab confirm is kept until the app quits and is not written to the settings file, a print being a matter of the moment (P6's settings file holds what lasts). Cancel keeps nothing.
- REQ-0026-05 The margins shall be 19 mm on all four sides, with no control. A paper that leaves no printable area is a failure of REQ-0026-15.
- REQ-0026-06 With Fit on one page on, a picture pixel shall be P/100 × 1/96 inch on a side, with the largest P not above 100 at which the picture fits one sheet, with no lower limit; the picture is never enlarged (`docs/reference/new-behaviors.md`: where XP starts at 100 %). With it off, P is 100. The dialog's own scale applies on top.
- REQ-0026-07 A picture larger than one printable area shall continue on further sheets, left to right and then top to bottom. Each sheet holds only whole picture pixels, as many columns and rows as fit, and the rest go on the next, so that no pixel is lost or repeated. Only sheets that hold picture pixels are printed, even if those pixels are all white or transparent.
- REQ-0026-08 The picture shall be centered in the printable area along each axis on which it takes one sheet; on several sheets along an axis it starts at the margin.

### Pixels on paper
- REQ-0026-09 Pixels shall choose how picture pixels reach the paper's dots, a pixel with transparency being flattened on white first (REQ-0005-11): Sharp, each dot takes the picture pixel at its center, so each pixel is a hard square; Smooth when shrunk, as Sharp where a dot covers less than one picture pixel on a side, and where it covers more, the dot is the average of the pixels it covers, by the area covered; Smooth, blended between neighboring pixels at every scale. The tooltips: "Each pixel a sharp square", "Sharp, but averaged where a printer's dot covers several pixels", "Blended between pixels".

### Printing
- REQ-0026-10 The tab shall show "Takes N sheets" whenever N is above one, following the paper, the orientation and the switch as they change, so that the count is seen before the print is confirmed (JOD, 2026-11-03: in place of a prompt, since the switch lies in the dialog).
- REQ-0026-11 The same picture, Page Setup, paper and printer resolution shall give the same page, whatever the zoom, the scroll and the state of the screen, and printing shall write no picture pixel.
- REQ-0026-12 (Pointer note, Extra shapes, 2026-11-11: Print puts a waiting shape down too, and stops if that fails, REQ-0043-10, -13.) "Print…" shall first end what is held as for Save: a waiting curve, polygon or gradient, a floating selection and an open box are put down (REQ-0011-12, REQ-0012-07, REQ-0013-11, REQ-0017-17), and a marked selection stays marked (REQ-0013-12). Unlike Save, if a put-down fails, its dialog shows and no print dialog opens: a gradient, curve or polygon has been dropped and a selection's pixels have gone back (REQ-0012-09, REQ-0011-15, REQ-0013-19), or a box stays open (REQ-0017-18). The app then opens the desktop's print dialog, with the paper and orientation of REQ-0026-04, the document name being the window's title without the modified mark and without " – Easel". Cancel in it prints nothing, and what was put down stays put down.
- REQ-0026-13 Scale, sheets and centering shall be worked out from the paper and orientation the print dialog confirms. If they leave no printable area, REQ-0026-15's dialog shows and nothing prints. The print dialog's copies and pages apply to that print; its paper, orientation (REQ-0026-04), printer and other choices are kept until the app quits.
- REQ-0026-14 The printed pixels shall be the picture's as they were when the print dialog was confirmed, with no overlay, grid, checkerboard or margin color. Apart from the put-down of REQ-0026-12, printing shall not change the picture, the undo history, the modified mark or the file. Edits made while a confirmed print is being sent do not reach it. A second Print… during sending is allowed, and each print is its own job.
- REQ-0026-15 If the print dialog cannot be shown, the print cannot be started or the printer system reports a failure before it has accepted the job, an error dialog shall show, heading "Could Not Print", body "The picture could not be printed.", one for each failed print, one after the other. Failures after it has accepted the job (out of paper) are the desktop's. A failure reported later waits until no button is held and no dialog is open (as REQ-0021-04). A failed print changes nothing in the app, what was put down staying put down. If the memory to keep the picture as confirmed cannot be had, the same dialog shows and nothing prints. A quit closes this dialog and goes on (as REQ-0021-03).
- REQ-0026-16 While the desktop's print dialog is open, a quit request shall do nothing, as for Open's dialog (REQ-0005-02), and the canvas takes no input (REQ-0010-17); a print dialog that goes away with no answer counts as Cancel. Once confirmed, the app goes on while the print is sent. When a quit comes during sending, the window closes as usual (with its prompt, if modified) and the app exits when what it is sending has been accepted; a failure from then on shows nothing (JOD, 2026-11-03).

## Scenarios
- AS-1 (REQ-0026-01) Given a picture, Then the first section reads New, Open…, Save, Save As…, Print… with Ctrl+P; Given a stroke held, the quit prompt, the color chooser, a menu, or the focus in the font card's size field, When Ctrl+P is used, Then nothing opens | Verified by: automated (ui) + manual M1 (the real key, also on a Ukrainian layout)
- AS-2 (REQ-0026-02) Given the app, When Keyboard Shortcuts opens, Then File lists Print… with Ctrl+P; the Help page names it and the Picture tab | Verified by: automated (ui)
- AS-3 (REQ-0026-03, REQ-0026-04) Given a start and an 800 × 600 picture, When Print… opens, Then the tab shows Sharp with its tooltips and Fit on one page on, and the orientation is Landscape; a 600 × 800 or square picture Portrait; When the tab and the dialog are changed and Cancel, Then nothing is kept; When confirmed with Portrait, Smooth and Fit off, Then they are what the next print starts with, and a wide picture stays Portrait; after a restart they are the starting values and the settings file has no such key | Verified by: automated (ui) + manual M1 (its look)
- AS-4 (REQ-0026-05) Given a paper with no printable area at 19 mm margins, When confirmed, Then the error dialog shows and nothing prints | Verified by: automated (core + ui)
- AS-5 (REQ-0026-06) Given A4 and Portrait (172 × 259 mm printable), When Fit on one page, Then a 600 × 400 picture prints at 100 %, 158.8 × 105.8 mm; an 800 × 600 at 81.3 %, 172 mm wide; a 3000 × 2000 at 21.7 %; a 1 × 1 at 100 %; a 10000 × 10000 on one sheet, at 6.5 %; When Landscape, Then the 800 × 600 prints at 100 % and the 3000 × 2000 at 32.5 %; When Fit off, Then the 3000 × 2000 prints at 100 % | Verified by: automated (core)
- AS-6 (REQ-0026-07) Given A4 Portrait with Fit off, When a 3000 × 2000 picture prints, Then it takes 5 × 3 = 15 sheets, in order, and every pixel lies on exactly one; a picture 650 wide takes one sheet across and one 651 wide takes two; an all-white picture on 2 sheets prints both | Verified by: automated (core)
- AS-7 (REQ-0026-08) Given a small picture, When printed, Then equal space lies at its left and right and at its top and bottom within the margins; Given a picture on 2 sheets across, Then it starts at the margin across and is centered down | Verified by: automated (core)
- AS-8 (REQ-0026-09, REQ-0026-11) Given a 2 × 2 picture of #FF0000FF, #00FF00FF, #0000FFFF and #00000000, printed to an image of 600 dots per inch at 400 %, When Sharp, Then each is a square of its exact color, the last white, with no other color; When Smooth, Then a dot midway between two squares differs from both; Given a checker of #000000FF and #FFFFFFFF pixels at 10 % and 300 dots per inch (a dot covering about 3.2 pixels), When Smooth when shrunk, Then dots are grays; When Sharp, Then only black and white; Given it at 50 % and 300 dots per inch (a dot under one pixel), Then Smooth when shrunk is Sharp's page; When printed twice, or at another zoom and scroll, Then the pages are equal; the picture's pixels are unchanged in each | Verified by: automated (ui, a page rendered to an image)
- AS-9 (REQ-0026-10) Given a 3000 × 2000 picture with Fit on, Then the tab shows no count; When Fit off on A4 Portrait, Then "Takes 15 sheets"; When the paper or the orientation changes in the dialog, Then the count follows (Landscape: 16); When Fit on again, Then no count | Verified by: automated (ui)
- AS-10 (REQ-0026-12) Given a waiting curve on an unmodified picture, When Print… then Cancel, Then it is put down, the title has the mark, one step is added and a quit prompts; Given a floating selection or an open box, Then each is put down before the dialog; Given a marked selection, Then it stays marked and the whole picture prints; Given a put-down that fails, for each of the four, Then its dialog shows and no print dialog opens | Verified by: automated (ui)
- AS-11 (REQ-0026-13) Given a picture that follows to Portrait, When the print dialog confirms Landscape, Then the scale is worked out for Landscape, and the next print starts Landscape; Given a paper with no printable area, Then the error dialog shows and nothing prints | Verified by: automated (core + ui)
- AS-12 (REQ-0026-14) Given a modified picture with nothing held, When printed, Then the mark and the undo steps are as before; Given an edit after the confirm, Then the page is of the picture as confirmed; the grid and checkerboard on screen do not reach the page; two prints sent together are two jobs | Verified by: automated (ui)
- AS-13 (REQ-0026-15) Given a printer system that fails, or a print dialog that cannot open, Then the error dialog shows and the picture is as it was; two failures show one after the other; Given a failure reported during a stroke, Then it waits for the release; Given no memory to keep the picture, Then the same dialog shows and nothing prints; a quit closes the dialog and goes on | Verified by: automated (ui)
- AS-14 (REQ-0026-16) Given the desktop's print dialog open, When a quit is requested, Then nothing happens; When it goes away with no answer, Then it is as Cancel; Given a print being sent, When the app quits, Then the window closes and the job still arrives; a failure then shows nothing | Verified by: automated (ui) + manual M2 (a real print or a print to file, each Pixels choice looked at, a quit during sending)

## Edge cases
- A picture of 1 × 1 at Fit to 1 by 1: one small square, 0.26 mm at 96 pixels per inch, not enlarged.
- A tall picture chosen Landscape simply shrinks more: the orientation never turns the picture.
- Under Flatpak the portal's print dialog may have no Picture tab: the print then uses Sharp and Fit on one page. Checked at packaging.
- No printer at all: the desktop's print dialog offers Print to File, or says none is there.
- The 19 mm margins are larger than most printers' own unprintable edge.
- A release lost during a failure's wait ends as REQ-0013-18 says, and the failure then shows.

## Out of scope
- Print Preview of the app's own (the system dialog's preview stands in); headers and footers; several pictures to a sheet; color management; printing only a selection; a Page Setup item (the print dialog has it); margins and centering controls; Fit to more than one page; writing the choices to the settings file.

## Open questions

## Plan
**Approach.** The numbers are pure core: from the picture's size, the printable area and Page Setup, the scale, the sheets and each sheet's pixels and place. A sheet is drawn by one function on any cairo context, so a test draws it on an image of a known resolution and the printer gets the same calls. The window owns the flow through `GtkPrintOperation`, with its page setup embedded and a custom tab (a probe: with no embedding the dialog's Orientation showed Portrait over a landscape sheet), and a seam for its dialog.

**Files.** New: `src/core/print-layout.{c,h}`, `src/ui/print.{c,h}` (the job, drawing a sheet, the Picture tab), `tests/core/test-print-layout.c`. Changed: `src/ui/window.{c,h,ui}`, `src/ui/application.c`, `src/ui/help.c`, `data/help/index.html`, `src/meson.build`, `tests/meson.build`, `tests/ui/test-window.c`, `tests/mutants.txt`.

**Key interfaces.**
- Core: `EaselPrintLayout easel_print_layout (int pic_w, int pic_h, double area_w_mm, double area_h_mm, bool fit, int percent)`: the percent, the pixels a sheet holds across and down, the sheets across and down. `void easel_print_sheet (const EaselPrintLayout *l, int pic_w, int pic_h, int index, int *x, int *y, int *w, int *h)`. `void easel_print_place (l, pic_w, pic_h, area_w_mm, area_h_mm, center_h, center_v, &x_mm, &y_mm)`: where a sheet's pixels start, with centering. `int easel_print_sheets (l)`. `bool easel_print_area_ok (...)`: REQ-0026-05's test. `bool easel_print_wide_is_landscape (int w, int h)`. `void easel_print_average (const uint32_t *src, int src_w, int src_h, size_t stride, double pixels_per_dot, uint32_t *dst, int dst_w, int dst_h)`: the area-weighted average of opaque pixels.
- Ui: `EaselPrintJob *easel_print_job_new (const EaselImage *picture, const EaselPrintPage *page)`: the flattened copy, NULL with no memory. `void easel_print_job_draw_sheet (job, cairo_t *cr, int index, double dots_per_inch)`. `easel_window_set_print_runner` (test hook, stands in for the dialog), then `easel_window_print_confirmed`, `_cancelled`, `_finished`; `easel_window_set_print_export` and `_set_print_copy_limit` (test hooks).

**Drawing a sheet.** Sharp draws the sheet's pixels with `CAIRO_FILTER_NEAREST`; Smooth with `CAIRO_FILTER_BILINEAR`; Smooth when shrunk, where a dot covers more than one pixel, first reduces the sheet's pixels to the dots by `easel_print_average`, then draws those with nearest.

**The one function.** The put-down: `easel_canvas_put_down_keeping_marked`, as Save, as every other command (REQ-0026-12), called before the dialog. The copy of the picture: `begin-print`, the one signal that says the dialog was confirmed.

**Data handed to another process.** GTK spools the pages to a file and hands that to the printer system; the app waits for nothing but `done`. The application is held from the confirm to `done`, so a quit closes the window and the process stays until then (REQ-0026-16). What a seam cannot see: the real dialog and the printer; M2 prints for real and to a file.

**Readers of what this extends.**
- `dialog_is_open` gains the print dialog's own flag, read by every gate; `easel_window_close_request` returns at it as at Open's.
- The primary menu's first section: 0005's `as1_file_menu`; `ui-0005`, `ui-0013`, `ui-0020` and `ui-0021` run after T003 (`docs/engineering.md`, What to run when).
- The key list: 0021's `as2_every_key_listed` and `as4_help_page_text`.

**What grows.** One flattened copy of the picture per job, width × height × 4 bytes, one allocation, freed at `done`; the sheet's reduced pixels while it is drawn, never more than the sheet's pixels. Neither is counted by the undo budget.

**Scenario map.**
| AS | Test |
|---|---|
| 1 | ui `as1_print_items`, `as1_print_gates`, manual M1 |
| 2 | ui `as2_print_in_shortcuts` |
| 3 | ui `as3_print_tab`, manual M1 |
| 4 | core `as4_room_and_orientation`, ui `as11_print_dialogs_paper` |
| 5 | core `as5_scale` |
| 6 | core `as6_sheets` |
| 7 | core `as7_place` |
| 8 | ui `as8_pixels_on_paper`, core `as8_average` |
| 9 | ui `as9_sheets_line` |
| 10 | ui `as10_put_down_first` |
| 11 | ui `as11_print_dialogs_paper` |
| 12 | ui `as12_print_changes_nothing` |
| 13 | ui `as13_could_not_print` |
| 14 | ui `as14_quit_and_sending`, `as14_real_operation_exports`, manual M2 |

**Constitution.** Core stays libc only; printing writes no picture pixel; P7 is met as Why says; no ADR.

**Risks.**
- R1 `GtkPrintOperation`'s export runs headless, calls `begin-print` and `draw-page`, and reports a resolution: T002 probes it before the code relies on it.
- R2 Cairo's nearest filter takes the pixel at a dot's center on an image surface and marks a PDF image not to be interpolated: T002 probes both. Fallback for PDF: scale the pixels up by a whole factor before drawing.
- R3 A print dialog's resolution can be low for a file (72 dots per inch makes Smooth when shrunk reduce hard): T002 reads it; if so the job uses at least 300 for the reduction.
- R4 A close request with the print dialog open: libadwaita's handler sees no dialog of its own, so `easel_window_close_request` runs and returns: `as14_quit_and_sending` in T006.
- R5 Under Flatpak the portal shows the dialog, perhaps with no custom tab: checked at packaging, with the defaults as the fallback.
- R6 The Picture tab: a probe shows GTK makes it and the dialog's Orientation follows the page setup once embedded; `custom-widget-apply` comes before `begin-print`: T009 relies on it, M1 and M2 see the real order.

## Tasks
- [x] T001 The layout: scale, sheets, place, room, orientation, average (REQ-0026-04, -05, -06, -07, -08, -09; AS-5, AS-6, AS-7) - done when the core tests pass, red first
  Result: `as5_scale` and the rest failed first by build (`print-layout.h: No such file`), then green. A 3000 × 2000 on A4 at Adjust to 100 % is 15 sheets, whole pixels.
- [x] T002 Probes R1 to R3, then drawing a sheet (REQ-0026-09, -11; AS-8) - done when `as8_pixels_on_paper` passes, red first, and the probes' results are here
  Result: slip of order: test and code in one step, so no red run of the test. Probes: export runs headless and calls `begin-print` and `draw-page` (R1); nearest gives hard squares on an image and the PDF is not marked to interpolate (R2); export reports 72 dots per inch, so reduction uses at least 300 (R3). Breaks prove it: nearest for Smooth fails at `mid != 0xFF00FF00`, no reduction at `grays > others`, an unflattened copy at the half-transparent pixel, after that first passed by coincidence and was changed to #40404080.
- [x] T003 The items, Ctrl+P, Keyboard Shortcuts and the Help page (REQ-0026-01, -02; AS-1, AS-2) - done when they pass, red first, with `ui-0005`, `ui-0013`, `ui-0020`, `ui-0021`
  Result: `as1_print_items` failed first on the section, `as2_print_in_shortcuts` on `found`; `ui-0005`, `ui-0013`, `ui-0020`, `ui-0021` green after.
- [x] T004 Page Setup (REQ-0026-03, -04, -05; AS-3, AS-4) - done when they pass, red first; ends with a look at the dialog
  Result: both failed first by build (`page-setup-dialog.h: No such file`). The dialog was then replaced at JOD's word, T009.
- [x] T005 Print: the prompt, the put-down, the dialog, the confirmed copy (REQ-0026-10, -12, -13, -14; AS-9, AS-10, AS-11, AS-12) - done when they pass, red first
  Result: red by build (`easel_window_set_print_runner`). A prompt's Print must wait for the prompt to close, which still counts as open; a shape's and a gradient's failure show their dialog later, so a flag says so: without it `as10_put_down_first` fails at `print_runs == 0`. Ctrl+P by place and its repeat in `as1_print_gates`. The real operation in `as14_real_operation_exports`.
- [x] T006 Failures and quits (REQ-0026-15, -16; AS-13, AS-14) - done when they pass, red first
  Result: red by build with T005. Breaks: without the print dialog in the close request `as14_quit_and_sending` fails at the window gone; without the closed-window test it fails at the alert count. `gtk_window_destroy` does not dispose a window still referenced, so a closed window is told by having no application.
- [x] T009 Page Setup out, the Picture tab in (JOD, 2026-11-03) (REQ-0026-01 to -06, -08, -10, -12, -13; AS-1 to AS-5, AS-7, AS-9, AS-11) - done when they pass, red first, with `ui-0005`, `ui-0013`, `ui-0020`, `ui-0021`
  Result: red by build (`too few arguments to easel_print_layout`, `easel_print_tab_new`), then green. Removed the dialog, its files and tests, the prompt, and Fit to W by H from the core. The tab is real in a probe; its order of signals and its look go to M1. `ui-0005`, `ui-0013`, `ui-0020`, `ui-0021` green after.
- [x] T007 Manual M1 (Ctrl+P on two layouts, the Picture tab's look and its sheet count, the dialog's Orientation, where the keys go after it closes) and M2 (a real print or a print to file with each Pixels choice, a quit during sending) - done when JOD has seen them
  Result: JOD, 2026-11-03, on the M1 and M2 steps for the Picture tab: "all steps are good". An earlier look at the real dialog led to T009: its Orientation read Portrait over a landscape sheet, fixed by embedding the page setup.
- [x] T010 `tools/mutate.sh --core` again, after T009 changed the core (REQ-0026-05 to -09) - done when its summary line is here
  Result: `417 mutants: 416 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 0 errors, 1 accepted in 113 s`.
- [x] T008 The core mutants, and `tools/mutate.sh --core` once, in the background (REQ-0026-05 to -09) - done when its summary line is here and each survivor is one line for Check
  Result: `418 mutants: 417 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 0 errors, 1 accepted in 112 s`. Twelve new mutants (scale, sheets, place, room, orientation, average); no survivor.

## Check
- Date and commit: 2026-11-03, uncommitted on `7a2138f`
- Tests: `meson test` 65 of 65 pass (the cases of this feature: 12), before F1's one-line fix and its test, after which `ui-0026` passes; scale 2: 3 of 3
- Mutants: `tools/mutate.sh --core`: 416 killed, 0 killed by crash (test gap), 0 survived, 1 accepted (113 s)
- Warnings: 0
- Scenarios: AS-1 `as1_print_items`, `as1_print_gates`, M1; AS-2 `as2_print_in_shortcuts`; AS-3 `as3_print_tab`, M1; AS-4 core `as4_room_and_orientation`, `as11_print_dialogs_paper`; AS-5 core `as5_scale`; AS-6 core `as6_sheets`; AS-7 core `as7_place`; AS-8 `as8_pixels_on_paper`, core `as8_average`; AS-9 `as9_sheets_line`, M1; AS-10 `as10_put_down_first`; AS-11 `as11_print_dialogs_paper`; AS-12 `as12_print_changes_nothing`; AS-13 `as13_could_not_print`; AS-14 `as14_quit_and_sending`, `as14_real_operation_exports`, M2. No characterization task; T002's slip of order is in its Result.
- Results: see each task's `Result:` under Tasks; every task has one.
- Findings:
  - F1 (user-visible, rare) `print_error_timer_fired`: after a quit with a failure still waiting and another job keeping the window referenced, it showed the dialog on a closed window. Fixed. `as14_quit_and_sending`'s first form passed with the fix out, a held pan blocking the dialog by itself; with the pan released it fails at the alert count with the fix out, and passes with it.
  - F2 (drift) `docs/specs/INDEX.md` said "Print the picture, with page setup". Fixed.
  - F3 (not settled by a run) `run_print` handles a result of ERROR at the run with no `done`, and then keeps its references on the operation and the window; whether GTK can answer so is unknown, and the path has no test. Accepted, known gap.
- Known gaps (no test of their own, also in `docs/phase2-plan.md`): the document name, without the mark and " – Easel"; the quit prompt, color chooser and open menu as gates of Print…; a print dialog that cannot be shown; a quit during sending with a modified picture; another zoom and scroll in AS-8 (the job reads no view, by construction); the title's mark in AS-10 (the modified flag is what is tested); the Picture tab under Flatpak, at packaging.
- Survivors: none.
