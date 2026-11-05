# 0009 View Polish

Status: implemented
Depends on: 0001, 0004, 0007, 0008

## Why
A picture smaller than the canvas area sits flush against the toolbox and the top edge, on the same color as the panels,
so the picture does not read as a sheet on a desk. Reference measurements from JOD (2026-10-18): the workspace around the
picture is a light grey, #E7E7E7, the picture is 12 px from the toolbox and the top edge, and a soft shadow about 6 px
wide surrounds it. This feature gives the picture its margin and shadow, and makes the workspace color a setting.

## Requirements
- REQ-0009-01 The picture shall lie 12 logical pixels from the left and from the top edge of the canvas area when the view is at its left and top (scrolled to 0), and the scrollable area shall hold the same 12 pixels after its right and bottom edges, so that scrolled to the far right or bottom there is 12 pixels between the picture and the edge of the canvas area. The margin is the same at every zoom; it is part of the workspace, not of the picture (it is never saved and a press on it is a press outside the picture).
- REQ-0009-02 A soft shadow about 6 pixels wide shall surround the picture on all four sides, darkest at the picture's edge and fading to nothing, drawn in the margin and never over the picture, never saved, and shown whatever the zoom. The resize handles (REQ-0004-16) are drawn over it, and the shadow does not change what a handle or the picture is hit by.
- REQ-0009-03 The workspace, that is the canvas area outside the picture including its margin, shall be drawn in the workspace color: #E7E7E7 in the light theme and #2C2C2C in the dark theme, unless the settings file says otherwise (REQ-0009-04, REQ-0009-06). The toolbox, the color box and the status bar keep the theme's colors.
- REQ-0009-04 (Pointer note, Preferences, 2026-11-03: the app writes these keys only for a change in Preferences, REQ-0020-03.) The workspace color shall be a setting with two keys in section `[view]`: `workspace-color` and `workspace-color-dark`, each a value of exactly the form `#rrggbb` (a `#` and six hexadecimal digits, upper or lower case; syntax of the file as REQ-0001-40). A missing key and any other value (`#fff`, `E7E7E7`, `#E7E7E7FF`, `red`, an empty value) count as not set, silently. The app never rewrites the file for them (as REQ-0004-15).
- REQ-0009-05 Every rule that relates a point of the canvas area to a picture pixel shall count the margin, so that a pixel is where it is drawn: the pixel under the pointer for every tool, the zoom anchors of the wheel, the pinch and the zoom commands (REQ-0004-06), Fit to Window (REQ-0004-07), the Magnifier's focus and its preview box (REQ-0007-03, REQ-0007-04), the pixel grid, and the resize handles and their drag.
- REQ-0009-06 (Pointer note, Preferences, 2026-11-03: `[view] style` decides the theme in use, REQ-0020-03.) While the light theme is in use, the workspace shall be `workspace-color` if it is set, else #E7E7E7; while the dark theme is in use, `workspace-color-dark` if it is set, else `workspace-color` if it is set, else #2C2C2C. When the system theme changes while the app runs, the workspace follows it at once. (Amended twice on 2026-10-18 by JOD: first one color for both themes, at M1; then one color per theme, with the dark key optional, before M2.)

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

- AS-9 (REQ-0009-06, REQ-0009-04) Given no workspace keys, When the app starts in the light theme, Then the workspace is #E7E7E7, and in the dark theme #2C2C2C; Given `workspace-color=#3465A4` only, Then it is #3465A4 in both themes; Given also `workspace-color-dark=#102030`, Then it is #3465A4 in the light theme and #102030 in the dark one; Given `workspace-color-dark=#123` (invalid) and `workspace-color=#3465A4`, Then the dark theme shows #3465A4; Given no keys, When the system theme changes from light to dark and back while the app runs, Then the workspace changes to #2C2C2C and back to #E7E7E7 at once | Verified by: automated (ui, both themes, a theme change while running) + manual M2

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
- Rule REQ-0001-02 (an `auto` picture is the largest standard size smaller than the visible canvas area, so no scrollbars): smaller than the area less its two margins (approved by JOD on 2026-10-18 during the build; pointer note added to 0001).
- Rule REQ-0004-06 (zoom commands keep the center still; a smaller picture stays at its top-left): the same, counting the margin.
- Rule REQ-0004-07 (Fit to Window): the picture plus its margins must fit the area, so the whole percent can be one lower (AS-5).
- Rule REQ-0004-16 (resize handles touch the picture, scroll with it, the scrollable area includes them): they lie in the margin; the scrollable area is the picture, the handles and the margin.
- Rule REQ-0007-03 and REQ-0007-04 (the Magnifier's focus, scroll limits and preview box) and REQ-0004-12 (the grid's lines): counted from the picture's own origin, which is 12 pixels in.
- Rule REQ-0001-14, REQ-0001-19 (a pan drag outside the picture, the pixel under the pointer): a point in the margin or the shadow is outside the picture.

## Open questions
None. Settled by JOD on 2026-10-18: the workspace color depends on the theme, with an optional dark key (REQ-0009-04, REQ-0009-06; first the theme's own dark background, then at M1 one color for both themes, then before M2 this rule); the margin is 12 pixels on all four sides (REQ-0009-01); a Pan drag that starts in the margin pans (REQ-0009-07).

## Plan
Decided before the Plan, by JOD on 2026-10-18, to be built on and not reopened: approach B (a widget margin around the
canvas, one constant), pan from the margin checked by a test first, and the shadow and the workspace color drawn by the
style system.

### Approach
The canvas sits in a viewport inside the scrolled area. It gets a 12 logical pixel margin on all four sides
(`EASEL_VIEW_MARGIN`, `src/core/view.h`), so the viewport places it 12 pixels in and the scrollable size is the picture
plus 24, with no change to how the canvas maps a point to a pixel, draws, or finds a handle: those stay in picture-origin
coordinates. The handles, 7 pixels past the picture today (`HANDLE_REACH`, counted in the canvas's own size), now lie in
the margin, so the canvas measures the picture only and draws the handles past its own edge (widgets are not clipped to
their bounds by default; risk below). Every rule that turns a scroll offset into a picture position counts the margin, and
there are four such places: the zoom anchor (`easel_view_anchor`), the Magnifier's focus scroll (`easel_view_focus_scroll`),
Fit to Window (`easel_view_fit_zoom`, the area less two margins in device pixels), all three in the core and tested there,
and the grid's visible part, which instead of reading the scroll offsets is computed from the viewport's bounds in the
canvas's own coordinates, so it needs no constant. The shadow is a CSS box shadow on the canvas (`0 0 6px`, a dark grey
fading out), drawn by GTK around the canvas's box, which is now exactly the picture. The workspace color is the background
of the viewport: `style.css` gives #E7E7E7, and #2C2C2C under `@media (prefers-color-scheme: dark)` (the probe of T001 (d)
showed this follows a theme change at once); for the valid keys of the settings file, the code chooses the color for the
current theme (in the dark theme the dark key, else the light one) and loads it into a style provider of higher priority,
and loads it again on each change of theme (`notify::dark`), since a media query in a provider loaded from a string is not
re-evaluated (corrected at the check). The setting is parsed in the core with the other `[view]`
keys.

### Files
- `src/core/view.{c,h}` (the constant; the three functions count it), `src/core/settings.{c,h}` (`workspace_color`,
  `has_workspace_color`), `tests/core/test-view.c`, `tests/core/test-settings.c`.
- `src/ui/canvas.c` (margins; measure without `HANDLE_REACH`; the grid's visible part from the viewport; the `workspace`
  style class on the scrolled area), `src/ui/window.c` (the provider for a set `workspace-color`), `src/ui/style.css`,
  `easel.gresource.xml`; `tests/ui/test-window.c` (a 0009 block, and the helpers that turn
  scroll offsets into picture positions), `tests/mutants.txt`.

### Key interfaces
- `#define EASEL_VIEW_MARGIN 12` (logical pixels, each side).
- `easel_view_anchor` and `easel_view_focus_scroll` keep their signatures; `easel_view_fit_zoom` takes a `scale` as well,
  since the margins are logical pixels and the area device pixels (corrected in T003). Their scroll offsets and areas are
  those of the viewport, margin included, as GTK reports them.
- `EaselSettings.has_workspace_color`, `EaselSettings.workspace_color` (0xRRGGBB).

### How each rule is kept
- **REQ-0009-01, -05:** the margin is on the widget; the core functions add it where a scroll offset meets a picture
  position; the pointer mapping is unchanged, because pointer coordinates already arrive in the canvas's own coordinates.
- **REQ-0009-02:** a CSS box shadow outside the canvas's box, which equals the picture; the handles are drawn by the canvas
  after it, so over it; hit tests do not look at the shadow.
- **REQ-0009-03, -04, -06:** two rules give the defaults per theme; valid settings add higher-priority rules per theme.
- **REQ-0009-07:** a press in the margin reaches the canvas with negative coordinates (as on the grey area today); the test
  comes first, and only if it fails is the pan attached to the scrolled area.

### What grows
Nothing per use: one constant, one optional style provider, two settings fields.

### Measurements
`docs/engineering.md` budgets a pointer event to pixels within a frame and an 8000 × 8000 picture drawn without freezing.
A box shadow around a canvas that can be 64000 logical pixels wide is new drawing: `perf-0004` (frames at 8000 × 8000) and
`perf-0006` run before the change (T002) and after it (T012), and both sets of numbers go here.

Before 0009 (T002, 2026-10-18, commit `da3a72c`, GskVulkanRenderer, this machine), median / worst on screen:

| Measure | 1920 × 1080 | 4000 × 4000 | 8000 × 8000 |
|---|---|---|---|
| open to first frame (`perf-0004`) | 73.1 ms | 39.8 ms | 68.1 ms |
| pencil motion | 16.6 / 17.7 ms | 16.6 / 25.9 ms | 16.7 / 41.3 ms |
| undo of a 50,000-pixel stroke | 16.4 / 18.8 ms | 16.3 / 17.1 ms | 16.4 / 17.2 ms |
| frame with the grid, cell 1 at 800 % | 16.9 / 20.2 ms | 16.5 / 19.1 ms | 16.7 / 23.6 ms |
| resize, on screen | 16.7 / 49.5 ms | 24.2 / 25.9 ms | 61.4 / 84.2 ms |
| Brush circle 7 motion to frame (`perf-0006`) | 16.7 / 47.0 ms | – | 16.6 / 18.9 ms |
| spray (24) held 1 s: sprays, frames | 202, 64 | – | 200, 60 |

After 0009 (T012, same machine and renderer), median / worst on screen, with the before value in brackets:

| Measure | 1920 × 1080 | 4000 × 4000 | 8000 × 8000 |
|---|---|---|---|
| open to first frame | 105.9 ms (73.1) | 38.6 ms (39.8) | 61.0 ms (68.1) |
| pencil motion | 16.6 / 20.4 ms (16.6 / 17.7) | 16.6 / 26.3 ms (16.6 / 25.9) | 16.7 / 36.0 ms (16.7 / 41.3) |
| undo of a 50,000-pixel stroke | 17.0 / 47.3 ms (16.4 / 18.8) | 16.6 / 17.5 ms (16.3 / 17.1) | 16.2 / 18.8 ms (16.4 / 17.2) |
| frame with the grid, cell 1 at 800 % | 16.7 / 18.4 ms (16.9 / 20.2) | 16.8 / 19.3 ms (16.5 / 19.1) | 16.5 / 42.7 ms (16.7 / 23.6) |
| resize, on screen | 16.7 / 19.2 ms (16.7 / 49.5) | 23.4 / 26.0 ms (24.2 / 25.9) | 65.0 / 87.7 ms (61.4 / 84.2) |
| Brush circle 7 motion to frame | 16.7 / 25.7 ms (16.7 / 47.0) | – | 16.7 / 22.3 ms (16.6 / 18.9) |
| spray (24) held 1 s: sprays, frames | 202, 61 (202, 64) | – | 201, 61 (200, 60) |

Every median is the same frame as before (16.2 to 17.0 ms where a frame was the budget), so the shadow and the margin cost no
frame; the worst cases move both ways between runs (single outliers: 47.3 ms for one undo, 42.7 ms for one grid frame, and
before 49.5 ms for one resize and 47.0 ms for one brush motion), and the first open of 1920 × 1080 took 105.9 ms against
73.1 ms. No budget is broken on the medians, so the fallback (the shadow drawn in the canvas's snapshot) is not needed.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_margin_and_workspace` (rendered: the first picture pixel 12 from the area's left and top, #E7E7E7 around, the far right and bottom at the end of the scroll) + manual M1 |
| 2 | ui `as2_shadow_on_four_sides` (a rendered row and column across each edge: darker next to the picture, fading to #E7E7E7, picture pixels unchanged; the saved file is the picture alone) + manual M1 |
| 3 | core `as3_workspace_color_setting` (the valid and invalid values of the scenario), ui `as3_workspace_color_from_settings` |
| 4 | ui `as4_pointer_maps_with_margin` |
| 5 | core `as5_fit_zoom_with_margin` (41 %), ui `as5_fit_to_window_with_margin` |
| 6 | core `as6_anchor_with_margin`, ui `as6_zoom_anchors_with_margin` (zoom keys, wheel, Magnifier; 100 %, 400 % scrolled, 41 %) |
| 7 | ui `as7_handles_in_margin` |
| 8 | ui `as8_grid_with_margin` (as `as10_grid_at_magnifier_levels`, positions from the viewport and the margin) |
| 9 | ui `as9_workspace_dark_theme` (the defaults per theme, one key for both, both keys, an invalid dark key, a theme change while running) |
| 10 | ui `as10_pan_from_the_margin` |

### Sites in `tests/ui/test-window.c` that read scroll offsets (T005, for JOD's approval; nothing edited yet)
- **Group 1, a view point taken as a canvas point** (the scroll offset plus a point of the view, used as canvas
  coordinates): `view_center` (5860-5861), `as5_zoom_keeps_center` (1787-1794), `as13_zoom_keeps_point_under_pointer` (188),
  `as17_pencil_after_magnifier` (6377-6387). Fix: one helper, `view_to_canvas (adjustment, view point)` = offset + point −
  `EASEL_VIEW_MARGIN`, used by all four (`view_center` through it).
- **Group 2, the end of the scroll**: `as4_magnifier_edges` (6008-6009: offset + page = 6000 / 4000) and
  `as26_scroll_clamp_after_resize` (2853-2854: at most 1700 − page). Fix: the scrollable size is the content plus
  2 × `EASEL_VIEW_MARGIN` (6024 / 4024, 1724 − page), written with the constant.
- **Group 3, where the canvas is drawn in the scrolled area**: `as13_offset_is_displayed` (2209-2220) and
  `as3_magnifier_click` (5907-5908) assert that the canvas's origin is at minus the scroll offset. Fix: at
  `EASEL_VIEW_MARGIN` minus the offset.
- **Unaffected, listed for completeness**: the "offset 0 at the start" asserts (1805-1806, 1850-1851, 2860-2861, 2868,
  4057-4058, 5995-6002, 6020-6021), since 0 is still the start of the scroll; the page-size reads used as the canvas area
  (130-131, 1818-1819, 5897, 5993, 6056); the canvas-coordinate helpers `canvas_point`, `click_pixel`, `point_at_pixel`;
  `as10_grid_pixels` (2331-2334: its rows and columns R and C only need to be visible and on the 4-pixel grid, and the
  margin, 12, keeps both); `as10_grid_at_magnifier_levels` (positions from the rendered canvas origin); `as20_attributes_dialog`
  (2675: a spin row's adjustment, not a scroll).
- **Not a test fix but a spec gap**: `as2_auto_fits_view` (130-133) checks REQ-0001-02, an `auto` picture smaller than the
  visible area "so no scrollbars"; with the margins, a 1024 × 768 picture in an area under 1048 × 792 would scroll.

### Constitution check
Layering holds (the constant, the three scroll rules and the setting are libc-only core); no threads or caches (P5); the
workspace color is a setting with a silent default (P6); every REQ has a scenario and every scenario a test (S3).

### Risks
- **Existing tests that read scroll offsets:** about 40 places in `test-window.c` turn the adjustments into picture
  positions or assert a scroll limit (offset 0 at the top-left, offset + page = content at the bottom-right). With the
  margin those numbers move by 12 (or 24) logical pixels, as REQ-0009-01 says they should. They are listed before any is
  changed, and the shared helpers (`view_center` and the ones like it) count the margin once; a test that mixes canvas
  coordinates and scroll offsets is fixed in its own task with the rule it checks named.
- **GTK behavior, each checked by a probe before the code relies on it (in a real frame):** (a) a margin on the viewport's
  child moves the child and adds to the scrollable size; (b) a widget draws outside its own bounds (the handles) and a press
  there still reaches the canvas through the scrolled area; (c) a CSS box shadow on the canvas is drawn outside its box and
  not over the picture; (d) `style-dark.css` from the resource path is loaded and swapped with the theme.
- **Probe T001 (2026-10-18, in a real frame, temporary code):** (a) holds: the canvas sits at (12, 12) in its
  `GtkViewport` and the scrollable size of a 3000 × 2000 picture is 3024 × 2024. (b) holds: with the canvas measured at the
  picture's size, the right-middle handle is drawn past the canvas's edge (navy at x + 302 on a 300-pixel picture). (c) holds:
  a CSS box shadow `0 0 6px rgba(0, 0, 0, 0.35)` is drawn outside the picture only, fading from #C4C4C4 next to it to
  #E6E6E6 6 pixels out on #E7E7E7, and the picture's first pixels keep their color. (d) fails as planned and holds another
  way: libadwaita refuses `style-dark.css` ("deprecated … use style.css with media queries", a fatal warning in the tests),
  so the dark default goes in `style.css` under `@media (prefers-color-scheme: dark)`; it gives #2C2C2C in the dark theme and
  follows a theme change while the window is open (dark #2C2C2C, back to light #E7E7E7). (e) holds: a point on a handle in the
  margin picks the viewport, inside the scrolled area whose controller feeds the canvas; the handle is hit and a press starts
  a resize. (f) holds: a Pan drag from (-6, -6), on the shadow, moved the view by 50 pixels. Noted for M1: in the dark theme
  the same shadow was barely visible (#252525 next to the picture on #2C2C2C); with REQ-0009-06 as amended the workspace is
  #E7E7E7 in both themes, so the dark-theme rule was not built; amended again before M2 (one color per theme), it is built
  with the media query of (d).
- **The canvas's size is the picture's size rounded up** (`easel_view_content_size`): at a fractional zoom the box, and so
  the shadow, can be one device pixel bigger than the picture; the shadow test allows one pixel.
- **A box shadow on a very large widget** may cost a frame; measured in its task, with the fallback of drawing the shadow in
  the canvas's own snapshot, clipped to the visible part.

## Tasks
Groups end with a full `meson test` and `tools/mutate.sh --lint`; each ticked task gets a `Result:` line under it. The 0004
test of the grid at a scrolled view, `as10_grid_at_magnifier_levels`, stays green after every task of groups D and E.

A. Probe, look and the numbers before
- [x] T001 Probe in a real frame, then a first look (Plan risks a to d; REQ-0009-01, -02, -03, -07) - done when a temporary probe, with a 12 px margin, a box shadow and a #E7E7E7 background put on the running window, has shown and written into the Plan's risks: (a) the margin moves the canvas and adds 24 px to the scrollable size; (b) the canvas draws past its own edge and a press there reaches it; (c) the shadow is drawn outside the canvas's box and not over the picture; (d) `style-dark.css` is loaded and swapped with the theme; (e) a press on a resize handle lying in the margin reaches the canvas and starts a resize; (f) a Pan drag that starts in the margin pans; and manual M1 (JOD looks at the margin, the shadow and the color on a small picture, light theme) is done; no probe code is kept
  Result: manual M1 step 1 (JOD, 2026-10-18): "all is good": the 12 px margin from the toolbox and the header bar, the shadow on four sides, the light grey around the picture and the handles over the shadow, light theme
  Result: manual M1 step 2 (JOD, 2026-10-18): the dark theme works as specified (the workspace follows the theme at once), but JOD wants the workspace the same grey in the dark theme as in the light one (spec change proposed, REQ-0009-06)
  Result: probe, no code kept (the temporary margin, class and style rules reverted): (a), (b), (c), (e) and (f) hold; (d) holds with a media query, not `style-dark.css`, and is not built since REQ-0009-06 was amended to one color in both themes; details in the Plan's risks
- [x] T002 The numbers before (engineering.md: a pointer event within a frame, 8000 × 8000 without freezing) - done when `perf-0004` and `perf-0006` have run on the code before 0009 and their numbers are in the Plan's Measurements
  Result: measurement, no test: `perf-0004` and `perf-0006` on the code before 0009 (`da3a72c`); numbers in the Plan's Measurements

B. The core
- [x] T003 The margin constant and the three rules that count it (REQ-0009-05; AS-5, AS-6) - done when core `as5_fit_zoom_with_margin` (41 % for 3000 × 2000 in 1390 × 853), `as6_anchor_with_margin` and a focus-scroll case (the far edge is the picture plus the margin) pass, each fails first, the core tests of 0004 and 0007 pass (amended only where they assert a scroll limit, named in the Result), and three mutants (the anchor, the focus scroll and Fit to Window each forget the margin) are killed by assertions
  Result: `as5_fit_zoom_with_margin` failed first by not compiling (`EASEL_VIEW_MARGIN` undeclared, `easel_view_fit_zoom` had no scale), then at run time at `test-view.c:172: easel_view_fit_zoom (1390, 853, 3000, 2000, 1.0) == 41` and `focus_scroll_with_margin` at `test-magnifier.c:100`; with the margin counted all pass. `easel_view_fit_zoom` gained a `scale` parameter (the margins are logical pixels, the area device pixels; the Plan's "signatures unchanged" is corrected by this line). Amended, as REQ-0009-01 moves their numbers: core `as13_zoom_steps_and_anchor` (pointer points and edge values), `as5_zoom_anchor_center` (center 988 / 788, 1488 / 1188, 6, 1024, 2988), `as6_fit_zoom` (138, 41, 15, 785, the exact-fit cases and the loop with 1366 × 829) and `as4_focus_scroll` (12 + … and 6024 / 4024). 3 new mutants (each rule forgets the margin) killed by assertions; the REQ-0007-03 "edge, not center" mutant rewritten for the new line, killed. The ui suites 0001, 0004, 0007 and 0008 fail until the canvas has its margin (T006): reported to JOD
- [x] T004 The `workspace-color` setting (REQ-0009-04; AS-3) - done when core `as3_workspace_color_setting` (`#3465A4`, `#3465a4`; `3465A4`, `#346`, `#3465A4FF`, `blue`, empty and missing give no color) passes and fails first, the file is not rewritten, and a mutant that accepts a bad value (`#346`) is killed by an assertion
  Result: `as3_workspace_color_setting` failed first by not compiling (no `has_workspace_color`), then at `test-settings.c:175` with the fields and no parsing; passes with `#rrggbb` parsing (`[view]` only, spaces trimmed as every value, an invalid repeat resets). The mutant that accepts `#346` is killed by an assertion. The file is never rewritten: nothing writes it for this key, and the default text a new file gets is unchanged (`as14_view_settings`)

C. The tests that read scroll offsets
- [x] T005 List every site in `tests/ui/test-window.c` that turns a scroll offset into a picture position or asserts a scroll limit, grouped by the helper that would fix each group (for example `view_center`, `canvas_point`, the R / C arithmetic of the grid tests, the "offset 0" and "offset + page = content" asserts) - done when the list is in the Plan and JOD has approved it; no test is edited in this task
  Result: JOD approved the list (in the Plan) and option (b): the sites were fixed through one helper, `view_to_canvas` (`view_center` uses it), the end-of-scroll asserts written with the constant, and the canvas origin as the margin less the offset; run before the canvas had its margin, `as26_scroll_clamp_after_resize`, `as13_offset_is_displayed`, `as3_magnifier_click` and `as4_magnifier_edges` failed (red), while `as13_zoom_keeps_point_under_pointer`, `as5_zoom_keeps_center`, `as17_pencil_after_magnifier` and `as2_zoom_to_preset` already passed (the core counted the margin and the tests now did too). The list missed two sites that read sizes, not offsets: `as20_attributes_dialog` (the auto size of the Default button, fixed with the REQ-0001-02 amendment) and `as15_handles` (the canvas grew by 7 for the handles; now they lie in the margin, REQ-0004-16 as amended)

D. The canvas
- [x] T006 The margin on the canvas, the handles in it, the canvas the size of the picture (REQ-0009-01, -05; AS-1 margin part, AS-4, AS-7) - done when ui `as1_margin_and_workspace` (the margin part), `as4_pointer_maps_with_margin` and `as7_handles_in_margin` pass and fail first, the sites of T005 are changed as approved, and ui-0001 to ui-0008 pass, `as10_grid_at_magnifier_levels` and `as10_grid_pixels` included
  Result: done together with T005 under option (b): the canvas has `EASEL_VIEW_MARGIN` on four sides and measures the picture only (`HANDLE_REACH` removed); the auto size counts the margins (REQ-0001-02 amended by a pointer note, `as2_auto_fits_view` failed first at `768 < 751`). The new `as1_margin_and_workspace` (margin part), `as4_pointer_maps_with_margin` and `as7_handles_in_margin` were written after the code and passed at once (characterization); with the canvas margin set to 0 for one run, `as1` failed, `as4` and `as7` still passed, so both were strengthened to check the window's own layout (the rendered pixel 12 in, the canvas 12 in its viewport) and then failed without the margin. Pointer notes added on REQ-0001-11, REQ-0004-06, -07, -12, -16 and REQ-0007-03. Full `meson test` 28 of 28 (ui-0009 added), `as10_grid_at_magnifier_levels` and `as10_grid_pixels` green; `tools/mutate.sh --lint` 154 mutants, 0 stale; 0 warnings
- [x] T007 The zoom anchors, Fit to Window and the Magnifier in the window (REQ-0009-05; AS-5, AS-6) - done when ui `as5_fit_to_window_with_margin` and `as6_zoom_anchors_with_margin` (zoom keys, wheel, Magnifier; at 100 %, at 400 % scrolled and at 41 %) pass and fail first
  Result: characterization: `as5_fit_to_window_with_margin` and `as6_zoom_anchors_with_margin` passed at once, because T003 (the core rules) and T006 (the canvas margin) had already built what they check; both read the picture point from the window's own layout (the canvas's bounds in the scrolled area), not from the constant. Checked by breaking the code once: the three core mutants that forget the margin, run against ui-0009, were each killed by these tests (`x1 == x0: (670.25 == 664.5)`, the Magnifier's center `993.75` against `1000.5`, the fit not leaving room for the margins). The first version of `as6` failed its own precondition (scroll 1107.5 at 400 %, not more than a page): two Zoom Ins with no frame between them lost the center; with a layout between them the center holds. Not checked whether that was so before 0009; reported to JOD. ui-0009, core and io green; mutants of REQ-0009-04 and -05 killed
- [x] T008 The grid's visible part from the viewport (REQ-0009-05, REQ-0004-12; AS-8) - done when ui `as8_grid_with_margin` passes and fails first, `as10_grid_at_magnifier_levels` stays green, and a mutant (the grid reads the scroll offsets without the margin) is killed by an assertion
  Result: `as8_grid_with_margin` failed first at `assert_grid_lines_near: 'pixel_is (px, w, x, y + step / 2, line)' should be TRUE` (no line near the view's top-left, where a grid read from the scroll offsets leaves 12 pixels bare); the grid now takes its visible part from the viewport's bounds in the canvas's own coordinates and the test passes. The first version sampled 5 pixels in, inside the 2 pixels the scrolled area shades along its edge (a probe of the rendered rows showed #E6E6E6 and #F6F6F6 there), so it now samples 8 pixels in; with only the old grid code put back it fails again, as above. `as10_grid_at_magnifier_levels` and `as10_grid_pixels` stay green. A mutant (the grid reads the scroll offsets without the margin) is killed by that assertion; the mutants of REQ-0004-12, REQ-0009-04 and -05 all killed
- [x] T009 Pan from the margin (REQ-0009-07; AS-10) - done when ui `as10_pan_from_the_margin` (from the shadow, from 11 px off the area's edge; nothing with Pencil) passes, with a red run or a characterization line if the probe of T001 (f) already showed it working
  Result: characterization: `as10_pan_from_the_margin` passed at once, as T001 (f) predicted: a press in the margin reaches the canvas with negative coordinates and Pan already pans from any point of the canvas area. The press points come from the window's layout (the canvas's bounds in the scrolled area). Checked by breaking the code once (a copy of `canvas.c` set aside, Pan made to ignore presses outside the picture, the copy restored): the test failed at `gtk_adjustment_get_value (view.h) == before + 50: (0 == 50)`. That break is now the mutant of REQ-0009-07, killed by the assertion. ui-0009, core and io green

E. The look
- [x] T010 The shadow (REQ-0009-02; AS-2) - done when ui `as2_shadow_on_four_sides` (a rendered row and column across each edge, darker next to the picture and fading to the workspace color, the picture's pixels unchanged, the saved file the picture alone; one device pixel of tolerance for the rounded canvas size) passes and fails first
  Result: `as2_shadow_on_four_sides` failed first at `brightness (near) < brightness (far) - 20: (750 < 730)` (no shadow: #FAFAFA next to the picture as 9 pixels out); with the `easel-picture` style class on the canvas and `box-shadow: 0 0 6px rgba(0, 0, 0, 0.35)` in `style.css` it passes: #DFDFDF next to each of the four sides, fading to the workspace, the picture's edge pixels #FFFF00, the saved PNG 300 × 200 with its first pixel the picture's color. 2 mutants (no class; a shadow too faint to see) killed by that assertion. ui-0009, core and io green
- [x] T011 The workspace color in both themes and from the setting (REQ-0009-03, -04, -06; AS-1 color part, AS-3, AS-9) - done when ui `as1_margin_and_workspace` (the color part), `as3_workspace_color_from_settings` and `as9_workspace_dark_theme` (the same color in both themes, a set color, unchanged by a theme change while running) pass and fail first, and manual M2 (the final look in the light and dark themes, with and without a set color) is done
  Result: each failed first: `as1_margin_and_workspace` (color part) at `0xfffafafa == 0xffe7e7e7` (the theme's color), `as3_workspace_color_from_settings` at `0xfffafafa == 0xff3465a4`, `as9_workspace_dark_theme` at `0xff2c2c2c == 0xffe7e7e7` (the dark theme's own color); with the `workspace` style class on the scrolled area, one rule in `style.css` (#E7E7E7, both themes) and an app-wide style provider of higher priority loaded with a valid `workspace-color` (or emptied) by each new window, all pass: #E7E7E7 by default in both themes and across a theme change, #3465A4 when set in either case, every invalid value the default, the settings file unchanged. 3 mutants (no class; a set color ignored; the workspace following the dark theme) killed by assertions, with the REQ-0009-04 parsing one. ui-0009, core and io green; full `meson test` 28 of 28; `tools/mutate.sh --lint` 161 mutants, 0 stale. Manual M2: see below

  Result: manual M2 (JOD, 2026-10-18): "all is good": the margin, the shadow and #E7E7E7 in the light theme; the workspace following a switch to dark (#2C2C2C) and back at once, the shadow in the dark theme accepted; the set colors per theme with the dark key falling back to `workspace-color`; five fast Zoom Ins keeping the center (the unverified double Zoom In of T007 not seen in the real app); a Pan drag from the margin panning and Pencil painting nothing there
- [x] T013 The workspace color per theme (REQ-0009-04, -06 as amended before M2; AS-9) - done when core `as3_workspace_color_setting` (the dark key: valid, invalid, missing) and ui `as9_workspace_dark_theme` (the defaults per theme, one key for both, both keys, an invalid dark key, a theme change while running) pass and fail first, and mutants (the dark default not used; the dark key ignored; no fall back from the dark key to the light one) are killed by assertions; manual M2 then covers T011 and T013 together
  Result: REQ-0009-03, -04, -06 and AS-9 amended first (per-theme rule, optional dark key). Each failed first: core `as3_workspace_color_setting` by not compiling and then at `test-settings.c:194` (the dark key not read), ui `as9_workspace_dark_theme` at `0xffe7e7e7 == 0xff2c2c2c` (no dark default). With the dark default under a media query in `style.css` and the dark key parsed, the case with both keys still failed (`0xff3465a4 == 0xff102030`): a media query in a provider loaded from a string is not re-evaluated when the theme changes, so the app's provider now holds the color for the current theme and is reloaded on the style manager's `notify::dark`; then all pass. `as1_margin_and_workspace`, `as2_shadow_on_four_sides` and `as3_workspace_color_from_settings` now force the light theme, since the system's default had become dark and their colors are the light theme's. Mutants: the stale "set color ignored" one rewritten; the "follows the dark theme" one removed (it is now the rule); five added (no dark default; dark key ignored; no fall back to `workspace-color`; no reload on a theme change; dark key not read), all killed by assertions. ui-0009, core, io and the full `meson test` (28 of 28) green; `tools/mutate.sh --lint` 165 mutants, 0 stale

F. The numbers after
- [x] T012 The numbers after (engineering.md budgets as T002) - done when `perf-0004` and `perf-0006` have run on the finished code, the numbers are in the Plan next to those of T002, and any frame over budget has been reported to JOD with the Plan's fallback (the shadow drawn in the canvas's snapshot, clipped to the visible part) before it is built
  Result: measurement, no test: `perf-0004` and `perf-0006` on the finished code; every median the same frame as before, the worst cases single outliers in both directions, the first open of 1920 × 1080 105.9 ms against 73.1 ms; numbers next to T002's in the Plan; no frame over budget, the fallback not built

## Check
- Date and commit: 2026-10-18, `da3a72c` plus the uncommitted work of 0009
- Tests: `meson test` 28 of 28 (the cases of this feature: 10 ui `/0009/...`; core `as5_fit_zoom_with_margin`, `as6_anchor_with_margin`, `focus_scroll_with_margin`, `as3_workspace_color_setting`; and the 0001, 0004, 0007 and 0008 tests amended for the margin)
- Mutants: 164 killed, 1 killed by crash (test gap), 0 survived, as `tools/mutate.sh` reports (921 s); the crash is REQ-0005-12, not 0009's (finding 1)
- Warnings: 0. Lint, `lint --names 0009` and status clean (13 of 13 tasks)
- Budget, open to first frame at 1920 × 1080 (`perf-0004`, five runs now): 114.7, 71.1, 71.7, 76.9, 74.7 ms, median 74.7 ms, spread 71.1 to 114.7 ms; T002 measured 73.1 ms before 0009 and T012 105.9 ms after. `perf-0004` names no budget for this line; the nearest is "start to usable window < 0.5 s" (`docs/engineering.md`), which it is far under. The median is the same as before: T012's 105.9 ms was one slow run (noise), as is the 114.7 ms here
- Results of the tasks: T001 probe, (a) to (f) hold, (d) by a media query, M1 good (dark default replaced at M1, then restored per theme before M2); T002 and T012 measured; T003 red then green, four older core tests amended by name; T004 red then green; T005 the list approved, two sites missed and fixed in T006; T006 the margin built with T005 (option b), its three new tests characterization; T007 characterization; T008 red then green; T009 characterization; T010 red then green; T011 red then green, M2 all good; T013 red then green
- Scenarios and their tests (C = written after the code, a characterization; the mutants that killed it are listed):
  AS-1 ui `as1_margin_and_workspace` (margin part C in T006, broken by a run with the canvas margin 0; color part red in T011; killed by "the workspace keeps the theme's color"); AS-2 ui `as2_shadow_on_four_sides` (red; killed by "no shadow" and "too faint"); AS-3 core `as3_workspace_color_setting` and ui `as3_workspace_color_from_settings` (red; killed by "accepts #346" and "a valid color ignored"); AS-4 ui `as4_pointer_maps_with_margin` (C in T006, strengthened to read the window's layout, then broken by margin 0; no mutant in the list); AS-5 core `as5_fit_zoom_with_margin` (red) and ui `as5_fit_to_window_with_margin` (C in T007; killed by "Fit to Window forgets the two margins" when run against ui-0009); AS-6 core `as6_anchor_with_margin` and `focus_scroll_with_margin` (red) and ui `as6_zoom_anchors_with_margin` (C in T007; killed by "the anchor forgets the margin" and "the focus scroll forgets the margin" against ui-0009); AS-7 ui `as7_handles_in_margin` (C in T006, strengthened, broken by margin 0; no mutant in the list); AS-8 ui `as8_grid_with_margin` (red; killed by "the grid reads the scroll offsets"); AS-9 core `as3_workspace_color_setting` and ui `as9_workspace_dark_theme` (red; killed by the four REQ-0009-06 mutants and "dark key not read"); AS-10 ui `as10_pan_from_the_margin` (C in T009; killed by "a Pan drag from the margin does not pan")
- Learning log: the six lines asked for are in place (the core rule and the ui change in one task; every reader of a changed layout; a ui test that fires several actions needs a layout between them; never git stash for a red check; and, added at this check, a ui test that sets its own theme, and a media query not re-evaluated on a theme change)
- Findings:
  1. Medium, outside 0009: the mutant REQ-0005-12 "a failed Save As already made the new file the picture's file" was killed by a crash in the whole run (`the case after "ok 7 /0005/busy_cursor_before_decoding" died ... SIGABRT`, no assertion) and by an assertion when run alone (`as6_open_replaces_picture: count_alert_dialogs ... (1 == 0)`). The mutant's own text frees `self->path` and then duplicates `path`, which can be the same pointer, so the crash is likely the mutant's use after free, not a gap in the tests. Fixed: the mutant copies `path` first, frees `self->path`, and saves from the copy; run alone it is killed by an assertion (`as11_save_as: gtk_window_get_title ... == "b.jpg – Easel"`). (A first rewrite did not build: a blank line inside a mutant's text ends the block, and the copy's name shadowed a local.)
  2. Low: the canvas margin itself (the four `gtk_widget_set_margin_*` lines, REQ-0009-01) has no mutant in the list; AS-1, AS-4 and AS-7 were shown to catch it only by a manual run with the margin at 0. Fixed: the mutant "the canvas has no margin" is in the list, tested by ui-0009, and killed by an assertion (`as1_margin_and_workspace: r.origin.x == EASEL_VIEW_MARGIN: (0 == 12)`).
  3. Low: the three core mutants of REQ-0009-05 name only core tests, so the list does not record that ui-0009 (AS-5, AS-6) also kills them. Fixed: their `test:` lines name `ui-0009` after the core test.
  4. Low: Plan drift: the approach still says the dark key goes "under the same media query" in the app's provider; the code chooses the color for the current theme and reloads on `notify::dark` (T013). Fixed: the Plan says the code chooses the color for the current theme and loads it again on each change of theme (`notify::dark`).
  5. Low: `load_workspace_color (void)` is connected to `notify::dark`, whose handlers get three arguments; calling a no-argument function through `G_CALLBACK` works on the usual ABIs but is not strictly portable C. Fixed: `on_dark_changed (AdwStyleManager *, GParamSpec *, gpointer)` is connected and calls it; behavior unchanged, `as9_workspace_dark_theme` (the theme switch) green; the mutant that quoted the old connection follows it.
  6. Low: `parse_rgb` in `src/core/settings.c` has a line of 125 characters. Fixed: the line is wrapped, one condition per line.
  7. Low, recorded: T006, T007 and T009 are characterization tasks (their tests came after the code); each was checked by breaking the code once, as listed above.
- After the fixes (2026-10-18, no Easel window open): `meson test` 28 of 28; the whole mutation list 166 killed, 0 killed by crash (test gap), 0 survived, 0 errors, as `tools/mutate.sh` reports (927 s); `tools/mutate.sh --lint` 166 mutants, 0 stale; spec lint and `lint --names 0009` clean; 0 warnings.
- Verdict: 0009 is built as specified and every scenario has a passing test; findings 1 to 6 fixed, 7 recorded.
