# 0001 Walking skeleton

Status: draft
Owner: JOD
Depends on: none

## Why
The thinnest end-to-end path through the app: open it, draw, keep the result, leave safely. It proves the whole
loop (spec to shipped behavior) once, and every later feature grows from it. Users get nothing smaller that is still useful.

## User stories
- US1 (P1): As a user, I want the app to open on a blank picture, so that I can start drawing at once.
- US2 (P1): As a user, I want to draw freehand with a pencil, so that I can make a quick sketch.
- US3 (P1): As a user, I want to save my picture as PNG, so that I can keep and share it.
- US4 (P1): As a user, I want to be warned before losing unsaved work, so that quitting is safe.

## Requirements

### Start (US1)
- REQ-0001-01 When the app starts without arguments, the app shall show one window with a new blank picture filled with opaque white.
- REQ-0001-02 When the app starts, the app shall show the picture at 100 % zoom (1 picture pixel = 1 device pixel), with the pencil as the active tool (pending open question 2), black as the foreground color and white as the background color.
- REQ-0001-03 While a picture has never been saved, the window title shall show "Untitled"; after a save it shall show the file name.
- REQ-0001-04 While the picture has changes that are not saved, the window title shall show a modified mark.

### Pencil (US2)
- REQ-0001-05 When the user presses the primary button on the picture with the pencil, the app shall set the one pixel under the pointer to the foreground color.
- REQ-0001-06 While the user drags with the primary button held, the app shall draw a 1-pixel-wide line through the pointer positions with no gaps between consecutive positions and no antialiasing.
- REQ-0001-07 When the user presses or drags with the secondary button, the app shall draw as in REQ-0001-05/06 but with the background color.
- REQ-0001-08 If the user presses the other mouse button during a stroke, then the app shall cancel the stroke and restore the picture to how it was before the stroke began.
- REQ-0001-09 While a stroke continues outside the picture, the app shall keep drawing the parts of the line that fall inside the picture and ignore the rest.
- REQ-0001-10 When a stroke changes at least one pixel, the app shall mark the picture as modified.

### Save (US3)
- REQ-0001-11 When the user chooses Save (Ctrl+S) on a picture that has never been saved, the app shall ask for a file name and location, and save as PNG.
- REQ-0001-12 When the user chooses Save on a picture that already has a file, the app shall overwrite that file without asking.
- REQ-0001-13 When a save succeeds, the app shall clear the modified mark; the saved file shall reopen in another image viewer with exactly the same size and pixels.
- REQ-0001-14 If saving fails, then the app shall show an error that names the file and the reason, keep the picture unchanged and keep it marked modified.
- REQ-0001-15 If the user cancels the file dialog, then the app shall save nothing and change nothing.

### Quit (US4)
- REQ-0001-16 When the user quits (Ctrl+Q or closing the window) and the picture is not modified, the app shall close at once.
- REQ-0001-17 When the user quits and the picture is modified, the app shall ask with three choices: Save, Discard (labelled with the action), Cancel.
- REQ-0001-18 When the user chooses Save in the quit prompt, the app shall save as in REQ-0001-11/12 and close only if the save succeeds.
- REQ-0001-19 When the user chooses Discard, the app shall close without saving; when the user chooses Cancel or dismisses the prompt, the app shall stay open with the picture unchanged.

## Acceptance scenarios
- AS-1 (REQ-0001-01, REQ-0001-02) Given the app is not running, When it starts, Then one window shows a white picture of the default size at 100 %, pencil active, foreground black, background white | Verified by: automated (ui) + manual M1
- AS-2 (REQ-0001-03, REQ-0001-04) Given a fresh start, When the user draws one dot, Then the title reads "Untitled" with the modified mark; When the user saves as "a.png", Then the title reads "a.png" without the mark | Verified by: automated (ui)
- AS-3 (REQ-0001-05) Given a white picture, When the user primary-clicks at (10,10), Then pixel (10,10) is black and no other pixel changed | Verified by: automated (core)
- AS-4 (REQ-0001-06) Given a white picture, When the user drags from (0,0) to (20,5) in one motion event, Then a connected 1-pixel line of pure black joins both points and every other pixel is pure white | Verified by: automated (core)
- AS-5 (REQ-0001-07) Given a picture filled black, When the user secondary-drags across it, Then the touched pixels become white | Verified by: automated (core)
- AS-6 (REQ-0001-08) Given a picture with an earlier stroke, When the user primary-drags and then presses the secondary button before releasing, Then the picture equals its state before the cancelled stroke and the modified state is unchanged | Verified by: automated (core)
- AS-7 (REQ-0001-09) Given a 100x100 picture, When the user drags from (50,50) to (150,50), Then pixels (50..99,50) are black and nothing fails | Verified by: automated (core)
- AS-8 (REQ-0001-10) Given an unmodified picture, When a stroke changes no pixel (black on black), Then the picture stays unmodified | Verified by: automated (core)
- AS-9 (REQ-0001-11, REQ-0001-13) Given a new picture with a drawing, When the user presses Ctrl+S and chooses "a.png", Then a.png exists, decodes to the same size and pixels, and the mark is cleared | Verified by: automated (core round-trip) + manual M2
- AS-10 (REQ-0001-12) Given a picture saved as a.png, When the user draws and presses Ctrl+S, Then no dialog appears and a.png holds the new pixels | Verified by: automated (ui)
- AS-11 (REQ-0001-14) Given a modified picture, When the user saves into a read-only folder, Then an error names the file and the reason, and the picture stays modified | Verified by: automated (core) + manual M3
- AS-12 (REQ-0001-15) Given a modified untitled picture, When the user presses Ctrl+S and cancels the dialog, Then no file is written and the title is unchanged | Verified by: manual M4
- AS-13 (REQ-0001-16) Given an unmodified picture, When the user presses Ctrl+Q, Then the app closes with no prompt | Verified by: manual M5
- AS-14 (REQ-0001-17, REQ-0001-19) Given a modified picture, When the user closes the window, Then a prompt offers Save, Discard and Cancel; Cancel keeps the app open unchanged; Discard closes it and writes no file | Verified by: manual M6
- AS-15 (REQ-0001-18) Given a modified untitled picture, When the user chooses Save in the quit prompt and then cancels the file dialog, Then the app stays open and modified; When the save succeeds instead, Then the app closes | Verified by: manual M7

## Edge cases and errors
- Click exactly on the last row or column; drag that leaves and re-enters the picture (the outside part is not drawn, the line resumes on re-entry without a jump across).
- Very fast drags: no gaps (REQ-0001-06).
- Save to an existing file through the dialog: the dialog's own overwrite confirmation applies.
- Disk full or file removed mid-save: same as REQ-0001-14; an existing file must not be left half-written.
- Quit requested twice while the prompt is open: one prompt only.

## Out of scope
Undo/redo (0002), palette and color choice (0003), zoom, scroll, grid, resizing the picture (0004), Open, Save As,
other formats, command-line files (0005), every tool except the pencil (0006+), Preferences (0013).

## Open questions
- [NEEDS CLARIFICATION: Default picture size? Options: fixed (e.g. 640x480 or 800x600), XP-like (derived from screen size), or jspaint's 400x300. 0013 will make it a preference later.]
- [NEEDS CLARIFICATION: new-behaviors.md makes Pan the startup tool, but Pan arrives later. For 0001: pencil active at start (and change the default when Pan lands), or ship a minimal Pan now?]
- [NEEDS CLARIFICATION: Does the skeleton show a toolbox with only the pencil, or no toolbox at all until 0006?]
- [NEEDS CLARIFICATION: Modified-mark style: GNOME-style "• Untitled" / "*Untitled", and does the title include the app name ("Untitled – Easel")?]
- [NEEDS CLARIFICATION: Window bigger than the picture: picture at top-left (XP) or centered (GNOME image apps)? And picture bigger than the window: clipped until scrolling arrives in 0004, or scrollable now?]
- [NEEDS CLARIFICATION: Should a pencil click or drag show the cursor/pointer position in a status bar now, or is the status bar 0004/0014?]

## Behavior reference
- `docs/reference/xp-behaviors.md`: second button cancels a stroke; right button draws with the background color.
- `docs/design/interaction-model.md` (pointer, pending state), `docs/design/ui-ux.md` (labelled destructive actions, header bar).
- XP Paint: pencil is 1 px, hard-edged; title "untitled - Paint"; quit prompt "Save changes to untitled?" Yes/No/Cancel.
