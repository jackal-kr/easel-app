# Feature map

Order = dependency order. Create a folder with `tools/new-feature.sh NNNN slug`. Status via `tools/spec-status.sh`.
Vertical slices first (a thin end-to-end feature), breadth later.

| No | Slug | One line | Notes |
|---|---|---|---|
| 0001 | walking-skeleton | Start in Pan mode on a blank canvas (size and background from the configuration file), pan and zoom it, draw with the pencil, save as PNG, quit with an unsaved-changes prompt | **Pilot.** Includes Pan and the settings file. Write together, slowly |
| 0002 | undo-redo | Undo/redo with memory budget and "saved" state | |
| 0003 | colors-palette | Palette, fore/background colors, Edit Colors | |
| 0004 | canvas-view | Zoom range, Fit to Window, pixel grid and Grid dialog, scrolling, resize handles, Attributes dialog | Builds on Pan from 0001 |
| 0005 | file-io | Open/save formats, Save As, modified tracking, errors | gif open-only |
| 0006 | freehand-tools | Brush shapes/sizes, airbrush, eraser | |
| 0007 | fill-pick-magnify | Fill, pick color, magnifier | |
| 0008 | lines-curves-shapes | Line, curve, polygon, rectangle, rounded rectangle, ellipse, fill styles, gradient | |
| 0009 | selection | Rect and free-form select, floating selection, move, smear, stamp, transparent mode | |
| 0010 | clipboard | Cut, copy, paste, oversize paste, paste from / copy to file | |
| 0011 | text | Text box, fonts, styles, per-selection fonts | |
| 0012 | image-transforms | Flip, rotate, stretch, skew, invert, clear, black and white | |
| 0013 | preferences | Appearance, new-picture and drawing defaults, palette persistence, undo budget | Dialog edits the same configuration file the app already reads |
| 0014 | window-extras | Shortcuts window, thumbnail, frame time, print, context menus, About with acknowledgements (XP Paint, JS Paint) | |
| 0015 | accessibility | Names/roles, keyboard-only drawing and palette | |
| 0016 | i18n | UI languages (pl, uk, ru, de, cs, it, es, pt + English) | |
| 0017 | packaging-release | Flatpak, deb, AppImage, metadata | |
