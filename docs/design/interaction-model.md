# Interaction model

How input maps to actions across all tools. Seed from XP Paint behavior; verify while writing each spec.

## Modes
- Pan is a tool and the tool active at start-up. A left-button drag pans; Ctrl + mouse wheel zooms (only in Pan mode). Nothing in Pan mode changes the picture or the undo history.
- Ctrl+drag on a selection moves a copy of it (classic XP).
- After any paste the Select tool becomes active with the pasted picture selected.

## Pointer
- Primary button: foreground color. Secondary button: background color.
- Pressing the other button during a stroke cancels it (nothing is added to undo).
- Shift constrains (straight lines at 45-degree steps, squares and circles for shapes); with a selection,
  Shift-drag smears, Shift-click stamps.
- Right-click on a selection or text box opens a context menu instead of drawing.

## Pending state
Things not yet put down (floating selection, text being typed, unfinished curve/polygon) are "pending".
They enter undo history only when committed; save, close, New and tool change must commit or ask first.
One user-visible edit = exactly one undo step.

## Keyboard
- Numpad +/- changes brush size. Ctrl+G toggles the grid at 4x zoom and above. Esc cancels pending state.
- Canvas takes keyboard focus; the view must not jump when focus changes.
- Every pointer operation has a keyboard path (see accessibility feature).

## Feedback
- Cursor shows the tool; status bar shows a one-line hint per tool and the pointer coordinates.
- Hover previews for shape tools and brush outline where XP had them.
