# Product

## Vision
A lightweight, native GNOME app for quick pictures and photo touch-ups with the simplicity of Windows XP Paint,
that could one day sit among the default GNOME apps.

## Inspired by
- **Microsoft Paint in Windows XP**: the original whose functionality and workflow this app follows (XP version specifically, not later Paint releases).
- **[JS Paint](https://github.com/1j01/jspaint) by Isaiah Odhner**: studied for how the XP behavior works. MIT licensed; no code is copied.

The About dialog credits both (see the window extras feature).

## Users
People who want to sketch, crop, annotate or fix a screenshot or photo in seconds, without learning a big editor.

## Scope (v1)
All 16 XP tools plus Pan and Gradient (see `docs/reference/new-behaviors.md`): free-form select, select, eraser, fill, pick color, magnifier, pencil, brush, airbrush, text,
line, curve, rectangle, polygon, ellipse, rounded rectangle. Tool options, 28-color palette edited in place (no separate Edit Colors entry),
fore/background colors, zoom + grid, selections (move, smear, stamp, transparent), clipboard,
text boxes, Image-menu transforms (flip, rotate, stretch, skew, invert, attributes, clear), unlimited-ish undo,
open/save png, jpeg, bmp, tiff, webp (gif open only), print, Preferences, shortcuts window, thumbnail, i18n, accessibility,
Flatpak / deb / AppImage packaging.
Additions beyond classic XP Paint: `docs/reference/new-behaviors.md`.

## Non-goals
XP visual styling; the third (Ctrl) color slot; layers; non-destructive editing; plugins; cloud features; Windows/macOS ports.

## Success criteria
- Every behavior written in a spec is implemented and verified, and the app feels like XP Paint in a native GNOME shell.
- Experiment measures (fill in `docs/learning-log.md`): spec gaps found per feature, rework after first implementation,
  defects at first run, time per feature.

