# UI / UX

Cross-cutting look and feel. Feature specs reference this file instead of repeating it. Seed from GNOME HIG and XP Paint's
workflow; JOD to confirm or change each point; mark changes with an ADR if they reverse a decision.

- GNOME-native (GTK 4 + libadwaita, GNOME HIG). Follows system light/dark and accent color. No XP visuals.
- Header bar: Open, Save, Undo, Redo, Image menu button, View menu button, slim primary (hamburger) menu.
  No nested XP-style menu bar.
- Layout: toolbox (18 tools: the 16 XP tools plus Pan and Gradient) with per-tool options below it, canvas in a scrolled viewport, palette and
  fore/background swatches, status bar with pointer position, selection size and tool hint.
- Dialogs are libadwaita dialogs; destructive actions (discard unsaved changes) are labelled with the action, not "OK".
- Preferences dialog: appearance, new-picture defaults, drawing defaults, palette persistence, undo memory budget.
- Shortcuts window lists every shortcut. Every action reachable by menu or toolbar also has a shortcut where XP had one.
- All user-visible strings are translatable and not built by concatenation.

## Settings
All user-adjustable defaults (new-picture size and background, drawing defaults, panel visibility, theme, undo budget, text font and style, palette) are settings kept in a configuration file. The Preferences dialog (feature 0013) edits the same settings; until then the file is edited by hand. Concrete default values are decided in the spec that needs them.

## Palette editing
Palette colors are edited in place: double-click a palette color, press F2 on it, or click a swatch, which opens the system color chooser. Changes are remembered between runs; Preferences can reset the palette to the classic 28 colors. Whether a separate Edit Colors menu entry is also wanted is settled in spec 0003.
