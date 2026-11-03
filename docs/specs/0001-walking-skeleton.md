# 0001 Walking skeleton

Status: implemented
Depends on: none

## Why
The thinnest end-to-end path through the app: start on a blank picture, look around it, draw, keep the result, leave
safely. It proves the whole loop (spec to shipped behavior) once, and every later feature grows from it.

## Requirements
Look, layout and input rules come from `docs/design.md`; this spec only adds what 0001 needs.

### Settings file
- REQ-0001-01 The app shall read its settings from `~/.config/easel/settings.ini`, a text file of `key=value` lines under `[section]` headers. 0001 uses section `[new-picture]` with keys `width`, `height` and `canvas-color` (`background` before the rename of REQ-0001-03).
- REQ-0001-02 `width` and `height` shall each be `auto` or a whole number from 1 to 10000. If either is `auto`, both are: the picture takes the largest of 640x480, 800x600 and 1024x768 that is smaller than the visible canvas area in both directions when the window first appears (in device pixels, so at 100 % space remains right and bottom and no scrollbars show); if none fits, 640x480. Later window resizes do not change the picture. (0009: the area less its two margins of 12 logical pixels, REQ-0009-01, so that the picture still shows without scrollbars.)
- REQ-0001-03 `canvas-color` shall be `#RRGGBB`, `#RRGGBBAA` or `transparent` (= `#00000000`): the color the pixels of a new picture are filled with. The older name `background` is still read, with the same values, so that a settings file written before the rename keeps working; when both are given, a valid `canvas-color` wins and `background` counts only when `canvas-color` is missing or invalid. (Renamed by JOD on 2026-10-18, after 0009; the key was `background`.)
- REQ-0001-04 (Pointer note, Preferences, 2026-11-03: a created file also holds the keys of REQ-0020-11.) If the settings file does not exist at start-up, then the app shall create it with `width=auto`, `height=auto`, `canvas-color=#FFFFFF` (`background=#FFFFFF` before the rename of REQ-0001-03). (Amended by REQ-0002-19: a new file also holds `[history]` `memory=512`.) (0004 adds `[view]` to a created file: REQ-0004-15; 0005 adds `[save]`: REQ-0005-14.) (Pointer note, Text, 2026-10-26: A created file also gets the section [text], REQ-0017-24.)
- REQ-0001-05 If a value is missing or invalid, then the app shall use the default from REQ-0001-04 for that value only, without a message to the user; if the file cannot be read or parsed at all, then every value uses its default and the file is left untouched.
- REQ-0001-40 The settings file syntax shall be: blank lines and lines starting with `#` are ignored; spaces around `=` and around the value are trimmed; keys are case-sensitive; `auto`, `transparent` and hex digits are case-insensitive; numbers are digits only (`05` is 5, `+5` and `5.0` are invalid); when a key repeats, the last one wins; unknown sections and keys are ignored. Any other non-empty line makes the file unparseable (REQ-0001-05). (Pointer note, Text, 2026-10-26: The section [text] adds true/false values in any case and a family name matched without regard to case, REQ-0017-24.)
- REQ-0001-41 When the app starts, the window shall open at 75 % of the width and height of the monitor it opens on (if that monitor is not known before the window appears, the first monitor); the system decides where it is placed.
- REQ-0001-42 If `.png` was added to the chosen name (REQ-0001-28) and a file with the resulting name exists, then the app shall ask before replacing it: heading "Replace “<name>”?", body "A file with this name already exists.", buttons Cancel and Replace (destructive); Cancel saves nothing and changes nothing.
- REQ-0001-43 If the user chooses Save in the quit prompt while a save dialog is already open, then the app shall treat it as Cancel: the app stays open, the open save dialog carries on, and a later quit shows the prompt again.

### Start
- REQ-0001-06 When the app starts, the app shall show one window with a new picture of the configured width, height and background, showing its top-left corner first; command-line arguments are ignored (0005 adds them).
- REQ-0001-07 When the app is started while it is already running, the app shall bring the existing window to the front and open no second window.
- REQ-0001-08 When the app starts, the app shall show the picture at 100 % zoom (1 picture pixel = 1 device pixel), with Pan as the active tool, black as the foreground color and the configured picture background as the background color.
- REQ-0001-09 While the window is open, the app shall show a toolbox with two tools, Pan and Pencil; clicking a tool makes it the active tool, shown in the selected state; activating a tool while a stroke is in progress shall be ignored. (0007 adds Magnifier, Pick Color and Fill: REQ-0007-01, and ignores a tool change during a pick: REQ-0007-16.) (0006 adds Brush, Airbrush and Eraser and an options panel below the toolbox: REQ-0006-01, REQ-0006-02.)
- REQ-0001-10 While the pointer is over the canvas area, the app shall show a cursor that identifies the tool: an open hand for Pan (a closed hand while dragging), a pencil for Pencil.
- REQ-0001-11 While the picture is smaller than the view, the app shall place it at the top-left of the canvas area; transparent pixels shall show a checkerboard. (0009: 12 logical pixels in from the left and the top, the margin of REQ-0009-01.)

### Window title
- REQ-0001-12 The window title shall be `<name> – Easel`, where `<name>` is "Untitled" while the picture has never been saved and the saved file's base name with extension (e.g. `a.png`) after a save.
- REQ-0001-13 While the picture has changes that are not saved, the title shall start with "• " (e.g. `• Untitled – Easel`). (Since 0002, "modified" follows the undo history: REQ-0002-12.)

### View: pan, scroll, zoom
- REQ-0001-14 While Pan is active and the user drags with the primary button, the app shall move the view with the pointer (the picture follows the pointer), stopping at the picture's edges, without changing any pixel or the modified state; a drag that starts in the canvas area outside the picture also pans.
- REQ-0001-15 While any tool is active, the mouse wheel shall scroll the view vertically and Shift + wheel horizontally, stopping at the picture's edges; scrollbars shall show the view's position while the picture is larger than the view.
- REQ-0001-16 While Pan is active and the user turns the mouse wheel with Ctrl held, the app shall move one step per notch through the zoom levels 10, 25, 50, 100, 200, 300, 400, 600, 800, 1600 % (wheel up = larger; 1600 % is Easel's own, beyond XP's 800 %, JOD, 2026-10-27), keeping the picture point under the pointer under the pointer as far as the view's edge limits allow; at 10 % and 1600 % further notches in that direction do nothing. Touchpad Ctrl + scroll counts in whole notches. (0004 adds zoom commands for every tool, and the pinch: REQ-0004-04, REQ-0004-08; the wheel rule is unchanged.)
- REQ-0001-17 While any tool other than Pan is active, Ctrl + wheel shall not change the zoom.
- REQ-0001-18 Above 100 %, the app shall show each picture pixel as a solid square of its color; below 100 %, each device pixel shall show the color of exactly one picture pixel, with no blending.

### Pencil
- REQ-0001-19 When the user presses the primary button on the picture with the Pencil, the app shall set the one picture pixel under the pointer to the foreground color, at any zoom.
- REQ-0001-20 While the Pencil is active and the user drags with the primary button held, the app shall draw an 8-connected, 1-pixel-wide line between each pair of consecutive pointer positions, with no antialiasing.
- REQ-0001-21 When the user presses or drags with the secondary button with the Pencil, the app shall draw as in REQ-0001-19/20 but with the background color.
- REQ-0001-22 A stroke may start in the canvas area outside the picture; the app shall draw only the parts of each line segment that fall inside the picture.
- REQ-0001-23 If the user presses the secondary button during a primary stroke, or the primary button during a secondary stroke, then the app shall cancel the stroke and restore the picture to how it was before the stroke began.
- REQ-0001-24 The middle mouse button shall do nothing and shall not end or cancel a stroke.
- REQ-0001-25 If the window loses focus or the button is released outside the window during a stroke, then the app shall end the stroke and keep what was drawn.
- REQ-0001-26 When a stroke changes at least one pixel, the app shall mark the picture as modified; a stroke that changes no pixel shall leave the modified state as it was. (Since 0002, "modified" follows the undo history: REQ-0002-12.)

### Save
- REQ-0001-27 When the user chooses Save (Ctrl+S or the header-bar Save button) on a picture that has never been saved, the app shall open a file dialog in the Pictures folder suggesting `Untitled.png`, and save as PNG.
- REQ-0001-28 If the chosen name has no extension, then the app shall add `.png`; if it has an extension other than `.png` (any case), then the app shall show an error saying only PNG can be saved in this version and save nothing. (0005 saves JPEG, BMP, TIFF and WebP too, by the extension: REQ-0005-10.) A name ending in a dot counts as having no extension (`a.` becomes `a.png`); a leading dot marks a hidden file, not an extension (`.png` becomes `.png.png`); `.PNG` in any case is kept as typed.
- REQ-0001-29 When the user chooses Save on a picture that already has a file, the app shall write that file again without asking, whether or not the picture is modified.
- REQ-0001-30 When a save succeeds, the app shall clear the modified mark; the saved file shall decode to exactly the same size and pixels, including transparency. (For PNG; JPEG and BMP cannot keep transparency: REQ-0005-11.)
- REQ-0001-31 If saving fails, then the app shall show an error dialog that names the file and the system's reason, keep the picture, its title and its modified state unchanged, and leave any existing file at that path with its old content.
- REQ-0001-32 If the user cancels the file dialog, then the app shall save nothing and change nothing.
- REQ-0001-33 If Save or Quit is requested during a stroke, then the app shall first end the stroke as if the button were released, then proceed.

### Quit
- REQ-0001-34 When the user quits (Ctrl+Q, Ctrl+W or closing the window) and the picture is not modified, the app shall close at once.
- REQ-0001-35 When the user quits and the picture is modified, the app shall show one prompt with heading "Save Changes?", body "“<name>” has unsaved changes." and buttons Cancel, Discard, Save; Save is the default (Enter) and Esc means Cancel.
- REQ-0001-36 When the user chooses Save in the quit prompt, the app shall save as in REQ-0001-27..29 and close only if the save succeeds.
- REQ-0001-37 When the user chooses Discard, the app shall close without saving; when the user chooses Cancel or dismisses the prompt, the app shall stay open with the picture unchanged.
- REQ-0001-38 While the quit prompt is open, further quit requests shall not open a second prompt.

### Before the picture exists
- REQ-0001-39 Until the start-up picture exists (it may take one frame while the window learns its monitor, REQ-0001-02), the app shall ignore drawing, Save and zoom input; Quit closes at once (nothing is modified). (0004: the View and Image actions also do nothing before the picture exists: REQ-0004-03.)

## Scenarios
- AS-1 (REQ-0001-01, REQ-0001-06, REQ-0001-08) Given a settings file with width 300, height 200, background #FFFF00, When the app starts, Then one window shows a 300x200 yellow picture at 100 %, Pan active, foreground black, background color yellow | Verified by: automated (ui) + manual M1
- AS-2 (REQ-0001-02, REQ-0001-41) Given width=auto and height=auto on a 1920x1200 monitor at scale 1, When the app starts, Then the window is 1440x900 and the picture is 1024x768 with space right and bottom and no scrollbars; given a canvas area of 1000x700, the picture is 800x600; given 600x400, it is 640x480 | Verified by: automated (core sizes, ui picture fits view) + manual M2
- AS-3 (REQ-0001-03, REQ-0001-11) Given background=transparent, When the app starts, Then every pixel is #00000000 and the canvas shows a checkerboard | Verified by: automated (core) + manual M3
- AS-4 (REQ-0001-04) Given no settings file, When the app starts, Then the file exists with width=auto, height=auto, canvas-color=#FFFFFF | Verified by: automated (core)
- AS-5 (REQ-0001-05) Given a settings file with width=abc, height=200, canvas-color=#00FF00, When the app starts, Then width is auto, so the size is chosen as in REQ-0001-02 (height 200 is ignored), the picture's color is green, and no message is shown | Verified by: automated (core)
- AS-6 (REQ-0001-05) Given a settings file of binary garbage, When the app starts, Then all values are defaults and the file is byte-for-byte unchanged | Verified by: automated (core)
- AS-7 (REQ-0001-02, REQ-0001-05) Given width=0, then width=-5, then width=10001, When the app starts each time, Then width falls back to auto each time | Verified by: automated (core)
- AS-36 (REQ-0001-40) Given a settings file with a `# note` line, `width = 05 `, `Width=9`, `background=TRANSPARENT`, `height=7` then `height=8`, and a section `[other]` with `width=3`, When it is parsed, Then width is 5, height 8 and background #00000000; Given a file with a line `hello`, Then every value is its default | Verified by: automated (core)
- AS-37 (REQ-0001-42) Given b.png exists, When the user enters "b" in the save dialog, Then a prompt asks to replace “b.png”; Cancel leaves b.png unchanged and the picture modified; Replace writes it | Verified by: manual M20
- AS-38 (REQ-0001-43) Given a modified untitled picture and its save dialog open, When the user closes the window and chooses Save in the prompt, Then the app stays open and modified; When the user closes the window again, Then the prompt appears again | Verified by: automated (ui)
- AS-39 (REQ-0001-03) Given `background=#00FF00` and no `canvas-color`, When the app starts, Then the new picture is green; Given `canvas-color=#0000FF` and `background=#00FF00` in either order, Then it is blue; Given `canvas-color=red` (invalid) and `background=#00FF00`, Then it is green; the file is not rewritten | Verified by: automated (core)
- AS-8 (REQ-0001-07) Given the app is running, When it is started again, Then the existing window comes to the front and there is still one window | Verified by: automated (ui) + manual M4
- AS-9 (REQ-0001-09, REQ-0001-10) Given the app just started, When the user clicks Pencil, Then Pencil is selected, Pan is not, and the cursor over the canvas is a pencil; When the user clicks Pan, Then the cursor is an open hand, and a closed hand while dragging | Verified by: automated (ui, selected state) + manual M5
- AS-10 (REQ-0001-12, REQ-0001-13) Given a fresh start, When the user draws one dot, Then the title is "• Untitled – Easel"; When the user saves as "a.png", Then the title is "a.png – Easel" | Verified by: automated (ui)
- AS-11 (REQ-0001-14) Given a 2000x2000 picture at 100 % with the top-left visible and Pan active, When the user drags left by 50 device pixels, Then the view shows 50 picture pixels further right and no pixel and no modified state changed; When the user then drags right by 500, Then the view stops at the left edge | Verified by: manual M6
- AS-12 (REQ-0001-15) Given a 2000x2000 picture and Pencil active, When the user turns the wheel down, Then the view scrolls down and the picture is unchanged; Shift + wheel scrolls sideways | Verified by: manual M7
- AS-13 (REQ-0001-16, REQ-0001-18) Given Pan active at 100 %, When the user turns Ctrl + wheel up one notch with the pointer over pixel (40,30), Then the zoom is 200 %, pixels show as solid 2x2 squares and pixel (40,30) is still under the pointer; When the user turns 20 more notches up, Then the zoom is 800 %; 20 notches down give 10 % | Verified by: automated (core, zoom steps and anchor) + manual M8
- AS-14 (REQ-0001-18) Given a picture of alternating black and white columns at 50 %, When it is shown, Then every device pixel is pure black or pure white, never grey | Verified by: manual M9
- AS-15 (REQ-0001-17) Given Pencil active, When the user turns Ctrl + wheel, Then the zoom stays the same | Verified by: manual M10
- AS-16 (REQ-0001-19) Given a white picture, When the user primary-clicks at (10,10), Then pixel (10,10) is black and no other pixel changed | Verified by: automated (core)
- AS-17 (REQ-0001-19) Given a white picture at 400 %, When the user primary-clicks anywhere inside the square showing pixel (3,3), Then only pixel (3,3) turns black | Verified by: automated (core)
- AS-18 (REQ-0001-20) Given a white picture, When the pointer moves from (0,0) to (20,5) with no position reported in between, Then the black pixels form an 8-connected 1-pixel line from (0,0) to (20,5) and every other pixel is pure white | Verified by: automated (core)
- AS-19 (REQ-0001-21) Given a picture filled black and background color white, When the user secondary-drags from (0,0) to (9,0), Then pixels (0..9,0) become white | Verified by: automated (core)
- AS-20 (REQ-0001-22) Given a 100x100 white picture, When the user drags from (50,50) to (150,50), Then pixels (50..99,50) are black, every other pixel is white, and no error is shown; When the user presses at (-10,50) and drags to (10,50), Then pixels (0..10,50) are black | Verified by: automated (core)
- AS-21 (REQ-0001-23, REQ-0001-24) Given a picture with an earlier stroke, When the user primary-drags and then presses the secondary button before releasing, Then the picture equals its state before the cancelled stroke and the modified state is unchanged; pressing the middle button instead changes nothing and the stroke continues | Verified by: automated (core)
- AS-22 (REQ-0001-25) Given a stroke in progress, When the window loses focus, Then the stroke ends and its pixels stay | Verified by: manual M11
- AS-23 (REQ-0001-26) Given an unmodified picture whose pixel (5,5) is black, When the user primary-clicks (5,5), Then the picture stays unmodified | Verified by: automated (core)
- AS-24 (REQ-0001-27, REQ-0001-30) Given a new picture with a drawing, When the user presses Ctrl+S, Then the dialog opens in Pictures suggesting Untitled.png; When the user chooses "a.png", Then a.png decodes to the same size and pixels and the mark is cleared | Verified by: automated (io, headless round trip) + manual M12
- AS-25 (REQ-0001-28) Given the save dialog, When the user enters "b", Then b.png is written; When the user enters "c.jpg", Then an error says only PNG can be saved and no file is written; "d." gives d.png, ".png" gives .png.png and "E.PNG" stays E.PNG | Verified by: automated (core, name rule) + manual M13
- AS-26 (REQ-0001-29) Given a picture saved as a.png, When the user draws and presses Ctrl+S, Then no dialog appears and a.png holds the new pixels | Verified by: automated (ui)
- AS-27 (REQ-0001-30) Given a transparent picture with one black dot, When it is saved and decoded, Then the dot is opaque black and every other pixel is #00000000 | Verified by: automated (io, headless)
- AS-28 (REQ-0001-31) Given a modified picture, When the user saves into a read-only folder, Then an error dialog names the file and the reason, and the title and modified mark are unchanged | Verified by: manual M14
- AS-29 (REQ-0001-31) Given a.png exists, When writing new content to a.png fails partway, Then a.png still holds its old content | Verified by: automated (io, headless; skipped when run as root)
- AS-30 (REQ-0001-32) Given a modified untitled picture, When the user presses Ctrl+S and cancels the dialog, Then no file is written and the title is unchanged | Verified by: manual M15
- AS-31 (REQ-0001-33) Given a primary stroke in progress, When the user presses Ctrl+S, Then the stroke ends with its pixels kept and the saved file contains them | Verified by: manual M16
- AS-32 (REQ-0001-34) Given an unmodified picture, When the user presses Ctrl+Q, Then the app closes with no prompt; Ctrl+W behaves the same | Verified by: manual M17
- AS-33 (REQ-0001-35, REQ-0001-37, REQ-0001-38) Given a modified picture, When the user closes the window, Then one prompt shows "Save Changes?" with Cancel, Discard, Save; pressing Ctrl+Q again opens no second prompt; Esc keeps the app open unchanged; Discard closes it and writes no file | Verified by: manual M18
- AS-34 (REQ-0001-36) Given a modified untitled picture, When the user chooses Save in the quit prompt and then cancels the file dialog, Then the app stays open and modified; When the save succeeds instead, Then the app closes | Verified by: manual M19
- AS-35 (REQ-0001-39) Given a window whose picture does not exist yet, When the user presses on the canvas with the Pencil, presses Ctrl+S or turns Ctrl + wheel in Pan, Then nothing happens and no error is shown | Verified by: automated (ui)

## Edge cases
Each is covered by the REQ named.
- Click exactly on the last row or column: REQ-0001-19 (AS-20 checks x = 99).
- A drag that leaves and re-enters the picture: segments are clipped, REQ-0001-22.
- Very fast drags: no gaps, REQ-0001-20.
- Secondary-button drag in Pan mode does nothing (the press opens Pan's menu, REQ-0021-13): REQ-0001-14 covers only the primary button.
- Zoom at a view edge: the anchor point moves as little as the edge limits allow, REQ-0001-16.
- Save to an existing file through the dialog: the dialog's own overwrite confirmation applies, REQ-0001-27; when `.png` was added, REQ-0001-42.
- Disk full or file removed mid-save: REQ-0001-31.
- The first frame before the picture exists: REQ-0001-39.
- Quit pressed again while the quit prompt is open: no second prompt (REQ-0001-38); the open prompt closes as Cancel (standard libadwaita behavior).

## Out of scope
Undo/redo (0002), palette, color swatches and color choice (0003), Fit to Window, touchpad pinch zoom, grid, resize handles,
status bar, Attributes (0004), Open, Save As, other formats, command-line files (0005), every tool except Pan and Pencil
(0006+), Pan right-click menu (Window Extras), Image and View menus, Preferences dialog (until then the settings file is
edited by hand), primary menu contents beyond Quit (Window Extras).

## Open questions
(none)

## Behavior reference
- `docs/reference/new-behaviors.md`: Pan mode is the start-up tool; left drag pans; Ctrl + wheel zooms only in Pan mode; zoom 10 % to 800 %; transparency is real.
- `docs/reference/xp-behaviors.md`: second button cancels a stroke; right button draws with the background color.
- `docs/design.md`: header bar (Save), labelled destructive actions, pointer rules, pending state, settings file.
- XP Paint: pencil is 1 px, hard-edged; picture top-left; default size from the screen; quit prompt Yes/No/Cancel.

## Plan

### Approach
Three layers, each one testable on its own:
- `src/core` (libc only) holds the picture, pencil strokes, line clipping, zoom math, settings parsing and the save-name rule.
- `src/io` (GLib, GIO, gdk-pixbuf; no display) reads and writes the settings file and saves PNG files. Saves write a temporary file and rename it
  (`g_file_set_contents_full`, consistent mode), so a failed save leaves the old file intact.
- `src/ui` (GTK 4 + libadwaita) has:
  - `AdwApplication`, `AdwApplicationWindow` and `AdwToolbarView`;
  - a header bar with a Save button and a primary menu containing Quit;
  - a toolbox of two toggle buttons wired to a stateful `win.tool` action;
  - an `EaselCanvas` widget inside a `GtkScrolledWindow`.

The canvas draws the picture as one `GdkTexture`:
- It is kept up to date with `GdkMemoryTextureBuilder`, which re-uploads only the region marked dirty.
- It is scaled with `GSK_SCALING_FILTER_NEAREST` (P4, REQ-0001-18).
- A repeating checkerboard is drawn under it.
- The canvas size is `picture * zoom / surface scale` in logical pixels, so at 100 % one picture pixel is one device pixel.

Input:
- Pan moves the scrolled window's adjustments.
- Ctrl+wheel is caught by a capture-phase scroll controller, and only in Pan mode.
- The window's `is-active` going false ends any stroke in progress.

Pixel format: **straight (non-premultiplied) `0xAARRGGBB`** (ADR-0002, constitution updated). It keeps exactly the
color the user picked, so PNG round trips are lossless (REQ-0001-30). The texture is declared as `GDK_MEMORY_B8G8R8A8`;
GDK converts to premultiplied internally when drawing (we write no conversion code for display). Blending, which later
features need, divides by alpha in core routines.

The picture **must** be drawn with nearest-neighbour filtering (`GSK_SCALING_FILTER_NEAREST`) at every zoom. This is the
real guard for REQ-0001-18 / AS-14, which can only be checked manually.

### Files
- `meson.build`, `src/meson.build`, `tests/meson.build`
- `src/core/image.{c,h}`: `EaselImage` (w, h, `uint32_t *px`, dirty rect).
- `src/core/stroke.{c,h}`: pencil stroke with an undo log of the pixels it touched, for cancel.
- `src/core/view.{c,h}`: zoom levels, zoom steps, the pointer-to-pixel mapping, the zoom anchor.
- `src/core/settings.{c,h}`: parse and format the settings text.
- `src/core/savename.{c,h}`: the PNG extension rule.
- `src/io/settings-file.{c,h}`: find, read and create `~/.config/easel/settings.ini`.
- `src/io/png.{c,h}`: save a PNG atomically.
- `src/ui/main.c`, `application.{c,h}`, `window.{c,h}`, `window.ui`, `canvas.{c,h}`, `easel.gresource.xml`
- `tests/core/*.c` (plain C, `assert`-style, exit 0/1), `tests/io/test-png.c`, `tests/io/test-settings-file.c`
- `tests/ui/test-window.c` (skips with exit 77 when there is no display; run with `GSK_RENDERER=cairo`)

### Key interfaces (core)
- `EaselImage *easel_image_new (int w, int h, uint32_t fill)`; `uint32_t easel_image_get (img, x, y)`;
  `void easel_image_set (img, x, y, uint32_t)`. This is the one dirty-marking path; it grows `img->dirty`.
- `EaselStroke *easel_stroke_begin (img, x, y, uint32_t color)`; `easel_stroke_to (s, x, y)`;
  `size_t easel_stroke_end (s)` (returns the number of pixels changed; since 0002 it is `easel_stroke_end (s, history)`); `easel_stroke_cancel (s)` (restores from its log).
  It draws 8-connected Bresenham lines, clipped by skipping the pixels outside the picture (coordinates clamped to ±1,000,000).
- `int easel_view_zoom_step (int pct, int notches)`; `int easel_view_content_size (int picture_size, int pct, double scale)`;
  `void easel_view_to_pixel (double x, double y, int pct, double scale, int *px, int *py)` (x, y relative to the
  picture's top-left); `double easel_view_anchor (...)` gives the scroll offset per axis that keeps a picture point under
  the pointer; `int easel_view_take_notches (double *acc, double delta)` adds touchpad scrolling up into whole notches.
- `EaselSettings` {width, height (0 = auto), uint32_t background}; `void easel_settings_parse (const char *text, EaselSettings *out)`;
  `char *easel_settings_default_text (void)`.
- `char *easel_savename_fix (const char *path, int *err)`.

### Architecture notes
These go into `docs/engineering.md` in task T001:
- the three layers above;
- data flow for an edit: input → canvas → `easel_stroke_*` → dirty rect → texture update → redraw;
- settings: one file, parsed in core, file handling in io.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_starts_with_settings` + manual M1 |
| 2 | core `as2_auto_size_fits_area`, core `as2_window_size`, ui `as2_auto_fits_view` + manual M2 |
| 3 | core `as3_transparent_background` + manual M3 |
| 4 | io `as4_creates_default_file`, core `as4_default_text_parses_to_defaults` |
| 5 | core `as5_invalid_value_falls_back` |
| 6 | io `as6_garbage_file_untouched`, core `as6_garbage_is_all_defaults` |
| 7 | core `as7_size_limits` |
| 36 | core `as36_syntax_rules` |
| 8 | ui `as8_one_window` + manual M4 |
| 9 | ui `as9_tool_selected_state` + manual M5 |
| 10 | ui `as10_title_tracks_state` |
| 11 | manual M6 |
| 12 | manual M7 |
| 13 | core `as13_zoom_steps_and_anchor`, ui `as13_zoom_keeps_point_under_pointer` + manual M8 |
| 14 | manual M9 |
| 15 | manual M10 |
| 16 | core `as16_click_sets_one_pixel` |
| 17 | core `as17_zoomed_click_maps_to_pixel` |
| 18 | core `as18_line_is_8_connected` |
| 19 | core `as19_secondary_uses_background` |
| 20 | core `as20_clipped_line` |
| 21 | core `as21_cancel_restores`, ui `as21_pencil_on_canvas` (middle button ignored) + manual M11 |
| 22 | manual M11 |
| 23 | core `as23_noop_stroke_not_modified` |
| 24 | io `as24_png_round_trip` + manual M12 |
| 25 | core `as25_save_name_rule` + manual M13 |
| 26 | ui `as26_save_existing_no_dialog` |
| 27 | io `as27_transparent_round_trip` |
| 28 | manual M14 |
| 29 | io `as29_failed_save_keeps_old_file` (exit 77 when run as root, which ignores read-only folders) |
| 30 | manual M15 |
| 31 | manual M16 |
| 32 | manual M17 |
| 33 | ui `as33_one_quit_prompt` + manual M18 |
| 34 | manual M19 |
| 35 | ui `as35_input_ignored_before_picture` |
| 37 | manual M20 |
| 38 | ui `as38_quit_save_while_dialog_open` |

Helper tests, tagged in the test files with the requirement they support: core `image_rejects_bad_size` (REQ-0001-01/02),
`image_get_outside_is_transparent` (REQ-0001-22), `image_set_marks_dirty` (REQ-0001-19/20), `content_size_covers_picture`
(REQ-0001-08/16); io `reads_existing_file` (REQ-0001-01), `unreadable_file_is_defaults_and_untouched` (REQ-0001-05),
`uncreatable_file_is_defaults` (REQ-0001-04/05).

AS-14 is manual only: the core does no scaling, so no automated test can cover it (see the nearest-neighbour rule above).
AS-24, AS-27 and AS-29 are io tests: GIO and gdk-pixbuf are not in the core, but they need no display.

### Constitution check
- P4: nearest-neighbour filtering and device-pixel sizing.
- P5: no threads; the only partial texture update is GTK's own builder.
- P6: the settings file.
- Layering: core is libc only.
- Pixel format: changed by ADR-0002, which the constitution itself allows ("revisit in the first plan").

### Measurements (T015, 2026-10-15)
Benchmark `perf-0001` (`meson test -C build --benchmark --verbose`), default GTK renderer, AMD Ryzen AI 7 PRO 350
with Radeon 860M, 60 Hz display. Two runs; the larger value is shown.

| Budget | Measured | Verdict |
|---|---|---|
| Start to usable window < 0.5 s | 116 ms from `main()` to the first painted frame (1024x768); process launch and library loading are not included | met |
| Pointer event to pixels within one frame on 1920x1080 | stroke work 0.76 ms median, 2.2 ms worst; on screen at the next refresh (16.7 ms median); one 41 ms outlier in 60 | met |
| 8000x8000 opens and draws without freezing | opens in 140 ms; each pencil motion costs about 51 ms of work (the whole picture is copied to the screen texture), about 89 ms to screen, so roughly 11 updates per second | does not freeze, but lags; below one frame |

Follow-up for JOD to decide (P5: a measurement now justifies it): copy only the changed area to the screen texture
instead of the whole picture. Not done in 0001.

### Risks
- **Monitor before the window is shown.** On Wayland the monitor is known only after the window appears, so the
  initial window size uses the first monitor when there are several (REQ-0001-41). An `auto` picture is created on the
  canvas area's first size allocation; the standard sizes list is a constant until Preferences makes it a setting. The window may show an
  empty canvas for one frame; REQ-0001-39 makes input in that frame do nothing.
- **AS-29 "fails partway".** A real mid-write failure cannot be triggered in a test. The test uses a read-only directory with
  the target already in it. This proves the failure is reported and the old file survives; the temp-file-and-rename write
  covers the partway case. The test skips when run as root.
- **HiDPI start-up size.** An `auto` picture uses the monitor's scale, because a Wayland surface learns its own scale
  only after its first frame. Untested on a HiDPI screen (none available); check M2 on one when possible.
- **Fractional scaling.** Canvas size in logical pixels is rounded, so at 125 % scale the edge pixel can be off by one;
  manual check M1.
- **ui tests on a live display.** They open real windows, so mouse or wheel input during `meson test` can disturb them
  (one as13 run failed once this way). Later: run them under a headless compositor.
- **Touchpad zoom.** Touchpad scrolling arrives in pixels; 30 px counts as one notch (a feel constant, adjust after M8).
- **Fast motion.** GTK compresses motion events; the line between consecutive positions keeps the stroke free of gaps (REQ-0001-20).

## Tasks
- [x] T001 Build skeleton and single-instance window (REQ-0001-07, AS-8) - done when meson builds warning-free, the ui test `as8_one_window` passes (skips with 77 when there is no display), the app opens one empty window, a second launch focuses it (manual M4), and `docs/engineering.md` Architecture is filled in
- [x] T002 Core image and settings parsing (REQ-0001-01, REQ-0001-02, REQ-0001-03, REQ-0001-05, REQ-0001-40; AS-2, AS-3, AS-5, AS-7, AS-36) - done when the core tests for those scenarios pass
- [x] T003 Settings file read and create (REQ-0001-01, REQ-0001-04, REQ-0001-05; AS-4, AS-6) - done when the io tests pass against a temporary config directory
- [x] T004 Core pencil stroke: line, clip, cancel, change count (REQ-0001-19, REQ-0001-20, REQ-0001-21, REQ-0001-22, REQ-0001-23, REQ-0001-26; AS-16, AS-18, AS-19, AS-20, AS-21, AS-23) - done when the core tests pass
- [x] T005 Core view math: zoom steps, anchor, pointer to pixel (REQ-0001-16, REQ-0001-19; AS-13, AS-17) - done when the core tests pass
- [x] T006 Canvas shows the start-up picture (REQ-0001-06, REQ-0001-08, REQ-0001-11, REQ-0001-18; AS-1, AS-2, AS-3) - done when the ui test AS-1 passes and manual M1, M2, M3 pass
- [x] T007 Toolbox, cursors and pencil drawing on the canvas (REQ-0001-09, REQ-0001-10, REQ-0001-19..25, REQ-0001-26; AS-9, AS-21, AS-22) - done when the ui test AS-9 passes and manual M5, M11 pass
- [x] T008 Pan, scroll and Ctrl+wheel zoom (REQ-0001-14, REQ-0001-15, REQ-0001-16, REQ-0001-17, REQ-0001-18; AS-11, AS-12, AS-13, AS-14, AS-15) - done when manual M6, M7, M8, M9, M10 pass
- [x] T009 PNG save and the save-name rule (REQ-0001-28, REQ-0001-30, REQ-0001-31; AS-24, AS-25, AS-27, AS-29) - done when the core test AS-25 and the io tests AS-24, AS-27, AS-29 pass
- [x] T010 Save in the UI: dialog, overwrite, errors, cancel, title and modified mark, mid-stroke (REQ-0001-12, REQ-0001-13, REQ-0001-27..33, REQ-0001-42; AS-10, AS-24, AS-25, AS-26, AS-28, AS-30, AS-31, AS-37) - done when the ui tests AS-10, AS-26 pass and manual M12..M16, M20 pass
- [x] T011 Quit flow and prompt (REQ-0001-33..38; AS-32, AS-33, AS-34) - done when manual M17, M18, M19 pass
- [x] T012 Ignore input before the picture exists (REQ-0001-39; AS-35) - done when the ui test AS-35 passes
- [x] T013 Window size from the monitor; an auto picture is the largest standard size that fits the canvas area (REQ-0001-02, REQ-0001-05, REQ-0001-41; AS-2, AS-5) - done when core `as2_auto_size_fits_area`, `as2_window_size`, `as5_invalid_value_falls_back` and ui `as2_auto_fits_view` pass and manual M2 passes again
- [x] T014 Quit-prompt Save while a save dialog is open (REQ-0001-43; AS-38) - done when the ui test `as38_quit_save_while_dialog_open` fails before the fix and passes after
- [x] T015 Measure the performance budgets of `docs/engineering.md` (start-up: REQ-0001-06; drawing: REQ-0001-20) - done when the benchmark `perf-0001` runs and its numbers are recorded under Plan / Measurements
