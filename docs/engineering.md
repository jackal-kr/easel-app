# Engineering

## Coding standards (C)
- GNU-ish style: 2-space indent, space before `(`, return type on its own line, `/* */` comments (avoid `//`).
  Match the surrounding file; do not reformat untouched code.
- Warnings are bugs: build with `-Wshadow -Wmissing-prototypes` and friends, zero warnings. Non-static functions have prototypes.
  Two warnings are switched off in `meson.build`, on purpose: `-Wno-unused-parameter` (GTK and GLib callbacks have fixed
  signatures whose parameters a handler often does not need) and `-Wno-missing-field-initializers` (designated
  initializers such as `GActionEntry` tables leave the remaining fields zero by design). Everything else stays on.
- Naming: public UI API `easel_<module>_`; internals shared inside one module share a short prefix.
- `src/core`: libc only. No GTK, GLib or Cairo includes. No global state; functions take the object they act on.
- `src/ui`: turns input into core calls; no drawing logic in widgets. GLib/GTK lifetime: `g_autoptr`, disconnect or tie signal
  handlers to the object, unparent popovers in dispose.
- Every pixel write goes through one dirty-marking path (defined in the plan of the feature that introduces it).
- Menus and actions: declared in data files, registered in one place; an absent action disables its item.
- Strings: one place per dialog, no sentence concatenation.
- Comments explain why, not what. No dead code, no commented-out code.

## Architecture
Written in the plan of feature 0001 and updated by every plan that changes it. The `docs/constitution.md` constraints
still apply: one source of truth for tool definitions, and no threads or caches without a measurement.

### Layers and modules
- **`src/core`** (libc only, no global state) holds the picture (`EaselImage`, straight-alpha `0xAARRGGBB`, ADR-0002),
  pencil strokes, line clipping, view math (zoom levels, pointer to pixel), settings parsing and the save-name rule.
- **`src/io`** (GLib, GIO, gdk-pixbuf; never needs a display) reads and creates the settings file and saves PNG files with
  an atomic replace.
- **`src/ui`** (GTK 4 + libadwaita) has the application, the window (template `window.ui` in a GResource) and the
  `EaselCanvas` widget. It turns input into core calls and draws nothing itself.

### Build layout
- Meson builds each layer as a static library. `ui` is linked whole into the `easel` executable and into the ui tests.
- Tests are in `tests/{core,io,ui}`. Core tests are plain C (exit 0 or 1), io tests use GTest, and ui tests use GTest and
  exit 77 without a display.

### Data flow for an edit
1. Pointer input reaches the canvas.
2. The canvas calls `easel_stroke_*`.
3. The core writes pixels with `easel_image_set`, the one dirty-marking path, which grows the image's dirty rectangle.
4. The canvas re-uploads only that rectangle with `GdkMemoryTextureBuilder` and queues a redraw. The picture is drawn with
   nearest-neighbour filtering.

### Undo model
From feature 0002 (`src/core/history`):
- One step per user-visible edit. A step stores only the pixels the edit changed, as `{index, old, new}` (12 bytes each),
  plus a fixed 64 bytes per step for the budget. There are no bounding boxes and no picture copies.
- A stroke logs each changed pixel once, while drawing; at the end its log becomes the step (cancel uses the same log).
- Undo and redo write the step's values back through `easel_image_set`, so the screen update sees them like any edit.
- States are identified by step ids; 0 is the start. The saved state is an id, and it is lost when its step is discarded
  or dropped, or when any step is dropped while it is the start.
- A memory budget (`[history] memory`, MB) drops the oldest steps; a lone step over the budget is kept.
- Indexes are `y * width + x`: a feature that changes the picture's size must clear the history or record the size.

### Settings
- The file is `~/.config/easel/settings.ini`.
- The core parses its text and resolves defaults; io finds, reads and creates the file.
- Preferences (0013) will edit the same file.

### Packaging layout
Defined by feature 0017.

## Testing
- **Every scenario (AS-n) is verified** by an automated test or a numbered manual check named in the spec.
  Tests are named after scenarios (`as3_undo_restores_buffer`) so `/sdd-check` can trace them.
- **Layer 1, core (headless, libc only):** pixels, history, transforms, clipping. Fast, exact expectations, property checks
  (undo restores the original buffer, transform then inverse is identity).
- **Layer 2, widget behavior:** synthetic press/drag/release through the canvas input entry points; asserts pixels, undo/redo
  and modified state. Needs a display: skip with exit code 77 otherwise; run headless with `GSK_RENDERER=cairo`.
- **Layer 3, manual:** short numbered checklists at each checkpoint in the spec's Tasks section for look and feel. Say plainly what cannot be tested headless.
- **Test first** where feasible; a bug fix starts with a failing test.
- **Gates:** build warning-free, all tests green after each task; `tools/spec.sh lint` clean before a spec is accepted.
- **Later:** CI in a container or the Flatpak SDK (libadwaita >= 1.8 needed); screenshot checks only if they prove worth the upkeep.

## Performance budgets
Accepted by JOD (2026-10-15) as the starting budgets. Measure in features 0001 and 0004; change a number by ADR.
Rule P5 applies: no optimization without a measurement against a budget here.

| Area | Budget |
|---|---|
| Start to usable window | < 0.5 s on the dev machine |
| Pointer event to pixels on screen (pencil, brush) | within one frame (16 ms) on a 1920x1080 picture |
| Large picture | 8000x8000 opens and draws without freezing the UI |
| Undo memory | bounded by a user budget (default 512 MB, set in Preferences), never by step count |
| Undo / redo | shown at the next screen refresh on a 1920x1080 picture |
| Idle | 0% CPU when nothing is happening |
