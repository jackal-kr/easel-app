# 0009 View Polish

Status: accepted
Depends on: 0001, 0004, 0007, 0008

## Why
A picture smaller than the canvas area sits flush against the toolbox and the top edge, on the same color as the panels,
so the picture does not read as a sheet on a desk. Reference measurements from JOD (2026-10-18): the workspace around the
picture is a light grey, #E7E7E7, the picture is 12 px from the toolbox and the top edge, and a soft shadow about 6 px
wide surrounds it. This feature gives the picture its margin and shadow, and makes the workspace color a setting.

## Requirements
- REQ-0009-01 The picture shall lie 12 logical pixels from the left and from the top edge of the canvas area when the view is at its left and top (scrolled to 0), and the scrollable area shall hold the same 12 pixels after its right and bottom edges, so that scrolled to the far right or bottom there is 12 pixels between the picture and the edge of the canvas area. The margin is the same at every zoom; it is part of the workspace, not of the picture (it is never saved and a press on it is a press outside the picture).
- REQ-0009-02 A soft shadow about 6 pixels wide shall surround the picture on all four sides, darkest at the picture's edge and fading to nothing, drawn in the margin and never over the picture, never saved, and shown whatever the zoom. The resize handles (REQ-0004-16) are drawn over it, and the shadow does not change what a handle or the picture is hit by.
- REQ-0009-03 The workspace, that is the canvas area outside the picture including its margin, shall be drawn in the workspace color, #E7E7E7 unless the settings file says otherwise. The toolbox, the color box and the status bar keep the theme's colors.
- REQ-0009-04 The workspace color shall be a setting: section `[view]`, key `workspace-color`, a value of exactly the form `#rrggbb` (a `#` and six hexadecimal digits, upper or lower case; syntax of the file as REQ-0001-40). A missing key and any other value (`#fff`, `E7E7E7`, `#E7E7E7FF`, `red`, an empty value) mean the default, silently. The app never rewrites the file for it (as REQ-0004-15).
- REQ-0009-05 Every rule that relates a point of the canvas area to a picture pixel shall count the margin, so that a pixel is where it is drawn: the pixel under the pointer for every tool, the zoom anchors of the wheel, the pinch and the zoom commands (REQ-0004-06), Fit to Window (REQ-0004-07), the Magnifier's focus and its preview box (REQ-0007-03, REQ-0007-04), the pixel grid, and the resize handles and their drag.
- REQ-0009-06 While the dark theme is in use and the settings file holds no valid `workspace-color`, the workspace shall be the dark theme's own background (about #2C2C2C today) and not #E7E7E7; a valid `workspace-color` applies, as written, in the light and in the dark theme. Changing the theme while the app runs changes the workspace at once when no valid `workspace-color` is set.

- REQ-0009-07 While Pan is active, a drag with the primary button that starts in the margin or in the shadow shall pan the view, as REQ-0001-14 says of any point of the canvas area outside the picture, with no gap between the picture's edge, the shadow and the margin where it does not. With any other tool, a press there does nothing (REQ-0009-01).

## Scenarios
- AS-1 (REQ-0009-01, REQ-0009-03) Given a 300 × 200 picture at 100 % in a canvas area of 1390 × 853, Then the picture's top-left pixel is 12 pixels from the area's left edge and 12 from its top edge, the rest of the area is #E7E7E7, and the area to the picture's right and below it shows no picture; Given a 3000 × 2000 picture scrolled to the far right and bottom, Then the picture's last pixel column is 12 pixels from the area's right edge (not counting a scrollbar) and its last row 12 from the bottom edge | Verified by: automated (ui, by rendering the window) + manual M1
- AS-2 (REQ-0009-02) Given the same picture, Then a shadow about 6 pixels wide, darker next to the picture and fading outward, shows on its top, left, right and bottom edges, in the margin, and the picture's own pixels are the picture's colors; the saved file has no shadow and no margin | Verified by: automated (ui, a rendered row and column across each edge; core for the saved picture) + manual M1 (the look)
- AS-3 (REQ-0009-04) Given `[view]` with `workspace-color=#3465A4`, When the app starts, Then the workspace is #3465A4; `#3465a4` gives the same; `3465A4`, `#346`, `#3465A4FF`, `blue` and an empty value give #E7E7E7; a missing key gives #E7E7E7; the settings file is not rewritten | Verified by: automated (core, parsing) + automated (ui)
- AS-4 (REQ-0009-05) Given a 300 × 200 picture at 100 % with Pencil, When the user presses at 12 pixels from the area's left and top edge (the picture's first pixel), Then pixel (0, 0) is painted; a press 11 pixels from the left does nothing (the margin); at 400 % with the picture scrolled by more than a page, the pixel painted is the one drawn under the pointer | Verified by: automated (ui)
- AS-5 (REQ-0009-05, REQ-0004-07) Given a 3000 × 2000 picture and a canvas area of 1390 × 853, When the user chooses Fit to Window, Then the zoom is 41 % (the picture and its two margins, 3000 × 0.41 + 24 and 2000 × 0.41 + 24, fit; 42 % would not), where without the margin it was 42 % | Verified by: automated (core, fit level) + automated (ui)
- AS-6 (REQ-0009-05) Given a zoom command, the wheel or the Magnifier at a pointer position, Then the picture pixel under the pointer (or the Magnifier's focus) is the same pixel before and after, as REQ-0004-06 and REQ-0007-03 say, within one device pixel, at 100 %, at 400 % scrolled, and at 41 % | Verified by: automated (ui)
- AS-7 (REQ-0009-05, REQ-0004-16) Given Pencil and a 300 × 200 picture, Then the three handles touch the picture's right middle, bottom middle and bottom-right corner (inside the margin), a drag of the corner handle by 40 pixels adds 40 pixels at 100 %, and a press on the shadow beside a handle's hit area does nothing | Verified by: automated (ui)
- AS-8 (REQ-0009-05, REQ-0004-12) Given the grid on at 400 %, Then the lines lie on the picture's pixel edges, shifted by the margin, at every scroll position | Verified by: automated (ui, as AS-10 of 0004)

- AS-9 (REQ-0009-06, REQ-0009-04) Given the dark theme and no `workspace-color`, When the app starts, Then the workspace is the dark theme's background and not #E7E7E7; Given `workspace-color=#3465A4`, Then it is #3465A4 in the light and in the dark theme; Given an invalid value, Then it is the theme's default for each theme (#E7E7E7 in the light theme); When the theme changes while the app runs, Then the workspace follows it unless a valid value is set | Verified by: automated (ui, both themes) + manual M1

- AS-10 (REQ-0009-07) Given a 300 × 200 picture at 100 % in a canvas area of 1390 × 853 scrolled a little (or a 3000 × 2000 picture at 400 %), Pan active, When the user presses the primary button in the margin 6 pixels from the picture's top-left corner (on the shadow), drags 50 pixels and releases, Then the view moved by 50 pixels as a drag on the picture would move it, and no pixel and no modified state changed; the same drag started 11 pixels from the area's left edge pans too; with Pencil, the same press does nothing | Verified by: automated (ui, `as10_pan_from_the_margin`) + manual M1

## Edge cases
- A picture smaller than the area at any zoom: the top-left margin and shadow show, and nothing scrolls (REQ-0001-11, REQ-0004-06 now say "at the margin").
- A picture exactly as big as the area minus the margins: no scrollbars.
- 1 × 1 picture: the shadow is drawn around one pixel.
- The Magnifier at an edge of the picture: its preview box and zoom focus keep the rules of REQ-0007-03; "the view goes to that edge" means scrolled to its start or end, margin included.
- A very large picture (8000 × 8000 at 800 %): the margin adds 24 logical pixels to the scrollable size and nothing else; no new allocation per frame.
- The window or the text size changes: the margin and the shadow stay 12 and 6 logical pixels.

## Out of scope
- A setting for the margin or the shadow, and a color picker for the workspace (Preferences).
- Anything but the canvas area: the toolbox, color box and status bar colors, and their margins.
- The Pan right-click menu and other menus.

## Affected rules (pointer notes to add to the earlier specs when this one is accepted)
- Rule REQ-0001-11 (a picture smaller than the view sits at the top-left of the canvas area): at the margin, 12 pixels from the left and the top.
- Rule REQ-0004-06 (zoom commands keep the center still; a smaller picture stays at its top-left): the same, counting the margin.
- Rule REQ-0004-07 (Fit to Window): the picture plus its margins must fit the area, so the whole percent can be one lower (AS-5).
- Rule REQ-0004-16 (resize handles touch the picture, scroll with it, the scrollable area includes them): they lie in the margin; the scrollable area is the picture, the handles and the margin.
- Rule REQ-0007-03 and REQ-0007-04 (the Magnifier's focus, scroll limits and preview box) and REQ-0004-12 (the grid's lines): counted from the picture's own origin, which is 12 pixels in.
- Rule REQ-0001-14, REQ-0001-19 (a pan drag outside the picture, the pixel under the pointer): a point in the margin or the shadow is outside the picture.

## Open questions
None. Settled by JOD on 2026-10-18: in the dark theme the default is the theme's own dark background and a valid `workspace-color` wins in both themes (REQ-0009-06); the margin is 12 pixels on all four sides (REQ-0009-01); a Pan drag that starts in the margin pans (REQ-0009-07).

## Plan
(added by /sdd-build after the spec is accepted: approach, files, key interfaces, scenario-to-test map, risks)

Decided before the Plan, by JOD on 2026-10-18, to be built on and not reopened:
- **Approach B, a widget margin around the canvas.** The canvas keeps its origin at the picture, so the pointer-to-pixel mapping, the handles and the drawing stay in picture-origin coordinates and the existing ui tests, which pass canvas coordinates, stay valid. One constant, `EASEL_VIEW_MARGIN` (12 logical pixels) in `src/core/view.h`, is used in three places only: the margin of the canvas inside the scrolled area, the two core functions that relate a point of the view to a point of the canvas (`easel_view_anchor` and `easel_view_focus_scroll`), and `easel_view_fit_zoom` (the area less two margins).
- **Pan from the margin (REQ-0009-07).** Input comes from the scrolled area, not from the canvas widget, so a press in the margin already arrives at the canvas with negative coordinates (as a press on the grey area around a small picture does). The Plan checks that with a test first (`as10_pan_from_the_margin`), and if the margin or the shadow does not reach the pan gesture, the pan is attached to the scrolled area.
- The shadow and the workspace color are drawn by the theme's style system (a box shadow on the canvas, a background on the scrolled area set from the setting), not per frame by the canvas.

## Tasks
(added by /sdd-build after the plan is agreed: `- [ ] T001 title (REQ-..., AS-...) - done when ...`)
(filled in when the task is ticked, a line under it: `  Result: ...`. A code task: the test that failed first and its message,
or `characterization: <why there was no red run>`. A manual step: one line of what JOD saw.)

## Check
(written by /sdd-check as its last step, about ten lines)
- Date and commit: YYYY-MM-DD, `abc1234`
- Tests: `meson test` N of N pass (the cases of this feature: M)
- Mutants: K of K killed (seconds); none killed only by a crash
- Warnings: 0
- Findings, each with how it was settled (fixed in T0NN, decided by the owner, left open and why)
