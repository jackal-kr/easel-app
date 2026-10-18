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
  pencil strokes, line clipping, view math (zoom levels, fit zoom, pointer to pixel), the undo history, the palette,
  the saveable formats and the save-name rule (`format`), flattening on white and the one-pass conversion of decoded
  bytes with EXIF orientation (`image`), and the shared syntax of the hand-editable files (`ini`: sections, keys, colors), which settings and
  palette parsing build on.
- **`src/io`** (GLib, GIO, gdk-pixbuf; never needs a display) reads and creates the settings file, reads and writes the
  palette file, and opens and saves pictures (`picture-file`: PNG, JPEG, BMP, TIFF, WebP, and GIF for opening only).
  Opening starts with a header sniffer of our own, because gdk-pixbuf's `get_file_info` decodes the whole picture
  through glycin, and a memory pre-check, because glycin aborts the process when an allocation fails (0005 Plan).
  Every write is atomic (temporary file and rename), so a failure leaves the old file.
- **`src/ui`** (GTK 4 + libadwaita) has the application, the window (template `window.ui` in a GResource; it owns New,
  Open, Save and Save As, and the one "Save Changes?" prompt that Quit, New and Open share), the `EaselCanvas` widget
  (tiles, grid, resize handles, busy cursor), the palette bar (indicator, swap button, swatches), the status bar and
  the Grid and Attributes dialogs. It turns input into core calls and draws nothing itself.

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
- Preferences will edit the same file.

### Packaging layout
Defined by the packaging and release feature.

## Testing
- **Every scenario (AS-n) is verified** by an automated test or a numbered manual check named in the spec.
  Tests are named after scenarios (`as3_undo_restores_buffer`) so `/sdd-check` can trace them.
- **Layer 1, core (headless, libc only):** pixels, history, transforms, clipping. Fast, exact expectations, property checks
  (undo restores the original buffer, transform then inverse is identity).
- **Layer 2, widget behavior:** synthetic press/drag/release through the canvas input entry points; asserts pixels, undo/redo
  and modified state. Needs a display: skip with exit code 77 otherwise; run headless with `GSK_RENDERER=cairo`.
- **Layer 3, manual:** short numbered checklists at each checkpoint in the spec's Tasks section for look and feel. Say plainly what cannot be tested headless.
- **Test first** where feasible; a bug fix starts with a failing test.
- **Running the ui cases of one feature:** the ui cases are in one executable, registered as `/0001/...` to `/0005/...`, and
  each feature is its own meson test (`ui-0001` to `ui-0005`, suite `ui`, each running `test-window -p /NNNN`), so
  `meson test -C build --print-errorlogs ui-0005` runs one feature (about 15 s; 0004: 19 s, 0001: 7.5 s, 0003: 4.6 s,
  0002: 3 s; 97 cases in all, each in exactly one test). The core and io tests take about one second in all. Run the
  tests of the feature you touched after each task, the full `meson test -C build --print-errorlogs` (about 46 s) at each
  group boundary and at `/sdd-check`. The ui tests do not run in parallel (`is_parallel: false`): run in parallel, the
  ui windows failed `as5_zoom_keeps_center` with a stale layout in 2 of 3 runs. That is all that was shown; earlier
  one-off failures of the ui tests (run as a single executable) are unexplained and not fixed.
  Close every Easel window (and any other test or probe window of ours) before running the ui tests or `tools/mutate.sh`:
  with one open from a manual check, three ui cases failed in every run until it was closed (learning log, 2026-10-16).
  `test-window` refuses to start, and `tools/mutate.sh` refuses to run a mutant of a ui test, while another `easel` or
  `test-window` process exists (the core and io tests do not check).
- **A probe or throwaway window** is started with its pid saved and stopped with `kill <pid>`, never `pkill -f` (the pattern
  can match the shell that runs it, and the window stays open and spoils the next ui run).
- **Mutation tool (`tools/mutate.sh`):** puts one deliberate bug at a time into a scratch copy of the sources, builds, runs
  the tests that must catch it, and prints `killed` or `SURVIVED` (a hole in the tests) with the requirement ID and the
  time, then a total; the exit status is non-zero if any mutant survived or did not build. It never touches the working
  tree: it works in `.scratch/mutate` (git-ignored, deleted and made again at each start) with one build directory, so each
  mutant rebuilds one file.
  - `tools/mutate.sh` runs every mutant of `tests/mutants.txt` (100 mutants, 495 s, all killed; a mutant killed only by a crash counts as a test gap, so the cases for rules that lose work run in a subprocess or come first); `tools/mutate.sh
    REQ-0005-03` runs the mutants of one requirement (several IDs may be given); `MUTANTS=file tools/mutate.sh` uses
    another list. The ui tests need the display, as always.
  - To add a mutant, append a block to `tests/mutants.txt`: `=== REQ-NNNN-xx what the bug does`, then `file:` (from the
    project root), `test:` (the meson test names that must fail, tried one after the other), `--- find` with the exact text
    (it must occur once in the file; no blank line), `--- replace` with the text that takes its place. Write the mutant
    that breaks the rule, run it, and fix the tests until it is killed; a mutant that is equivalent (no input can see the
    change) does not belong in the list.
- **What to run when (the skills point here):** after a task, the tests of the feature touched (`meson test -C build
  --print-errorlogs ui-NNNN`, plus the core and io tests) and the mutants of the task's requirements (`tools/mutate.sh
  REQ-NNNN-xx`, and `tools/mutate.sh --lint`, seconds, which lists mutants whose find text no longer occurs, for example
  after the task edited a block a mutant quotes); at a group boundary, the full `meson test -C build --print-errorlogs`; at `/sdd-check`, everything: the
  full `meson test` and the whole mutation list.
- **Code that can abort the process** (a failed allocation in GLib, gdk-pixbuf or glycin) is tested in a subprocess
  (`g_test_trap_subprocess`, as `as19_save_without_memory` and `as8_open_without_memory` do), so that an abort is a failed
  assertion of the test and not a crash of the run.
- **Gates:** build warning-free, all tests green after each task; `tools/spec.sh lint` clean before a spec is accepted (`tools/spec.sh lint --names NNNN` additionally fails when a test named in that spec's scenario-to-test map is not registered in `tests/`; `/sdd-check` runs it, the plain lint does not, because a Plan names its tests before they are written).
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
| Fill, a whole 8000x8000 picture (core) | < 500 ms (JOD, 2026-10-18; measured 160 ms) |
| Fill or Magnifier press to the painted frame | < 100 ms at 1920x1080 and 8000x8000 (JOD, 2026-10-18; measured one frame) |

The Fill and Magnifier budgets are benchmark budgets: `perf-0007` prints the numbers, `/sdd-check` reports them against the
table, and no test fails on them (timing tests flake on a loaded machine). The worst-case fill (the comb) has no time budget;
its cost is memory, 12 bytes per one-pixel span.
