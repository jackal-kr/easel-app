# 0004 Canvas view

Status: implemented
Depends on: 0001, 0002, 0003

## Why
0001 gave a picture you can pan and zoom with the wheel, but no way to see the zoom, jump to a useful size, see pixel
boundaries, or change the picture's size. This feature adds the view tools of XP Paint (zoom commands, Fit to Window, the
pixel grid, a status bar) and the two ways XP changes the canvas size (drag handles and Attributes), and makes a size
change an undoable edit.

## Requirements
Look and input rules come from `docs/design.md` (header bar View and Image menu buttons; status bar; Pan never changes
the picture). Words used below: the *canvas area* is the scrolled area that shows the picture, without its scrollbars; a
*level* is one of the zoom levels 10, 25, 50, 100, 200, 300, 400, 600, 800, 1600 % (REQ-0001-16).

### Menus
- REQ-0004-01 (Pointer note, Window Extras, 2026-11-03: View also holds Show Thumbnail, directly after Grid…, REQ-0021-06.) The header bar shall have an Image menu button and, right after it, a View menu button, each showing a symbolic icon before its label ("Image", "View"), then the primary menu (design.md). Image shall hold one item for now: Attributes… (Ctrl+E); later features add theirs (Image Transforms). View shall hold, in order:
  - a zoom row: a Zoom Out button (tooltip "Zoom Out (Ctrl+-)"), the current zoom as `N%` (for example `100%`), and a Zoom In button (tooltip "Zoom In (Ctrl+=)"); Zoom In also answers to Ctrl++ and the numpad plus with Ctrl, Zoom Out to Ctrl+numpad minus;
  - a separator;
  - Normal Size (Ctrl+PageUp), Large Size (Ctrl+PageDown), Fit to Window (Ctrl+0) (0008 puts a Zoom submenu with the zoom presets first in this group: REQ-0008-03);
  - a separator;
  - Show Grid (Ctrl+G, a check item) and Grid…;
  - a separator;
  - Tool Box, Color Box and Status Bar, check items that are checked while the panel is shown (REQ-0004-27).
  View has no Frame Time item (it is a Preferences setting) and no Show Thumbnail item (Window Extras adds it). Each item shows its shortcut. The numpad plus and minus without Ctrl keep their own meaning (brush size, 0006). (Pointer note, Image Transforms, 2026-10-27: Image now holds Flip ›, Rotate ›, Invert Colors, Attributes… and Clear Image in three sections, REQ-0018-01.)
- REQ-0004-02 A control that cannot do anything shall be insensitive: Zoom In at 1600 %, Zoom Out at 10 %, Normal Size at 100 %, Large Size at 400 %.
- REQ-0004-03 (Pointer note, Wheel zoom in every tool, 2026-11-09: the zoom commands, Ctrl+wheel and the pinch do nothing while anything is held on the canvas, also Pan's drag, a selection or handle drag and a held key press, REQ-0036-02.) The View and Image actions and their shortcuts work wherever keyboard focus is in the window, and do nothing before the picture exists (REQ-0001-39), while a dialog, the quit prompt or the color chooser is open, and while a mouse button is held for a stroke or a resize drag.

### Zoom
- REQ-0004-04 (Pointer note, Wheel zoom in every tool, 2026-11-09: the wheel and the pinch now zoom with every tool, REQ-0036-01.) When the user chooses Zoom In or Zoom Out (the zoom row's buttons or the shortcuts), the app shall move one level in or out (from a zoom between two levels, to the next level above or below it), with any tool active; the wheel and the pinch of REQ-0004-08 keep working only in Pan (REQ-0001-17).
- REQ-0004-05 When the user chooses Normal Size, the app shall set the zoom to 100 %; Large Size sets it to 400 %.
- REQ-0004-06 A zoom command from the menu or keyboard shall keep the point at the center of the canvas area still, as far as the view's edges allow (REQ-0001-16 does the same for the pointer). A picture smaller than the canvas area stays at its top-left (REQ-0001-11). (0009: the scroll counts the margin of REQ-0009-01; a smaller picture stays at the margin.)
- REQ-0004-07 When the user chooses Fit to Window, the app shall set the zoom to the largest whole percent at which the whole picture fits in the canvas area, comparing device pixels with no scrollbars counted, but not below 10 % and not above 800 % (the cap of Fit to Window, which is no longer the largest zoom: that is 1600 %, REQ-0001-16; JOD, 2026-10-27), and show the picture's top-left corner. The zoom is then usually not one of the levels (for example 16 %), so that the picture reaches the right or the bottom edge of the area as closely as a whole percent allows; if even 10 % is too big for the picture, it gets 10 % and scrollbars, and a picture smaller than the canvas area even at 800 % gets 800 %. Zoom In and Zoom Out from such a zoom go to the next level above or below it (REQ-0004-04), and the zoom row and the status bar show it as `N%`. The zoom does not follow later window size changes. (This replaces the first version of this requirement, which chose a level; the owner saw a 5840 x 5184 photo get 10 % in a canvas area where 16 % fits.) (0009: the picture and its two margins must fit: REQ-0009-05.)
- REQ-0004-08 (Pointer note, Wheel zoom in every tool, 2026-11-09: with every tool, not while anything is held, REQ-0036-01, -02.) While Pan is active and the user pinches on a touchpad or touch screen, the app shall move one level each time the pinch's scale since the last level change reaches 1.5 (in) or 1/1.5 (out), keeping the pinch center still as far as the edges allow. The scale starts again after each level change, at 1600 % and 10 % and when the pinch ends. With any other tool the pinch does nothing. (1.5 is a fixed value of this spec.)

### Status bar
- REQ-0004-09 (Pointer note, Brush options that read at a glance, 2026-11-07: a part with the selected size preset for the tools with sizes, REQ-0030-08.) (Pointer note, Window Extras, 2026-11-03: a Frame time part may stand left of the position, REQ-0021-12; over the thumbnail no position shows, REQ-0021-09.) The window shall have a status bar below the palette bar, across the window's full width. At its left it shows a one-line hint for the active tool; at its right edge, in this order, it shows the pointer position as `x, y` in picture pixels, the zoom as `N%`, and the picture size as `W × H px`. The position is shown while the pointer is over the canvas area, also outside the picture (so it can be negative or beyond the size), and is empty elsewhere. Pan's hint is "Drag to pan. Ctrl+wheel zooms." and Pencil's is "Draw with the foreground color. The secondary button uses the background color." All its texts are translatable (i18n). (Selection: the selection size "W × H" just before the picture size, REQ-0013-06.)
- REQ-0004-10 Each field shall show a new value within the next frame drawn after it changes. During a resize drag the size field shows the new size; after a cancel it shows the picture's size again. Before the picture exists the position, size and zoom fields are empty.
- REQ-0004-11 The status bar shall never make the window wider: when there is no room, the hint is shortened with "…" at its end. (Selection adds the selection size next to the picture size.)

### Pixel grid
- REQ-0004-12 When the user chooses Show Grid or presses Ctrl+G, the app shall toggle the grid. While it is on, grid lines are drawn over everything shown inside the picture: the picture, its checkerboard, the pixels of a floating selection and the pixels of an open text box (Opaque or Transparent, while it is edited and after it floats), and under the outlines, handles and caret, never outside the picture (JOD, 2026-10-27; the text box hid them before), 1 device pixel wide, hard-edged, grey #808080 at 50 % opacity, at every x and y that is a positive multiple of the cell size in picture pixels and inside the picture; the picture's own edges get no line. The lines are drawn only while one cell is at least 4 device pixels wide on screen (so the 1-pixel grid first shows at 400 %). The check item stays checked while the lines are hidden, and Ctrl+G still toggles. The grid is not part of the picture: it is not saved, not an edit, and does not change the modified mark. (0009: the lines are counted from the picture's own origin, 12 pixels in: REQ-0009-05.)
- REQ-0004-13 When the user chooses Grid…, the app shall open a modal dialog "Grid" with the cell size (a whole number from 1 to 1000 picture pixels) and an Axonometric check box, and the buttons Cancel and Apply. Apply is insensitive while the cell size is empty, 0, not a whole number or above 1000. Enter applies, Esc cancels, and Apply never switches the grid on or off.
- REQ-0004-14 While Axonometric is on, the app shall draw the vertical lines as before and, instead of the horizontal lines, two families of diagonal lines at a slope of 1 picture pixel down for every 2 across: the lines through y = x / 2 + k × cell and y = -x / 2 + k × cell for every whole k, hard-edged and clipped to the picture. The 4-device-pixel rule of REQ-0004-12 applies to the cell size.
- REQ-0004-15 (Pointer note, Preferences, 2026-11-03: only a change in Preferences writes these keys, REQ-0020-08.) The grid starts as the settings file says (syntax as REQ-0001-40): section `[view]`, key `grid` (`on` or `off`, default `off`), key `grid-size` (a whole number 1 to 1000, default 1), key `grid-axonometric` (`on` or `off`, default `off`), and the panel keys `toolbox`, `colorbox` and `statusbar` (`on` or `off`, default `on`; REQ-0004-27). A missing or invalid value means its default, silently; an unparseable file gives every default (REQ-0001-05). Changes made in the window last until the app quits, and the settings file is never rewritten for them. A settings file the app creates also holds `[view]` with the defaults (amends REQ-0001-04 and REQ-0002-19).

### Changing the picture's size
- REQ-0004-16 (Pointer note, Extra shapes, 2026-11-11: a waiting shape's handles win over these where their hit areas overlap, REQ-0043-07.) While the picture exists and a tool other than Pan is active, the app shall show three resize handles just outside the picture, touching its edge: at the middle of its right edge, at the middle of its bottom edge and at its bottom-right corner, each 5 x 5 logical pixels with a hit area of 9 x 9 around its center. They scroll with the picture, the scrollable area includes them, and where hit areas overlap the corner handle wins. Pan hides them, because nothing in Pan mode changes the picture. The primary button on a handle starts a resize; the secondary or middle button on it does nothing. (0009: the handles lie in the margin around the picture, which the scrollable area always holds: REQ-0009-01.) (Pointer note, Selection handles, 2026-10-22: while a selection exists its own handles win over these where their hit areas overlap.) (Pointer note, Text, 2026-10-26: Outside an open text box, its handles and then its move band win over these handles' hit areas, REQ-0017-09.)
- REQ-0004-17 While the pointer is over a handle, the cursor shall be the horizontal resize cursor (right handle), the vertical resize cursor (bottom handle) or the diagonal resize cursor (corner handle), instead of the tool's cursor. (Pointer note, Tool cursors, 2026-11-06: the cursors named here are drawn by Easel in black with a white outline; which cursor shows where stays as here.)
- REQ-0004-18 While the user drags a handle with the primary button, the app shall show the new picture's outline: the right handle sets the width, the bottom handle the height, the corner both, each moving the picture's edge by the pointer's movement since the press (a click without moving changes nothing), the result rounded to the nearest pixel boundary and limited to 1 to 10000. The drag goes on while the pointer is outside the canvas area or the window, and the view does not scroll. On release the app shall resize the picture to the outlined size.
- REQ-0004-19 If the user presses Esc, presses the other mouse button, loses the window's focus, or asks to save or quit during a resize drag, then the app shall cancel it and change nothing. Until the button is released, Undo, Redo, the View and Image actions and the grid shortcut are ignored.
- REQ-0004-20 When the picture grows, the new area shall be filled with the background color current at that moment (a transparent background gives transparent pixels); when it shrinks, the pixels outside the new size are removed; the pixels that stay are unchanged. This holds for handles and for Attributes alike. (Pointer note, Clipboard, 2026-10-22: Resize Canvas to Picture is a third way to grow it, REQ-0016-07.)
- REQ-0004-21 (Pointer note, Preferences, 2026-11-03: Default uses a size changed in Preferences, REQ-0020-04.) When the user chooses Attributes…, the app shall open a modal dialog "Attributes" with the line "New area is filled with the background color; a smaller size crops the picture." under its heading, then Width and Height in pixels (whole numbers from 1 to 10000, starting at the current size), each a number field with a minus and a plus button that change it by 1, a Default button, and the buttons Cancel and OK. A typed number below 1 becomes 1 and above 10000 becomes 10000, and text that is not a whole number goes back to the value the field had; so OK is always sensitive and always gives a size from 1 to 10000. Default fills in the size a new picture would get now: the settings file's `width` and `height`, with `auto` resolved as REQ-0001-02 does against the canvas area at that moment. Enter is OK and Esc is Cancel. OK resizes as REQ-0004-20; Cancel changes nothing. No units other than pixels, and no aspect-ratio option.
- REQ-0004-22 A resize that changes the picture's size shall be exactly one undo step (REQ-0002-01). Undoing it restores the previous size and every pixel exactly, including the pixels that were cropped (transparent ones too); redoing it gives the same result as the first time, including the fill color used then, even if the background color has changed since. Steps before and after a resize stay undoable through it, and the modified mark and the saved state follow the history as in REQ-0002-12..14. (Pointer note, Clipboard, 2026-10-22: the growth of Resize Canvas to Picture is not a step of its own: it is one step with the paste's put-down, REQ-0016-07.)
- REQ-0004-23 A resize step counts for the memory budget as 4 bytes per pixel it removes plus 64 bytes (pixels it adds need no storage). It is one step for REQ-0002-16 and REQ-0002-17: a step bigger than the whole budget stays as the only step and the older steps are dropped silently. (Pointer note, Clipboard, 2026-10-22: that joint step costs this for the growth plus REQ-0002-16 for the pixels put down, REQ-0016-07.)
- REQ-0004-24 A resize to the size the picture already has (a drag that ends where it started, a click on a handle, Attributes with unchanged values) shall add no undo step and change nothing.
- REQ-0004-25 After a resize, undo or redo that changes the size, the zoom shall stay, the scroll position shall be kept as far as the new size allows (as REQ-0002-08), the window shall keep its size, and scrollbars appear or disappear as needed.
- REQ-0004-26 If the memory for the new picture or its undo step cannot be had, then the app shall show an error dialog, heading "Could Not Resize the Picture" and body "There is not enough memory for a picture of W × H pixels." (with the numbers), and change nothing: neither the picture nor the history. The same holds when Undo or Redo of a resize needs memory for the size it restores and cannot get it: the same dialog appears (with the numbers of the size that could not be had), the picture stays as it is, and the history position stays where it was, so the same Undo or Redo can be tried again.

### Panels
- REQ-0004-27 When the user chooses Tool Box, Color Box or Status Bar in the View menu, the app shall hide that panel if it is shown, or show it if it is hidden, at once, and update the check item. Hiding Tool Box hides the tool buttons (the active tool stays); hiding Color Box hides the whole palette bar (indicator, swap button and swatches) while colors still change with Ctrl+Shift+X and strokes still use them; hiding Status Bar hides all its fields. The canvas area takes the freed space. The window's minimum width of REQ-0003-03 applies only while the Color Box is shown. A panel starts shown or hidden as the settings file says (REQ-0004-15) and changes last until the app quits. Showing or hiding a panel is not an edit and changes neither the picture, the zoom nor the modified mark. (0006: the options panel below the toolbox hides and shows with it, REQ-0006-02.)

## Scenarios
- AS-1 (REQ-0004-01, REQ-0004-02) Given a fresh start, When the user looks at the header bar, Then an Image button then a View button, each with an icon and its label, precede the primary menu; When the user opens View, Then it shows the zoom row (Zoom Out, `100%`, Zoom In), a separator, Normal Size, Large Size, Fit to Window, a separator, Show Grid, Grid…, a separator, and Tool Box, Color Box and Status Bar checked, with no Frame Time and no Show Thumbnail, each item with its shortcut; Normal Size is insensitive at 100 % and the others sensitive; When the user opens Image, Then it lists Attributes… (Ctrl+E) | Verified by: automated (ui) + manual M1
- AS-2 (REQ-0004-03) Given the quit prompt (or the color chooser, or a stroke in progress) is open, When the user presses Ctrl+0, Ctrl+G or Ctrl+E, Then nothing happens; Given a window whose picture does not exist yet, Then the same | Verified by: automated (ui)
- AS-3 (REQ-0004-04, REQ-0004-02, REQ-0004-01) Given Pencil active at 100 %, When the user clicks the Zoom In button in the View menu, Then the zoom is 200 % and the row shows `200%`; at 1600 % the Zoom In button is insensitive and Ctrl+= does nothing; from 800 % it gives 1600 %; the Zoom Out button from 200 % gives 100 %, and at 10 % it is insensitive | Verified by: automated (core, levels) + automated (ui)
- AS-4 (REQ-0004-05, REQ-0004-02) Given 100 %, When the user presses Ctrl+PageDown, Then the zoom is 400 % and Large Size is insensitive; When the user presses Ctrl+PageUp, Then 100 % | Verified by: automated (ui)
- AS-5 (REQ-0004-06) Given a 2000x2000 picture at 100 % with a canvas area of 1000x800 scrolled to (500, 400), When the user chooses Zoom In, Then picture pixel (1000, 800), the center, is still at the center of the canvas area; Given a picture smaller than the canvas area, Then it stays at the top-left | Verified by: automated (core, anchor math) + automated (ui)
- AS-6 (REQ-0004-07) Given a canvas area of 1390x853 device pixels, When the user chooses Fit to Window, Then an 800x600 picture gets 142 %, a 3000x2000 picture 42 %, a 5840x5184 photo 16 % (its height 829 px, the width 934), a 10000x10000 picture 10 % (with scrollbars), a 174x100 picture 798 % and a 1x1 picture 800 %; one percent more would not fit, except at 800 %; the picture shows from its top-left corner; When the user then chooses Zoom In, Then the zoom is the next level above (16 % gives 25 %, 142 % gives 200 %), and Zoom Out gives the level below (16 % gives 10 %, 142 % gives 100 %); When the window is made larger afterwards, Then the zoom stays | Verified by: automated (core, fit zoom and level steps) + automated (ui)
- AS-7 (Pointer note, Wheel zoom in every tool, 2026-11-09: with every tool the pinch now zooms, REQ-0036-01; the last clause is replaced.) (REQ-0004-08) Given Pan active at 100 %, When the pinch scale reaches 1.5, Then the zoom is 200 % and the scale starts at 1 again; When the pinch then shrinks to 1/1.5 of that, Then back to 100 %; with Pencil active a pinch changes nothing | Verified by: automated (core, pinch accumulator) + manual M2
- AS-8 (REQ-0004-09, REQ-0004-10) Given a 300x200 picture at 200 % with Pan active, When the pointer is over pixel (12, 34), Then the status bar shows Pan's hint at the left and, from left to right at the right edge, "12, 34", "200%" and "300 × 200 px"; When the pointer is over the canvas area outside the picture, Then the position shows negative or beyond-size numbers; When the pointer is over the header bar, Then the position is empty; With Pencil active the hint is Pencil's | Verified by: automated (ui) + manual M3
- AS-9 (REQ-0004-11) Given the narrowest window, Then the status bar does not widen it and Pencil's hint ends with "…" | Verified by: manual M3
- AS-10 (REQ-0004-12) Given the grid off at 400 %, When the user presses Ctrl+G, Then Show Grid is checked and 1-pixel grid lines show every picture pixel inside the picture (none on its edges); When the user zooms to 300 %, Then no lines show but Show Grid stays checked; When the user presses Ctrl+G, Then it is unchecked; the file saved meanwhile has no lines and the title has no modified mark; Given a 3000 × 2000 picture at 400 % (and at 600 % and 800 %) scrolled by more than one page of the view in each direction, When the grid is on, Then the lines show, each on the left or top edge of a picture pixel that is a multiple of the cell, so that in the window the lines are where the picture pixels are drawn, at every scroll position | Verified by: automated (core, line positions and visibility) + automated (ui) + manual M4
- AS-11 (REQ-0004-12) Given cell size 16, When the zoom is 25 %, Then lines show every 16 picture pixels (4 device pixels apart); at 10 % none | Verified by: automated (core)
- AS-12 (REQ-0004-13) Given the Grid dialog, When the cell size field holds "", "0", "abc", "1001" or "-3", Then Apply is insensitive; "1000" and " 16 " (spaces) are valid; When the user presses Esc, Then nothing changes; When the user applies 8 while the grid is off, Then the grid stays off | Verified by: automated (ui)
- AS-13 (REQ-0004-14) Given cell size 4 and Axonometric on, Then the vertical lines are at x = 4, 8, 12 …, and the diagonals are the lines through y = x / 2 + 4k and y = -x / 2 + 4k, clipped to the picture, with no horizontal lines | Verified by: automated (core) + manual M4
- AS-14 (REQ-0004-15) Given `[view]` with `grid=on`, `grid-size=8`, `grid-axonometric=on`, When the app starts, Then the grid is on with cell 8 and Axonometric; `grid=maybe` and `grid-size=0` fall back to off and 1; a garbage file gives all defaults; a newly created file holds `[view]` with the defaults; `toolbox=off`, `colorbox=off` and `statusbar=off` start with those panels hidden and `statusbar=maybe` falls back to on; an existing file without it is byte-for-byte unchanged; changing the grid in the window leaves the file unchanged | Verified by: automated (core) + automated (io)
- AS-15 (REQ-0004-16) Given a 300x200 picture at 100 %, When Pencil is active, Then three handles show just outside the picture's right middle, bottom middle and bottom-right corner, 5x5 each; When Pan is active, Then none show; When the picture is scrolled, Then the handles move with it | Verified by: automated (ui, geometry and visibility) + manual M5
- AS-16 (REQ-0004-17) Given Pencil active, When the pointer is over the right, bottom and corner handles, Then the cursors are horizontal, vertical and diagonal resize | Verified by: manual M5
- AS-17 (REQ-0004-18, REQ-0004-24) Given a 300x200 picture, When the user presses the right handle and drags it 100.4 picture pixels to the right and releases, Then the picture is 400x200; dragging the bottom handle 50 pixels up gives 300x150; dragging the corner by (+200, +100) gives 500x300; dragging far to the left or up gives width or height 1; far to the right or down gives 10000; a drag that ends at the starting size, and a click without moving, add no undo step; the drag works the same at other zoom levels | Verified by: automated (core, size from movement) + automated (ui)
- AS-18 (REQ-0004-19) Given a handle drag in progress, When the user presses Esc, or presses the secondary button, or the window loses focus, or Save or Quit is requested, Then the picture keeps its size; While the drag is in progress, Undo, Ctrl+0 and Ctrl+G do nothing; the other-button press and Esc also work with the pointer outside the window | Verified by: automated (ui) + manual M5
- AS-19 (REQ-0004-20) Given a 4x4 picture with background white, When it grows to 6x5, Then the original 16 pixels are unchanged and the 14 new pixels are white; with a transparent background the new pixels are #00000000; shrinking to 2x2 keeps the top-left 2x2 pixels | Verified by: automated (core)
- AS-20 (REQ-0004-21) Given the Attributes dialog on a 300x200 picture, Then it shows the help line, 300 and 200; When the user presses plus on Width and minus on Height, Then they show 301 and 199; When Width is typed as "0", "10001" or "99999999999999999999", Then it becomes 1, 10000 and 10000; When it is typed as "abc", "1.5" or "", Then it goes back to its previous value; When the user enters 640 and 480 and presses Enter, Then the picture is 640x480; When the user presses Default with `width=auto height=auto` and a canvas area of 1390x853, Then the fields show 1024 and 768; Esc cancels | Verified by: automated (ui) + manual M6
- AS-21 (REQ-0004-22) Given a 4x4 picture with some pixels transparent, When the user shrinks it to 2x2 and presses Undo, Then the picture is 4x4 and every pixel, the transparent ones too, is as before; When the background was changed in between and the user presses Redo, Then the new area has the color of the first time | Verified by: automated (core)
- AS-22 (REQ-0004-22) Given stroke A, a resize and stroke B, When the user undoes three times, Then B, the resize and A are undone in turn, each with the right size and pixels; redoing three times returns to the end | Verified by: automated (core) + automated (ui)
- AS-23 (REQ-0004-22) Given a picture saved as a.png, When the user resizes it, Then the title has the modified mark; When the user undoes the resize, Then the mark is gone and quitting needs no prompt | Verified by: automated (ui)
- AS-24 (REQ-0004-23) Given a 16 MB budget and earlier small strokes, When the user shrinks a 2000x1000 picture to 1x1 (about 8 MB), Then it is one step costing 4 bytes per removed pixel plus 64 and the older steps that no longer fit are dropped; When one resize needs more than 16 MB, Then it stays as the only step | Verified by: automated (core)
- AS-25 (REQ-0004-24) Given a picture, When the user opens Attributes and presses OK with the unchanged size, Then no step is added and Undo stays insensitive | Verified by: automated (ui)
- AS-26 (REQ-0004-25) Given a 2000x2000 picture at 100 % scrolled to (1500, 1500) in a 1000x800 canvas area, When it is resized to 1200x1200, Then the scroll position is the nearest valid one (200, 400), the zoom and the window size are unchanged; When it is resized to 100x100, Then it shows at the top-left; undo gives 2000x2000 again with the same clamping | Verified by: automated (ui) + manual M5
- AS-27 (REQ-0004-26) Given a resize that cannot be allocated, Then the error dialog with the heading and body of REQ-0004-26 appears, and the picture, the title and Undo are unchanged | Verified by: automated (core, refusal) + automated (ui, dialog)
- AS-28 (REQ-0004-27) Given a fresh start, When the user chooses Color Box, Then the palette bar (indicator, swap button, swatches) disappears, the check item is unchecked, and the canvas area is taller by the bar's height; Ctrl+Shift+X still swaps the colors and the next stroke uses the swapped color; When the user chooses Color Box again, Then the bar is back with the current colors; hiding Tool Box hides the tool buttons and keeps the active tool, hiding Status Bar hides its fields; none of this changes the title's modified mark or adds an undo step | Verified by: automated (ui) + manual M7
- AS-29 (REQ-0004-23, REQ-0002-17) Given a 16 MB undo budget and three earlier strokes, When the user shrinks a 4000x2000 picture to 1x1 (about 32 MB of removed pixels, more than the whole budget), Then the resize is kept as the only step and the three strokes can no longer be undone; one Undo restores the 4000x2000 picture with every pixel exactly, and Redo shrinks it again; a later small stroke drops the oversized step, as REQ-0002-16 does for any step | Verified by: automated (core)
- AS-30 (REQ-0004-26) Given a 4000x2000 picture shrunk to 1x1 (one resize step) and memory that can no longer hold 4000x2000 pixels, When the user chooses Undo, Then the error dialog with the heading and body of REQ-0004-26 appears for "4000 × 2000", the picture is still 1x1, and Undo is still sensitive and Redo still insensitive; When memory is available again and the user chooses Undo, Then the picture is 4000x2000 with every pixel exactly as before; the same holds for Redo of a resize that grows the picture | Verified by: automated (core, refusal) + automated (ui, dialog)
- AS-31 (REQ-0004-12) Given the start-up picture (800 × 600) with Pan active, When the user switches the grid on and presses Zoom In (+) up to 1600 %, Then the grid lines are over the whole visible part of the picture at once (corners, middles and center of the view), also after Zoom Out and Zoom In again, and after the view is panned by more than a page right, down, left and up; the outline of a selection and the guide line of a waiting gradient are likewise over the whole visible part of their length | Verified by: automated (ui, rendered without asking the canvas to redraw: `as10_grid_covers_the_view_after_zoom_and_pan`, `as10_selection_outline_covers_the_view_after_pan`, `as10_gradient_guide_covers_the_view_after_pan`)

## Edge cases
- Text the user types: the Grid and Attributes fields hold empty, 0, negative, above the limit, leading zeros ("007" is 7), spaces around the number (trimmed), and anything non-numeric; the same rules for the settings keys of REQ-0004-15 (REQ-0001-40).
- A rule that changes the user's input after they confirmed it: does not apply (the fields are only valid or not).
- The same action again while its dialog is already open: Grid… and Attributes… cannot both be open, and neither opens twice (REQ-0004-03).
- Key repeat on Ctrl+= or Ctrl+- steps one level per repeat; mashing Ctrl+G toggles each time; double-clicking a handle is two clicks without a drag, so no step (REQ-0004-24).
- Zoom commands during a stroke are ignored until release (REQ-0004-03).
- Resizing to 1 x 1, and to 10000 x 10000 (400 MB for the picture, plus the undo step's pixels when shrinking).
- A resize while zoomed so the new picture is smaller than the canvas area: it sits at the top-left (REQ-0001-11).
- Fit to Window on a picture so large that even 10 % needs scrollbars: 10 %, scrollable (REQ-0004-07).
- The grid with a cell size larger than the picture shows no lines; the grid with the picture transparent is drawn over the checkerboard.
- Pan with the grid on draws it; Pan shows no handles.

## Out of scope
Pan's right-click menu with zoom (Window Extras); a thumbnail window and its View menu item (Window Extras); Frame Time (a Preferences setting, shown by Window Extras); custom zoom values and a Custom Zoom dialog; keyboard navigation of the
handles (Accessibility); Stretch, Skew and the other Image menu items (Image Transforms); units other than pixels; black-and-white pictures
(Image Transforms); remembering the grid or the panels between runs (Preferences); scrolling beyond what 0001 gives (wheel, Shift+wheel,
scrollbars, Pan drag); the selection size in the status bar (Selection).

## Open questions
(none)

## Behavior reference
- `docs/reference/new-behaviors.md`: zoom 10 to 800 %; Fit to Window; Ctrl+PageUp / Ctrl+PageDown normal and large size; Grid dialog with cell size and an axonometric grid; the pixel grid from far enough in (XP: 4x); Ctrl+G.
- JOD's reference View menu (screenshot, 2026-10-15): a zoom row with Zoom Out, the zoom as `100%` and Zoom In; Normal Size, Fit to Window, Custom Zoom…; Grid…, Show Thumbnail; check items Tool Box, Color Box, Status Bar; Frame Time. This spec follows its layout and the zoom row, the panel items and the Image-before-View order, but keeps XP's shortcuts and Large Size, the Show Grid check item, no Custom Zoom, and leaves Show Thumbnail (Window Extras) and Frame Time (Preferences) out.
- `docs/design.md`: header bar Image and View menu buttons; status bar with pointer position and a hint per tool; Pan never changes the picture.
- XP Paint: View > Zoom (Normal Size Ctrl+PageUp, Large Size Ctrl+PageDown), Show Grid Ctrl+G from 4x; three resize handles just outside the right, bottom and bottom-right of the canvas; Image > Attributes (Ctrl+E) with width and height; growing fills with the background color; the status bar shows the pointer position and the picture size.
- 0001: REQ-0001-04 and REQ-0002-19 (a created settings file also holds `[view]`, REQ-0004-15), REQ-0001-16 and REQ-0001-39 (zoom commands and the other new actions follow the same rules, REQ-0004-03/04), REQ-0002-08 (scroll position kept as far as the size allows, REQ-0004-25), REQ-0002-16 (a resize step is sized by REQ-0004-23).

## Plan

### Approach
- **Core (libc only, headless):**
  - `view` gains the Fit to Window level, the pinch accumulator and the drag-position-to-size rule.
  - `grid` is new: the visibility rule and the line geometry (vertical and diagonal lines as plain segments).
  - `number` is new: the whole-number field parser used by both dialogs and the settings keys.
  - `image` gains `easel_image_resize`; `history` gains a resize step.
  - `settings` gains the `[view]` keys.
- **ui:** the header-bar Image and View buttons (View's popover holds the zoom row as a custom child and a menu for the
  rest), the actions and accelerators, the status bar widget, the Grid and Attributes dialogs (libadwaita), the grid,
  the handles and the drag outline (drawn by the canvas), and the three panel toggles.
- **io:** unchanged; the created file's default text, which gains the `[view]` section, lives in core.

### A size change and the undo history (finding 5 of the 0002 check)
A stroke step stores pixel indexes, `y * width + x`, for the width the picture had when it was drawn. That stays correct
because history is one line and a step is only ever applied or reverted while the picture has the size it had when the
step was recorded: undoing step k happens in the state right after k, and redoing it in the state right before. A
resize step is the only thing that changes the size between its neighbours. So stroke steps need no change and no size
is stored in them.
- **A resize step stores:** the old and new size, the fill color, and the removed pixels (4 bytes each, in the row-major
  order of the old picture, skipping the kept area). No indexes: restoring walks the same order. Growing stores no
  pixels, since undo just cuts the added area, which is still pure fill because every later step has been undone first.
- **Budget:** 4 bytes per removed pixel plus 64 (REQ-0004-23); the drop-oldest loop and the lone oversized step of
  REQ-0002-17 are unchanged.
- **Atomic:** the removed-pixels array and the new pixel buffer are allocated before anything changes. If either fails,
  nothing changed and the window shows the REQ-0004-26 dialog. Undo and redo allocate the restored buffer first too;
  on failure they return an error, `applied` does not move, and the window shows the same dialog (AS-30).
- **Copies:** a resize allocates one new buffer of the new size and copies the kept area once; the history never copies
  the whole picture. A stroke step still stores only changed pixels.
- **Saved state and ids** work as before: a resize step is a step with an id.

### What grows with use
- The history: a stroke step stores 12 bytes per changed pixel; a resize step 4 bytes per removed pixel; both plus 64.
  Nothing is copied at stroke start.
- The grid, the handles and the drag outline store nothing and are drawn each frame, only for the visible area.
- The status bar stores its four current values and a write counter, nothing else.

### The status bar updates only when a value changes
Each field keeps its last value; a setter compares the new value with it and writes the label only if they differ, so the
pointer moving inside one picture pixel (many pointer events at 800 %), or an unchanged zoom or size, writes nothing. A
per-field counter of label writes is exposed for tests (the AS-8 test moves the pointer inside one pixel 50 times and
sees the counter grow by 0).

### New settings keys (REQ-0004-15, REQ-0004-27)
Section `[view]`, syntax as REQ-0001-40 (comments, spaces, case-insensitive values, last repeat wins, an unparseable file
gives every default). An invalid or out-of-range value is not clamped: it means the default, silently, as `[history]
memory` does.

| Key | Valid values | Default | Anything else |
|---|---|---|---|
| `grid` | `on`, `off` | `off` | `off` |
| `grid-size` | whole number 1 to 1000 (leading zeros and spaces fine) | `1` | `1` (0, 1001, negative, empty, text) |
| `grid-axonometric` | `on`, `off` | `off` | `off` |
| `toolbox` | `on`, `off` | `on` | `on` |
| `colorbox` | `on`, `off` | `on` | `on` |
| `statusbar` | `on`, `off` | `on` | `on` |

A settings file the app creates holds the `[view]` section with these defaults, after `[history]`; an existing file is
never rewritten. `EaselSettings` gains six fields (the default text of a created file lives in `settings.c`, so io needs no change); the window reads them once at start-up.

### Drawing only the changed area at 8000x8000
The measurements of 0001 and 0002 showed the lag was the screen copy: each update turned the whole picture into a new
`GBytes` (a 256 MB copy at 8000x8000, about 51 ms). What was tried, and what was built:
1. **First try, one zero-copy texture plus the changed region.** The copy disappeared (the action itself fell from 51 ms to
   0.03 ms), but the time to the screen stayed at 56 ms: the Vulkan renderer uploads the whole 256 MB texture each frame and
   does not narrow the upload to the changed region. The Plan's assumption was wrong; the benchmark showed it in one round.
2. **Built: tiles.** The picture is shown as tiles of 500 x 500 picture pixels (1 MB each), each a zero-copy view of the
   picture's own memory (the picture's stride, no copy); an update rebuilds only the tiles that hold a changed pixel, so
   GTK uploads about one tile. 500, not 512: 500 times any zoom level (10, 25, 50, ... 800 %) is a whole number of device
   pixels, so tile edges never fall between device pixels and leave no seams. The memory stays valid because the picture's
   buffer is replaced only by a resize, which points the canvas at the new buffer in the same call, and the canvas drops its
   tiles before the picture is freed.

### Measurements (T002, 2026-10-15)
Benchmark `perf-0004` (`meson test -C build --benchmark --verbose perf-0004`), the default renderer (GskVulkanRenderer),
AMD Ryzen AI 7 PRO 350 with Radeon 860M, 60 Hz display (a refresh is 16.7 ms), window visible. Medians, two runs. "Before"
is the 0001 and 0002 measurements of the same actions.

| Action | Size | Before | After the tiles |
|---|---|---|---|
| Pencil motion, on screen | 1920x1080 | 16.7 ms | 16.7 ms |
| Pencil motion, on screen | 8000x8000 | 92 ms (51 ms of it the copy) | **16.6 ms** (worst 54 to 59 ms, once per run) |
| Undo of 50,000 pixels, on screen | 1920x1080 | 16.7 ms | 16.4 ms |
| Undo of 50,000 pixels, on screen | 8000x8000 | 94 ms | **16.6 ms** |
| Redo of 50,000 pixels, on screen | 8000x8000 | 96 ms | **16.9 ms** |
| Bytes of the picture copied by the canvas | any | 256 MB per update at 8000x8000 | 0 |

The three budgets of `docs/engineering.md` that this touches are met at both sizes. The worst case of about 55 ms per run
is probably the first frame, which uploads the visible tiles; manual check M8 shows whether it is noticeable.

### Measurements (T015, 2026-10-15)
Same benchmark, machine and renderer as above (60 Hz, a refresh is 16.7 ms), one run, medians; "action" is the time of the
call itself, "on screen" runs to the next painted frame.

| Action | Size | Action | On screen |
|---|---|---|---|
| Frame with the grid, cell 1 at 800 % | 1920x1080 / 4000x4000 / 8000x8000 | 0.01 ms | 16.9 / 16.7 / 16.6 ms |
| Frame with the axonometric grid, cell 4 at 800 % | 1920x1080 / 4000x4000 / 8000x8000 | 0.00 ms | 16.8 / 16.8 / 16.6 ms |
| Resize removing about 50,000 pixels | 1920x1080 | 1.4 ms | 16.8 ms |
| Undo of that resize | 1920x1080 | 1.6 ms | 16.7 ms |
| Redo of that resize | 1920x1080 | 0.9 ms | 16.6 ms |
| Resize removing about 50,000 pixels | 4000x4000 | 22 ms | 24 ms |
| Undo / redo of it | 4000x4000 | 21 / 16 ms | 24 / 19 ms |
| Resize removing about 50,000 pixels | 8000x8000 | 75 ms | 77 ms |
| Undo / redo of it | 8000x8000 | 73 / 58 ms | 76 / 61 ms |

The grid costs nothing measurable: it draws only the visible area, and a frame with it is a normal frame at every size.
A resize allocates and fills a whole new picture (256 MB at 8000x8000), so its time grows with the picture, not with the
removed area: one frame at 1920x1080, a few frames at 4000x4000, about five at 8000x8000. The engineering budgets (pencil
motion, undo and redo of a stroke) do not cover a resize, which happens once in a while and not per pointer event; I
leave a faster resize (growing in place, or resizing rows only) unbuilt, per rule P5, unless JOD finds it slow in M8.

### Files
- **New:** `src/core/grid.{c,h}`, `src/core/number.{c,h}`, `src/ui/status-bar.{c,h}`, `src/ui/grid-dialog.{c,h}`,
  `src/ui/attributes-dialog.{c,h}`, `tests/core/test-grid.c`, `tests/core/test-number.c`, `tests/core/test-resize.c`,
  `tests/perf/perf-0004.c`.
- **Changed:** `src/core/view.{c,h}`, `src/core/image.{c,h}`, `src/core/history.{c,h}`, `src/core/settings.{c,h}`,
  `src/ui/canvas.{c,h}`, `src/ui/window.{c,h}`,
  `src/ui/window.ui`, `src/ui/application.c`, `src/ui/palette-bar.{c,h}` (hide and show), both `meson.build` files,
  `tests/core/test-view.c`, `tests/core/test-settings.c`, `tests/core/test-history.c`, `tests/io/test-settings-file.c`,
  `tests/ui/test-window.c`.

### Key interfaces (core)
- `int easel_view_fit_zoom (int area_w, int area_h, int pic_w, int pic_h)` (the largest whole percent that fits, 10 to 800);
  `int easel_view_pinch (double *scale_acc, double scale_delta)` (levels to move, -1, 0 or +1, resetting the
  accumulator); `void easel_view_resize_by (int handle, double dx, double dy, int *w, int *h)` (w and h are the size at the press and
  become the new size: the edge moves by dx, dy picture pixels, rounded to the nearest boundary, limits 1 to 10000;
  handles: right, bottom, corner).
- `bool easel_grid_visible (int cell, int zoom_pct)` (cell * zoom / 100 >= 4);
  `void easel_grid_lines (int w, int h, int cell, bool axonometric, double cx0, double cy0, double cx1, double cy1,
  EaselGridLineFunc fn, void *data)` (segments in
  picture coordinates, clipped to the picture).
- `bool easel_parse_field (const char *text, int min, int max, int *out)`.
- `EaselResizeResult easel_image_resize (EaselImage *img, int w, int h, uint32_t fill)`; and the edit as one call that
  allocates first, resizes and records the step: `EaselResizeResult easel_resize_edit (EaselImage *img, EaselHistory *h,
  int w, int h, uint32_t fill)` (OK, UNCHANGED, NO_MEMORY, BAD_SIZE).
- `easel_history_undo` and `easel_history_redo` handle the resize step; after either, the caller checks the picture's
  size and refreshes the canvas.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_menus` + manual M1 |
| 2 | ui `as2_actions_ignored` |
| 3 | core `as3_zoom_levels`, ui `as3_zoom_buttons` |
| 4 | ui `as4_normal_and_large` |
| 5 | core `as5_zoom_anchor_center`, ui `as5_zoom_keeps_center` |
| 6 | core `as6_fit_zoom`, ui `as6_fit_to_window` |
| 7 | core `as7_pinch_accumulator` + manual M2 (the real gesture) |
| 8 | ui `as8_status_fields`, ui `as8_status_writes_only_on_change` + manual M3 |
| 9 | manual M3 |
| 10 | core `as10_grid_visibility`, ui `as10_grid_toggle`, ui `as10_grid_pixels` (the drawn lines, in a rendered window), ui `as10_grid_at_magnifier_levels` (the lines at a scrolled view, at 4x, 6x and 8x, their places computed from the view's scroll offset and the cell size) + manual M4 |
| 11 | core `as11_grid_cell_16` |
| 12 | ui `as12_grid_dialog` |
| 13 | core `as13_axonometric_lines` + manual M4 |
| 14 | core `as14_view_settings`, io `as14_created_file_has_view`, io `as14_existing_file_untouched`, ui `as14_grid_starts_as_settings_say` |
| 15 | ui `as15_handles` + manual M5 |
| 16 | manual M5 |
| 17 | core `as17_size_from_movement`, ui `as17_handle_drag` |
| 18 | ui `as18_drag_cancel_rules` + manual M5 |
| 19 | core `as19_grow_and_shrink` |
| 20 | ui `as20_attributes_dialog` + manual M6 |
| 21 | core `as21_undo_redo_resize_exact` |
| 22 | core `as22_steps_through_a_resize`, ui `as22_undo_through_resize` |
| 23 | ui `as23_modified_across_resize` |
| 24 | core `as24_resize_step_budget` |
| 25 | ui `as25_same_size_no_step` |
| 26 | ui `as26_scroll_clamp_after_resize` + manual M5 |
| 27 | core `as27_resize_refused` (failure path, automated), ui `as27_error_dialog`, ui `as27_error_through_attributes` |
| 30 | core `as30_undo_redo_refused` (failure path, automated), ui `as30_undo_error_dialog` |
| 28 | ui `as28_panel_toggles` + manual M7 |
| 29 | core `as29_oversized_resize_kept_alone` |

Tasks without a scenario: T001 ui `texture_is_not_copied`; T002 benchmark `perf-0004` and manual M8; T015 `perf-0004` extended. Found during T010: ui `as13_offset_is_displayed` (0001 REQ-0001-16: after a zoom the picture is shown at the scroll position, not only the adjustment values).

### Constitution check
- P4: grid lines are hard-edged; picture pixels stay hard-edged at every zoom.
- P5: the screen-copy change is justified by the measurements of 0001 and 0002 and is itself measured.
- P6: the grid and panel start-up states are settings.
- Layering: the grid geometry, number parsing, fit and resize are in core with no GLib.
- One user-visible edit is one undo step: a resize is one step; grid, zoom and panels are not edits.

### Risks
- **Zero-copy texture memory (the riskiest part).** GTK treats a texture as immutable, but the picture's pixels change
  under it after every stroke. Torn or stale drawing is possible, and the ui tests (forced Cairo renderer) cannot show
  it. So the speed work comes first (group A): the manual check M8 draws pencil strokes, undoes and redoes, zooms and
  pans on an 8000x8000 picture with the real renderer, looking for flicker and old pixels. If it fails, the 512 x 512 tile
  fallback was needed for speed and is built (tiles of 500); any flicker, seam or old pixel M8 shows is fixed before any other UI is built on top. The picture's buffer is replaced only by a resize, which
  rebuilds the texture in the same call; a test (`as26`) resizes while a frame is pending.
- **Custom child in the View popover.** The zoom row is a widget inside GTK's popover menu; if it does not look right,
  the whole View menu becomes a custom popover. Manual check M1.
- **The pinch** cannot be synthesized in a test; the accumulator is tested in core and the real gesture by manual check M2.
- **Handle drag and scrolling.** The handles are in the canvas widget's own space (it reserves 5 + 2 pixels right and
  below the picture), so they scroll with it; the drag uses pointer positions relative to the picture.
- **Grid drawing cost.** Lines are generated only for the visible area; at 800 % with cell 1 that is about a hundred lines;
  the axonometric grid at a small cell is the worst case, so the benchmark includes it.
- **Memory of 10000 x 10000** (400 MB): tests of the refusal path use sizes over the limit, and the shrink tests use
  small pictures; the one real 10000 x 10000 case is a manual check.
- **The benchmark renderer.** The ui tests force the Cairo renderer; the benchmark uses the default one, as in 0001 and 0002.

### Task groups, with a look-check after each
The speed work comes first, so every later piece is built and checked on the faster drawing path.
A. Speed at 8000x8000 (T001, T002): the numbers recorded in this Plan, and manual M8 on the real renderer (flicker, stale pixels).
B. Menus and zoom (T003 to T006): M1 and M2.
C. Status bar and panels (T007, T008): M3 and M7.
D. The grid (T009 to T011): M4.
E. Resizing (T012 to T014): M6 (Attributes), then M5 (handles).
F. Measuring the grid and the resize (T015): numbers recorded in this Plan.

## Tasks
Group A, speed at 8000x8000 (budget: engineering.md, "Large picture" and "Pointer event to pixels")
- [x] T001 Draw only the changed area (no REQ: the budget of engineering.md and Plan "Drawing only the changed area") - done when the picture is shown as 500 x 500 tiles that are zero-copy views of its pixels and an update rebuilds only the tiles it touches, the 0001 and 0002 ui tests pass unchanged, and ui `texture_is_not_copied` shows the canvas copies 0 bytes of the picture, the texture follows the pixels after strokes, and the canvas can be pointed at another picture and back with the first freed meanwhile (the resize itself comes in T012)
- [x] T002 Benchmark perf-0004 and manual M8 (same budget) - done when `perf-0004` times a pencil stroke motion, an undo and a redo at 1920x1080 and 8000x8000 with the default renderer, its numbers are recorded in the Plan next to the 0001 and 0002 numbers, and manual M8 passes: on the real renderer, a pencil stroke, undo, redo, zoom and pan on an 8000x8000 picture show no flicker and no old pixels (they are drawn as tiles; if the check shows seams or old pixels, T001 is reworked before group B starts)
Group B, menus and zoom
- [x] T003 View settings keys and the number field parser (REQ-0004-13, REQ-0004-15, REQ-0004-21, REQ-0004-27; AS-14) - done when core `as14_view_settings` and the number-field tests pass, io `as14_created_file_has_view` and `as14_existing_file_untouched` pass, and the existing settings tests of 0001 to 0003 pass unchanged
- [x] T004 Core zoom rules: fit level, pinch accumulator, zoom levels and anchors (REQ-0004-04, REQ-0004-05, REQ-0004-06, REQ-0004-07, REQ-0004-08; AS-3, AS-5, AS-6, AS-7) - done when core `as3_zoom_levels`, `as5_zoom_anchor_center`, `as6_fit_level` and `as7_pinch_accumulator` pass
- [x] T005 Image and View menus, zoom row, zoom actions and shortcuts (REQ-0004-01, REQ-0004-02, REQ-0004-03, REQ-0004-04, REQ-0004-05, REQ-0004-06; AS-1, AS-2, AS-3, AS-4, AS-5) - done when ui `as1_menus`, `as2_actions_ignored`, `as3_zoom_buttons`, `as4_normal_and_large` and `as5_zoom_keeps_center` pass, and manual M1 passes
- [x] T006 Fit to Window and the touchpad pinch in the window (REQ-0004-07, REQ-0004-08; AS-6, AS-7) - done when ui `as6_fit_to_window` passes and manual M2 passes (the real pinch)
Group C, status bar and panels
- [x] T007 Status bar: position, size, zoom and hint, writing only on change (REQ-0004-09, REQ-0004-10, REQ-0004-11; AS-8, AS-9) - done when ui `as8_status_fields` and `as8_status_writes_only_on_change` pass and manual M3 passes
- [x] T008 Panel toggles: Tool Box, Color Box, Status Bar (REQ-0004-27, REQ-0004-15; AS-28) - done when ui `as28_panel_toggles` passes, panels start as the settings say, and manual M7 passes
Group D, the grid
- [x] T009 Core grid: visibility rule and line geometry (REQ-0004-12, REQ-0004-14; AS-10, AS-11, AS-13) - done when core `as10_grid_visibility`, `as11_grid_cell_16` and `as13_axonometric_lines` pass
- [x] T010 Grid drawing, Show Grid, Ctrl+G and the start-up state (REQ-0004-12, REQ-0004-15; AS-10) - done when ui `as10_grid_toggle` passes and the grid is not in the saved picture, and manual M4 passes
- [x] T011 Grid dialog (REQ-0004-13; AS-12) - done when ui `as12_grid_dialog` passes (invalid values disable Apply, Esc cancels, Apply never toggles the grid) and manual M4 passes with the dialog
Group E, resizing
- [x] T012 Core resize and the resize history step (REQ-0004-20, REQ-0004-22, REQ-0004-23, REQ-0004-24; AS-19, AS-21, AS-22, AS-24, AS-29, AS-30) - done when core `as19_grow_and_shrink`, `as21_undo_redo_resize_exact`, `as22_steps_through_a_resize`, `as24_resize_step_budget` and `as29_oversized_resize_kept_alone` pass, and a resize that cannot be allocated leaves picture and history unchanged (core `as27_resize_refused`) and so does an undo or redo that cannot allocate (core `as30_undo_redo_refused`)
- [x] T013 Attributes dialog and the resize in the window (REQ-0004-21, REQ-0004-22, REQ-0004-24, REQ-0004-25, REQ-0004-26; AS-20, AS-23, AS-25, AS-26, AS-27, AS-30) - done when ui `as20_attributes_dialog`, `as22_undo_through_resize`, `as23_modified_across_resize`, `as25_same_size_no_step`, `as26_scroll_clamp_after_resize` and `as27_error_dialog` and `as30_undo_error_dialog` pass, and manual M6 passes
- [x] T014 Resize handles: visibility, cursors, drag, cancel rules (REQ-0004-16, REQ-0004-17, REQ-0004-18, REQ-0004-19; AS-15, AS-16, AS-17, AS-18) - done when ui `as15_handles`, `as17_handle_drag` and `as18_drag_cancel_rules` pass and manual M5 passes
Group F, measuring
- [x] T015 Measure the grid and the resize (budgets: engineering.md "Pointer event to pixels"; REQ-0004-12, REQ-0004-14, REQ-0004-22) - done when `perf-0004` also times drawing with the grid on (cell 1 at 800 %, and an axonometric grid with cell 4) and a 50,000-pixel resize with its undo and redo at 1920x1080 and 8000x8000, and the numbers are recorded in the Plan

Group G, a fix after the screencast of 2026-10-18 (a tiny change: AS-10 amended, then the test, then the code)
- [x] T016 The grid at a scrolled view (REQ-0004-12; AS-10) - done when ui `as10_grid_at_magnifier_levels` (4x, 6x and 8x reached with the Magnifier, a white and an orange picture, 3000 × 2000 and 8000 × 6000, the view scrolled by more than a page, the line columns and rows computed from the view's scroll offset and the cell size) passes, `as10_grid_pixels` still passes and the mutant of REQ-0004-12 for the scroll offset is killed
  Result: the test failed first, at `assert_grid_lines_at_scroll: 'pixel_is (px, w, x, y + step / 2, line)' should be TRUE` (no line where the scroll puts it; 456 stray grey pixels against 20000 needed before the position check was added); `cairo_translate (d.cr, -vx0, -vy0)` removed from `append_grid` and it passes (431576, 301614 and 231898 line pixels at 4x, 6x and 8x on 3000 × 2000); mutant "the grid is drawn shifted by the scroll offset" killed by that assertion (12 s); a second mutant (drawing from the picture's top-left) survived and was dropped: the lines are the same, only more of them are drawn (a speed question, not a result)
  Result: manual (JOD, 2026-10-18): the 5840 × 5184 photo at 8x, scrolled to a far corner: grid lines visible and glued to the pixels while panning

Group H, a finding against this spec from JOD's screencast of 2026-10-21 (found by hand while building 0014; a tiny change: AS-31 added, then the tests, then the code)
- [x] T017 The grid over the whole view after a zoom and a pan (REQ-0004-12; AS-31) - done when ui `as10_grid_covers_the_view_after_zoom_and_pan`, `as10_selection_outline_covers_the_view_after_pan` and `as10_gradient_guide_covers_the_view_after_pan` pass
  Result: the grid test failed first, with JOD's steps (800 × 600, Pan, grid on, Zoom In to 800 %): `grid_over_the_whole_view (...) should be TRUE`, no grid near the view's top-left point and none within 6 seconds; the outline and guide tests passed at once (they are drawn whole, not limited to the view), so they stay as guards. Cause, found with a temporary probe of the rectangle `append_grid` used: it is the viewport's bounds seen from the canvas, found at each snapshot from where the viewport has placed the canvas, and the viewport places it one frame late (`replace_in_viewport`, a tick callback), so the one draw that followed a zoom saw the view a page away (rectangle (2130, 1868) with the scroll values already (3078, 2632)), and nothing drew the canvas again: a scroll moves the cached picture without a new snapshot of the canvas. The grid, the Line and shape preview, the preview tiles of a floating selection or gradient and the part of a floating selection outside the picture are limited to that rectangle; the selection's outline, the gradient's markers and guide and the handles are not. Fix: the canvas is drawn again when either scroll value changes and in the tick that has the viewport place it (`replace_in_viewport`); with it the grid covers the view about 1.2 s after the first Zoom In click (three zoom steps' layouts included). Why the tests missed it: every earlier scrolling test called `wait_for_next_frame`, which queues a redraw of the canvas and so hid it. Mutants of REQ-0004-12: the sideways redraw, the vertical redraw and the redraw after the placement removed, 3 of 3 killed by the grid test.

