# 0035 Remove background

Status: accepted
Reviewed: spec-reviewer, 2026-11-09
Depends on: 0005, 0013, 0018, 0021, 0023, 0024, 0034

## Why
A common edit is to cut a subject (a person, a pet, an object) out of its background, for a sticker, a logo or a photo on another
background. Windows 11 Paint does it in one command with no settings, the subject found automatically. JOD (2026-11-07) wants the
same, as a modern extra beyond XP (`docs/phase2-plan.md` F). Decided on 2026-11-09: hard edges, a cancellable progress dialog, one
message when nothing would be removed, every main subject kept, a free-form selection judged by its shape alone, and the same pixels
on one machine (an exception to P7.4 between machines). The subject is found by a model that runs on the machine and ships with the
app, since the Flatpak has no network (REQ-0024-10). An ADR chooses it before the Plan: its license (compatible with
GPL-3.0-or-later, its training data noted), its size in the download, where it runs given that `src/core` is libc only, its time
and memory on this machine (the budgets of REQ-0035-09, which JOD accepts there), and the line P7.4 gains in the constitution.

## Requirements
Throughout: "removed" means made fully transparent (alpha 0) with the color channels kept; every other pixel is left exactly as it
was (P4: hard edges, no partly transparent pixel is made). "The pixels it looks at" are the whole picture's, or the selection's as
shown (only its shape's for a free-form one, the rest of its box not looked at).

### The command
- REQ-0035-01 (Adds to REQ-0018-01, -02.) The Image menu shall hold Remove Background right after Black and White…, with no shortcut; the selection menu (REQ-0013-17) holds it right after its Black and White…. It follows REQ-0018-01's rules for when an item acts. Its strings (the item, the progress dialog's heading and Cancel, the headings and bodies of REQ-0035-05 and -06) are translated in every language of 0023, the item's mnemonic differing from the others of its menu.
- REQ-0035-02 Choosing it shall first do what an Image action does to what waits (REQ-0018-03): a pending curve, polygon or gradient and an open text box are put down, and stay put down whatever follows; a text box whose put-down cannot get its memory stays open and nothing more happens. Then it shall act on the selection while one exists, and on the whole picture otherwise. A marked selection is lifted first (if the lift cannot get its memory, REQ-0013-19's dialog and the selection stays marked); a floating one is acted on as shown, as REQ-0034-03 says for Black and White (the result becomes its pixels at its shown size with no flip or turn left over; a pasted one's fully transparent pixels stay not put down); only the pixels it looks at can be removed; the result floats and joins the selection's one undo step. On the whole picture it is one undo step that costs what REQ-0002-16 allows for the pixels it changes (a step larger than the budget is kept as the only one, REQ-0002-17).
- REQ-0035-03 The app shall find every main subject in the pixels it looks at (people, animals, objects: what a person would call the main things, a person and their dog both) and remove every pixel it judges background, with no setting. A pixel already fully transparent counts as background and stays as it is.

### Waiting
- REQ-0035-04 If the work takes longer than 0.5 s, the app shall show a modal dialog with the heading "Removing the Background", a progress bar (showing the share done where it is known, pulsing where it is not, moving at least once a second) and Cancel. From the choice of the command until the work ends, the window takes no input except through that dialog: the picture, the selection, the tool, the colors and the settings stay as they were when it was chosen. Cancel, Esc, Ctrl+Q, Ctrl+W or a close request stop the work within 0.5 s: nothing of the removal is applied and no step is added for it; what REQ-0035-02 put down stays put down; a selection is as before the command, marked again if the command lifted it; a close request then goes on as REQ-0001-34 and REQ-0001-35 say (the Save prompt answered Cancel leaves the window with the picture as before and starts nothing again). A Cancel that comes after the result is shown does nothing; Undo is then the way back. The dialog closes by itself when the work ends.

### When nothing is removed
- REQ-0035-05 If no pixel would change (no subject is found, everything is judged subject, or the background is already transparent), the app shall show a dialog with the heading "Nothing to Remove" and the body "No background was found to remove.", change nothing, add no step and no modified mark; a marked selection it lifted is marked again (unlike REQ-0034-03, since nothing happened and the dialog says so; JOD, 2026-11-09). Pixels that all have one color (all four channels the same), or that are narrower or shorter than 16 pixels, give this dialog at once, without the subject being looked for.

### Failures
- REQ-0035-06 If the memory it needs cannot be had, at whatever point of the work, the app shall show the heading "Could Not Remove the Background" with the body "There is not enough memory for this change." and change nothing of the removal (what was put down stays put down; a selection as before the command, marked again if the command lifted it). If the model that finds the subject is missing from the installation, cannot be loaded or fails while it runs, the same heading with the body "Background removal is not available." and nothing changed in the same way. Ctrl+Q, Ctrl+W or a close request while one of these dialogs or REQ-0035-05's is open closes it, then goes on as REQ-0001-34 and REQ-0001-35 say (as REQ-0021-03).

### The pixel contract and cost
- REQ-0035-07 (P7, with an exception to P7.4 in the model's ADR and the constitution.) Kept pixels are exactly as they were and removed ones keep their color with alpha 0 (P7.1); a floating selection shows its result one to one (P7.3). The same pixels give the same result every time on the same machine with the same build, in any process; between machines the result may differ along the subject's edge (JOD's expectation: a few pixels), with no bound promised or tested.
- REQ-0035-08 It shall work with no network (the Flatpak has none, REQ-0024-10).
- REQ-0035-09 Its time, from the choice of the command to the result painted, the first run after the app starts (the model's loading) and a later run each, for an 8000 × 8000 and a 1920 × 1080 picture, and the memory it needs beyond the picture and its undo step, shall be within the budgets JOD accepts in the model's ADR, added to `docs/engineering.md`'s budgets (ADR-0006, 2026-11-09: 2 s for 1920 × 1080, 3 s for 8000 × 8000, 2 GB of memory).

## Scenarios
- AS-1 (REQ-0035-01) Given a picture, When the user opens the Image menu, Then Remove Background follows Black and White… with no shortcut; the selection menu has it after its Black and White…; while a button is held in the canvas it does nothing; its strings show in German | Verified by: automated (ui)
- AS-2 (REQ-0035-03, REQ-0035-07) Given each picture of the test set kept with this spec (`docs/reference/0035/`: at least five, each one or more subjects cut from openly licensed photos and laid on a plain or a gently varying background, so that each picture's true subject mask is known exactly; their licenses noted), When Remove Background, Then every pixel is either unchanged in all four channels or alpha 0 with its color channels kept; counting only pixels farther than 4 pixels from the true mask's edge, in each picture at least 95 % of the background pixels are removed and at least 95 % of the subject pixels are kept; one undo step, and Undo gives the picture back exactly; a second run in a new process gives the same pixels | Verified by: automated (with the model) + manual M1 (JOD's own photos)
- AS-3 (REQ-0035-02, REQ-0035-07) Given a test-set picture and a marked rectangle selection of its left half, When Remove Background, Then only pixels inside the selection may change, the selection floats with the result shown one to one, and one Undo after its put-down gives the picture back; Given a free-form selection, Then only its shape's pixels may change and what lies outside the shape does not change the result; Given a floating selection stretched to twice its size, Then the result is made on what is shown; Given a pending curve, Then it is put down first and stays so | Verified by: automated (ui)
- AS-4 (REQ-0035-04) Given an 8000 × 8000 test picture, When Remove Background, Then the progress dialog shows with its heading, bar and Cancel; When Cancel is pressed, Then within 0.5 s it closes, the picture is unchanged and no step is added; the same with Esc; with a marked selection, it is marked again; Given a modified picture and the dialog showing, When Ctrl+Q, Then the work stops with nothing changed and the Save prompt shows; Cancel there leaves the picture as before; the same with Ctrl+W | Verified by: automated (ui)
- AS-5 (REQ-0035-05) Given a 300 × 200 picture of one color, one fully transparent, and one 15 pixels wide, When Remove Background, Then "Nothing to Remove" shows at once, nothing changes and no step or mark is added; with a marked selection of the one-color picture, it is marked again; Given a test picture whose background was already removed, Then the same dialog | Verified by: automated (ui)
- AS-6 (REQ-0035-06) Given memory made too small, When Remove Background on the whole picture, on a marked selection and on a floating one, Then the dialog "Could Not Remove the Background" with the memory body shows and nothing of the removal changes (the marked one marked again, the floating one as it was); Given the model missing from the installation, Then the same heading with "Background removal is not available."; Ctrl+Q with that dialog open closes it and goes on | Verified by: automated (ui)
- AS-7 (REQ-0035-08, REQ-0035-09) Given an 8000 × 8000 and a 1920 × 1080 test picture, When Remove Background runs (a first run after start and a later one), Then its time and memory are within the ADR's budgets; in the Flatpak, with no network | Verified by: automated (perf, in the real session) + manual (the Flatpak, at the packaging release)

## Edge cases
- A picture with partly transparent pixels: judged as any other and kept or removed whole; none is made partly transparent.
- A subject that touches the picture's edges: kept as found; the edge does not count as background.
- A long, thin picture (8000 × 16): looked at as any other; narrower than 16 pixels gives REQ-0035-05 at once.
- Saving afterwards: PNG, TIFF and WebP keep the transparency, JPEG and BMP flatten it on white (REQ-0005-11), nothing new.
- The selection moved partly outside the picture: the selection as shown is looked at, its off-picture part too.
- Remove Background twice: the second removes nothing more and gives REQ-0035-05.

## Out of scope
- Settings or a slider (tolerance, softness), soft edges (decided: hard), choosing the subject by a click, replacing the background with a color or a picture, a model downloaded later, a bound on the difference between machines.

## Open questions
None. Settled by JOD on 2026-11-07 (Windows 11 Paint's one command, the selection's area only, one undo step) and 2026-11-09 (hard edges; a progress dialog with Cancel; one message, "Nothing to Remove", a marked selection marked again; every main subject kept; a free-form selection judged by its shape; same pixels on one machine, an exception to P7.4 between machines; the budgets set in the model's ADR). Decided in the review (routine): the dialog only after 0.5 s and Cancel within 0.5 s; one color or under 16 pixels gives the message at once; the test set built from cut-out subjects so that its masks are exact, counted 4 pixels away from the edges; what was put down stays put down.

## Plan
(added by /sdd-build after the spec is accepted: approach, files, key interfaces, scenario-to-test map, risks)

## Tasks
(added by /sdd-build after the plan is agreed: `- [ ] T001 title (REQ-..., AS-...) - done when ...`)
(filled in when the task is ticked, a line under it: `  Result: ...`. A code task: the test that failed first and its message,
or `characterization: <why there was no red run>`. A manual step: one line of what JOD saw.)

## Check
(written by /sdd-check as its last step, about ten lines)
- Date and commit: YYYY-MM-DD, `abc1234`
- Tests: `meson test` N of N pass (the cases of this feature: M)
- Mutants: K killed, C killed by crash (test gap), S survived, as `tools/mutate.sh` reports (seconds)
- Warnings: 0
- Scenarios: each AS with its test, marked C for a characterization with how it was proven (the mutant or the manual break that killed it)
- Findings, each with how it was settled (fixed in T0NN, decided by the owner, left open and why)
