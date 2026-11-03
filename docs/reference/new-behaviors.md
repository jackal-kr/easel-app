# New behaviors to introduce

Behaviors beyond classic XP Paint that the app should have. Each becomes requirements in the spec of the feature it
belongs to (see `docs/specs/INDEX.md`). Behavior only.

## Navigation and view
- Pan mode: a tool in the toolbox and the default mode when the app starts, so a picture can be opened, panned and zoomed to understand it before editing. It is a safe view mode: a drag never changes the picture. While it is active, dragging with the left mouse button pans the canvas (no modifier needed), without modifying the image or affecting undo history. With Ctrl held, the mouse wheel zooms the picture in and out (this works only in Pan mode). Right-click opens a menu (Paste, Paste From, Select All, zoom, Clear Image). No other tool pans.
- Zoom from 10 % to 1600 % (1600 %, beyond XP's 800 %: JOD, 2026-10-27; Fit to Window stops at 800 %); Ctrl + mouse wheel zooms, only in Pan mode; Fit to Window; Ctrl+PageUp / Ctrl+PageDown switch between large and normal size.
- Grid dialog: user-set cell size and an optional axonometric grid; the pixel grid appears when zoomed in far enough (the threshold is set in the spec; XP shows it from 4x); Ctrl+G toggles it.

## Tools
- Gradient tool: fades from the foreground color (right button: background) to transparent, so a second gradient lays over the first instead of replacing it. Drag sets direction and length, Enter applies, Esc cancels.
- Eraser has an Opaque / Transparent option: transparent mode erases to transparency instead of the background color.
- Transparency is real: PNG, TIFF and WebP keep it; JPEG and BMP are saved flattened on a white background.

## Editing
- Undo and redo have no fixed step limit; they are bounded only by a memory budget the user sets. Redo is Ctrl+Y or Ctrl+Shift+Z.
- Pasting a picture larger than the canvas asks: Resize Canvas to Picture (the canvas grows to the picture's size where the picture is bigger, never shrinking: 0016), Fit Picture to Canvas (the picture is resized to fit, keeping its aspect ratio), or Cancel. Paste also accepts copied picture files from the clipboard. After any paste the app switches to the Select tool with the pasted picture selected (for an oversize picture: already resized to fit the canvas with its aspect ratio kept, or with the canvas already resized to the picture's size, according to the choice made in the popup), so it can be moved before it is put down. A picture that is not larger than the canvas is inserted with its top-left corner at the top-left picture pixel in view (the canvas's top-left corner when it is in view: 0016), as a selection that can be dragged to the right place and resized, like any selection.
- Stretch accepts percent or exact pixels, with a Keep aspect ratio option; Skew accepts degrees or exact pixels (JOD, 2026-11-02).
- Right-click menus: on a selection (selection menu), on a text box (Cut, Copy, Paste, Paste as Plain Text, Select All), and with a select tool outside any selection (Paste menu and canvas actions).
- Ctrl+Shift+X swaps the foreground and background colors.

## Text
- Font controls: a card over the canvas, next to the text box, holds the font family, size and Bold / Italic / Underline / Strikethrough in one row; there is no separate dialog, and the text color is the foreground color picked on the palette: a palette color colors the selected text, or the text typed next; to color all of a box's text, select all (Ctrl+A) first.
- Formatting can differ within one text box: select part of the text and change font, size, color and styles for just that part.
- Copying or cutting text from a box keeps its formatting: the clipboard gets rich text as well as plain text, and pasting rich text into a box keeps each part's font, size, color and styles; plain text takes the style of the box at the caret.
- Default text size is 12. Esc closes the Font and Thumbnail windows.

## Colors
- Changing a palette color (double-click or F2, then a color chooser) is remembered between runs; Preferences can reset to the classic 28 colors. Clicking the fore/background indicator opens the chooser for that color.
- Keyboard access to the palette: Tab to it, arrows to move, Enter or Space picks the foreground, Shift+Enter the background.

## Application
- Preferences (Ctrl+,): appearance (light / dark / system, workspace color, the strength of the selection's tint, which panels are shown, frame time), new-picture default size and background, drawing defaults (the tools' presets, line width, fill style, magnifier zoom, undo memory, JPEG quality, text font and style), palette reset.
- Frame Time display, switchable from View and Preferences.
- Keyboard shortcuts window (Ctrl+? or Ctrl+F1); Help (F1) opens as a readable page in the default browser.
- Pictures can be opened from the command line.
- About with acknowledgements (XP Paint, JS Paint). Twelve UI languages, screen-reader labels and announcements, and keyboard-only drawing: covered by features Window Extras, Accessibility and i18n.

## Phase 2 (after the core scope)
Optional; it starts only after the core XP scope is delivered.
- Crop to selection: the picture is cropped to the current selection in one command.
- Zoom: a zoom slider in the status bar; Ctrl+wheel zooms with every tool, not only in Pan mode.
- Invert selection: selects everything outside the current selection.
- Smooth text: a switch in the Text tool's options draws text with antialiasing; its default is set in Preferences and is off
  out of the box.
- Smooth shapes and brushes: one shared switch in the options of the Line, Curve, shape and Brush tools draws them with
  antialiasing; its default is set in Preferences and is off out of the box.
- Either switch changes only the current session; it does not rewrite the saved default. The Pencil, the Eraser and Fill stay
  exact and show no switch.
- Extra shapes: triangles, diamond, pentagon, hexagon, arrows, stars, callouts, heart, lightning bolt.

## Decisions
- Panning happens only in Pan mode; so Ctrl+drag on a selection keeps classic XP behavior: it moves a copy.
- Pan mode's right-click menu keeps Paste, Paste From, Select All, zoom and Clear Image; a paste from it switches to the Select tool.
- Default undo memory budget: 512 MB, adjustable in Preferences (min and max to be set in the spec).
