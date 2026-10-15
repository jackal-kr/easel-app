# Design

Cross-cutting look, feel and input rules. Feature specs reference this file instead of repeating it.

## UI / UX
Cross-cutting look and feel. Feature specs reference this file instead of repeating it. Seed from GNOME HIG and XP Paint's
workflow; JOD to confirm or change each point; mark changes with an ADR if they reverse a decision.

- GNOME-native (GTK 4 + libadwaita, GNOME HIG). Follows system light/dark and accent color. No XP visuals.
- Header bar: Open, Save, Undo, Redo, Image menu button, View menu button, slim primary (hamburger) menu.
  No nested XP-style menu bar.
- Layout: toolbox (18 tools: the 16 XP tools plus Pan and Gradient) with per-tool options below it, canvas in a scrolled viewport, palette bar
  (fore/background indicator, swap button, 28 swatches), status bar with pointer position, selection size and tool hint.
- Dialogs are libadwaita dialogs; destructive actions (discard unsaved changes) are labelled with the action, not "OK".
- Preferences dialog: appearance, new-picture defaults, drawing defaults, palette persistence, undo memory budget.
- Shortcuts window lists every shortcut. Every action reachable by menu or toolbar also has a shortcut where XP had one.
- All user-visible strings are translatable and not built by concatenation.

### Settings
All user-adjustable defaults (new-picture size and background, drawing defaults, panel visibility, theme, undo budget, text font and style, palette) are settings kept in a configuration file. The Preferences dialog (feature 0013) edits the same settings; until then the file is edited by hand. Concrete default values are decided in the spec that needs them.

### Palette editing
Palette colors are edited in place: double-click a palette color or press F2 on it, which opens the system color chooser; clicking a square of the fore/background indicator opens it for that color. Changes are remembered between runs; Preferences can reset the palette to the classic 28 colors. There is no separate Edit Colors menu entry (settled in 0003).

## Interaction model
How input maps to actions across all tools. Seed from XP Paint behavior; verify while writing each spec.

### Modes
- Pan is a tool and the tool active at start-up. A left-button drag pans; Ctrl + mouse wheel zooms (only in Pan mode). Nothing in Pan mode changes the picture or the undo history.
- Ctrl+drag on a selection moves a copy of it (classic XP).
- After any paste the Select tool becomes active with the pasted picture selected.

### Pointer
- Primary button: foreground color. Secondary button: background color.
- Pressing the other button during a stroke cancels it (nothing is added to undo).
- Shift constrains (straight lines at 45-degree steps, squares and circles for shapes); with a selection,
  Shift-drag smears, Shift-click stamps.
- Right-click on a selection or text box opens a context menu instead of drawing.

### Pending state
Things not yet put down (floating selection, text being typed, unfinished curve/polygon) are "pending".
They enter undo history only when committed; save, close, New and tool change must commit or ask first.
One user-visible edit = exactly one undo step.

### Keyboard
- Numpad +/- changes brush size. Ctrl+G toggles the grid at 4x zoom and above. Esc cancels pending state.
- Canvas takes keyboard focus; the view must not jump when focus changes.
- Every pointer operation has a keyboard path (see accessibility feature).

### Feedback
- Cursor shows the tool; status bar shows a one-line hint per tool and the pointer coordinates.
- Hover previews for shape tools and brush outline where XP had them.
