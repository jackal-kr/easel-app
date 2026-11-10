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
5. A texture is never changed after it is made: memory that is changed in place (the picture's tiles, the trail of a floating
   selection) gets a new `GdkTexture` after each change, and the old one is dropped; a renderer may keep what it uploaded of a
   texture or read its memory again, and only a new texture is sure to be uploaded anew. (The ui tests use the Cairo renderer, which
   reads the memory live and so cannot see a texture that was not made anew: a counter of the textures made checks it, 0014 T023.)

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
- The file is `~/.config/easel/settings.ini`; under Flatpak, `~/.var/app/io.github.jackal_kr.Easel/config/easel/settings.ini`.
- The core parses its text and resolves defaults; io finds, reads and creates the file.
- Preferences will edit the same file.

### Packaging layout
- `data/` holds the desktop entry and the software-center metadata (`.in`, translated through `po/` at the build), the icons and the screenshot.
- `build-aux/flatpak/` holds the Flatpak manifest, on the GNOME 51 runtime, its locales not split off. `debian/` holds the deb's packaging.
- `tools/package.sh flatpak|deb|all|install-flatpak|install-deb` builds and installs them; the files go in `dist/`, which git ignores. VS Code has the same as tasks.
- Under Flatpak a file can be written only by the name the save dialog returned (REQ-0024-10). Test a format inside an installed Flatpak, not under `flatpak-builder --run`, where glycin's sandbox helper is not reachable and every load fails.

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
  `meson test -C build --print-errorlogs ui-0005` runs one feature (2026-11-02: 0014: 59 s, 0025: 40 s, 0016: 34 s, 0004: 31 s, most others 10 to 25 s;
  371 cases in all, each in exactly one test). The core and io tests take about one second in all. Run the
  tests of the feature you touched after each task, the full `meson test -C build --print-errorlogs` (about 6.5 minutes) at each
  group boundary and at `/sdd-check`. The ui tests do not run in parallel (`is_parallel: false`): run in parallel, the
  ui windows failed `as5_zoom_keeps_center` with a stale layout in 2 of 3 runs. That is all that was shown; earlier
  one-off failures of the ui tests (run as a single executable) are unexplained and not fixed. A one-off crash is logged in
  `docs/learning-log.md` with its message and its reruns; at its second sighting it is hunted before the task ends (open:
  ui-0020's `object_already_finalized` of 2026-11-09).
  `meson test --test-args '-p /NNNN/case'` adds a second filter to meson's own `-p /NNNN`, so the case runs twice in one process (as17 passed the first time and failed the second at `gtk_window_destroy`, 2026-10-27); to run one case alone, run `test-window -p /NNNN/case` with the ui environment of `tests/meson.build` (`GDK_DEBUG=no-portals`, `GTK_A11Y=none`, `GSK_RENDERER=cairo`, `EASEL_TEST_DATA`) inside `tools/isolated.sh`: without `GDK_DEBUG=no-portals` the file chooser is the portal's and the cases that wait for it fail.
  Close every Easel window (and any other test or probe window of ours) before running the ui tests or `tools/mutate.sh` in the real session (not needed through `tools/isolated.sh`):
  with one open from a manual check, three ui cases failed in every run until it was closed (learning log, 2026-10-16).
  `test-window` refuses to start, and `tools/mutate.sh` refuses to run a mutant of a ui test, while another `easel` or
  `test-window` process exists (the core and io tests do not check).
- A ui test does not turn the main loop while a synthetic button is held.
- A fully transparent pixel is written as 0x00000000 (P7.7, ADR-0008): code that makes a pixel's alpha 0 writes 0, not the color with
  alpha 0 (`easel_px_canon`), and a test of it compares with 0, not with the alpha alone. Why (0035, 2026-11-10): Remove Background kept
  the removed pixels' colors under alpha 0, and Fill, which compares all four channels, filled only patches of the removed area.
- Every numeric table or conversion in `src/core` (a lookup table, a fixed-point formula, a color space) has an oracle test: its values
  checked against an independent reference computed in the test (libm in doubles, a standard's own formula), over its whole range
  or a sweep of it, within a stated tolerance; determinism and behavior tests alone do not count. Why (0035, 2026-11-10): the integer
  cube root behind Remove Background's Lab was wrong for 64 944 of its 64 956 values, and every test passed, since they checked only
  that the same pixels came out twice and what a picture looked like; every delta-E threshold had been tuned on the distorted Lab.
- A dialog's or a card's OK, Apply, Convert or Cancel is tested by activating its real button (`click_response` in
  `tests/ui/test-window.c`: the button found by the response's label, "clicked" emitted), and Esc by the dialog's own close, then
  the main loop is run until the dialog has closed, and only then is the result read. A test never emits the response itself nor
  calls the function the response calls. Why (0035, 2026-11-10): libadwaita emits "closed" before "response" when a button is
  clicked; Remove Background decided in "closed", so every real OK was taken for Cancel, while its tests, which emitted the response
  first and closed the dialog after, all passed.
- **Where to run what.** The ui tests and `/sdd-check` run through `tools/isolated.sh`, which runs any command in a private session: a headless GNOME Shell with a virtual monitor and its own Wayland socket, a private D-Bus session, a throwaway `HOME` and runtime folder and `GSETTINGS_BACKEND=memory`, with `EASEL_ISOLATED=1` set (the "another Easel window" check of `test-window` is skipped then: nothing else can be in that display). The private environment is set before the D-Bus session starts, so that the services it starts (online accounts, calendars, portals) see no real account: `tools/isolated.sh tools/check-isolated-private.sh` checks that, printing names and counts only. Example: `tools/isolated.sh meson test -C build --print-errorlogs` (all the tests, about 6.5 minutes). `tools/mutate.sh` starts its own sessions, one per worker, and is run without this script. **Two runs a plain `meson test` does not prove:** `ui-0025-vulkan` runs in it under the Vulkan renderer, and `ui-0025-scale2`, `ui-0019-scale2`, `ui-0021-scale2`, `ui-0022-scale2` and `ui-0030-scale2` skip there and meson counts that skip as OK, so their scenarios are proved only by `tools/isolated.sh --scale 2 meson test -C build ui-0025-scale2 ui-0019-scale2 ui-0021-scale2 ui-0022-scale2 ui-0030-scale2` (a virtual monitor of 3840 x 2400 at scale 2), which `/sdd-check` runs as well. It stops everything it started, and removes the throwaway `HOME` and its sockets, however it ends, also when interrupted or when its parent is killed; it returns the command's exit status (130, 143 or 129 when interrupted), 90 if the shell did not start, 69 if `gnome-shell` or `dbus-run-session` is missing. Why: a real pointer motion with no button down reads as a lost release (REQ-0007-18) and ends the held test stroke, and the mouse and windows of the person at the keyboard changed the layout and the results. With a virtual mouse wiggling over the real screen (2026-10-19) `ui-0006` failed 6 of 6 at `as6_airbrush_sprays` in the real session and passed 5 of 5 in the isolated one, and the whole ui suite passed 11 of 11 there. Without it (a machine with no headless GNOME Shell) the ui tests and mutation runs are started with the pointer parked and the mouse left alone.
- **Benchmarks stay in the real session**, with no Easel window open: `perf-*` and `meson test --benchmark` are not run through `tools/isolated.sh`, which refuses them (and `perf-common.h` refuses when `EASEL_ISOLATED` is set). The isolated session has a virtual monitor with its own frame clock, and the ui tests there use the Cairo renderer, so its timings are not comparable with the real display's, which is what the budgets are about. (Its shell does use the GPU render node, not software rendering; only the frame clock and the renderer of the tests differ.)
- `render_pixels` waits up to 30 frames for the window to have painted one. A window that still gives no snapshot ends the run with the line `EASEL-INCONCLUSIVE: no snapshot` and exit status 75, never an ordinary assertion; `tools/mutate.sh` reads that as inconclusive: the mutant is run once more, a second one is counted apart and fails the run, and none is ever counted as killed. A test that meson reports skipped (exit 77, a session with no display) is read the same way, never as a survivor (0017: five mutants "survived" in 1 to 2 s each). Its own test hooks: `EASEL_TEST_NO_SNAPSHOT=N` (the first N snapshots of a process fail) and `EASEL_TEST_NO_SNAPSHOT_ONCE=FILE` (every snapshot of the first process fails).
- A ui test that fires several actions waits for a layout between them.
- A red check copies the file aside and restores it; git stash is never used for it.
- A ui test that checks a color sets its own theme and puts the default back at the end.
- **A probe or throwaway window** is started with its pid saved and stopped with `kill <pid>`, never `pkill -f` (the pattern
  can match the shell that runs it, and the window stays open and spoils the next ui run). A probe or helper that sets the
  desktop's clipboard acts from a click, since the compositor ignores a set from a client without recent input, and it is checked
  from outside (a second process, `xclip`) before JOD is asked to use it (0016 M2: the first helper set nothing, and Easel saw an old
  screenshot).
- **Mutation tool (`tools/mutate.sh`):** puts one deliberate bug at a time into a scratch copy of the sources, builds, runs
  the tests that must catch it, and prints `killed` or `SURVIVED` (a hole in the tests) with the requirement ID and the
  time, then a total; the exit status is non-zero if any mutant survived or did not build. It never touches the working
  tree: it works in `.scratch/mutate` (git-ignored, deleted and made again at each start). It starts 8 workers (`EASEL_MUTATE_WORKERS`), each with its own copy of the sources, build folder and session; they take mutants from one queue, a file's mutants together, and each builds only the tests the mutant names. `tools/check-mutate-workers.sh` and `tools/check-mutate-build.sh` test that.
  - **The case that last killed a mutant runs first.** `.scratch/mutate-last-kills.tsv` keeps it from run to run; with no such file the first run is slower, not different. Each of those cases first runs alone on the clean sources, once per run: one that fails there is named and not used. A kept case that passes on the mutant sends the mutant to its full tests, so only a kill is ever shortened. A core or io test a mutant names runs on the clean sources too: one that fails there would "kill" every mutant, so the run stops with status 2 and no verdicts. **A kill must repeat:** the same case, or the same core `CHECK`, fails twice; otherwise the mutant is `UNREPEATED`, counted apart in the summary line (", N not repeated") and failing the run. This replaces the older "a ui mutant killed in under 5 s is run again alone". `tools/check-mutate-kills.sh` tests all of this.
  - `tools/mutate.sh` runs every mutant of `tests/mutants.txt` (2026-11-02: 1129 mutants in 581 s; the same verdict for each as the serial run before these changes, which took 10732 s; a mutant killed only by a crash counts as a test gap, so the cases for rules that lose work run in a subprocess or come first; the tool reports such a mutant as `crashed`, "killed by crash (test gap)", when the failing test shows no assertion of ours and a FATAL or CRITICAL message, "Aborted" or a signal, counts it apart, keeps that mutant's test log as `.scratch/mutate/crash-N.log` and fails the run); `tools/mutate.sh
    REQ-0005-03` runs the mutants of one requirement (several IDs may be given); `MUTANTS=file tools/mutate.sh` uses
    another list. The ui tests need the display, as always. A mutant's text has no blank line and is memory-safe on its own (copy before free), so a crash is never the mutant's own fault. A mutant that crashes now and then is made memory-safe and run again, and only that run's verdict counts: a crash can hide a survivor. A background run is awaited by its process id or by the summary line `tools/mutate.sh` prints, and its counts are copied from that summary line, never from a search of the log.
  - To add a mutant, append a block to `tests/mutants.txt`: `=== REQ-NNNN-xx what the bug does`, then `file:` (from the
    project root), `test:` (the meson test names that must fail, tried one after the other), `--- find` with the exact text
    (it must occur once in the file; no blank line, in the find or the replace text: `tools/mutate.sh` and `--lint` refuse a list that has one, naming the mutant; `tools/check-mutate-lint.sh` tests that), `--- replace` with the text that takes its place. Write the mutant
    that breaks the rule, run it, and fix the tests until it is killed; a mutant that is equivalent (no input can see the
    change) does not belong in the list.
  - A survivor the owner has decided to keep gets `accepted: who, date, why` after `test:`. It is printed as `ACCEPTED` with the reason at every run, counted apart in the summary line (", N accepted"), and fails nothing; once a test kills it, it reads `killed` and the mark can go. Only JOD's decision puts one there.
  - **Long suites.** A ui case that takes more than about 15 s is registered under `/long/NNNN/` and runs as the meson test
    `ui-NNNN-long` (suite `long`), which the full `meson test` (the group gate) and `/sdd-check` run and `tools/mutate.sh` does not:
    a mutant whose every `test:` ends in `-long` is printed as `LONG-ONLY` with what it needs, not run, counted apart in the
    summary line (", N long-only (not run)") and fails nothing; `/sdd-check` runs it by hand against that suite. A long case keeps a
    fast version in the short suite wherever that can kill the same mutants, so that only a mutant which truly needs the long case
    (0014: a path of more than 65536 corners) is long-only.
- **Progress of a long run.** `tools/mutate.sh` reports its progress on stderr and in a file, never on stdout (the result lines and the summary line of stdout are as they were): it prints `progress file: <path>` first, then for each mutant a line like `[48 of 263] 4:51 elapsed, about 22:10 left: mutant 48, REQ-... name`, one when it is done, and, while one mutant runs, a line every 30 seconds (`EASEL_MUTATE_HEARTBEAT` seconds; `still running REQ-... name, 31 s`). The estimate is the average of the finished mutants times the ones to come, less what has passed since the last one ended. A run started in the background is watched with `tail -f` on that file (or a Monitor on it) instead of waiting in silence for the summary line; `tools/check-mutate-progress.sh` checks all of this on the three mutants of REQ-0011-04, with one worker and no kept case. Each worker's session output is in `.scratch/mutate/wN/session.log`.
- **What to run when (the skills point here; JOD, 2026-11-02: time and tokens go to the product, not to more test runs):**
  - **After a task:** the feature's ui cases (`tools/isolated.sh meson test -C build --print-errorlogs ui-NNNN`) and the core and io tests. No `tools/mutate.sh` per task; `tools/mutate.sh --lint` (seconds) when the task edited a block a mutant quotes.
  - **A task that changes a shared menu or window furniture** also runs the features whose tests count its sections or items (0021: two separators in the primary menu broke `ui-0013`, seen only at `/sdd-check`).
  - **At the end of a spec:** `tools/mutate.sh --core`, once, in the background: only the mutants of files under `src/core`. `src/ui/canvas.c` and `src/ui/window.c` are not mutated per spec.
  - **At `/sdd-check`:** the full `meson test` and the scale 2 run through `tools/isolated.sh`, with the result of that core run.
  - **At the phase gate:** one full run of the list (`tools/mutate.sh`), together with the phase's acceptance spec, which also runs once under Vulkan, and its journeys 1 and 4 on a screen of scale 2. Vulkan is a command and not a test of the normal suite: `tools/isolated.sh env GSK_RENDERER=vulkan GTK_A11Y=none GDK_DEBUG=no-portals EASEL_TEST_LANGUAGE=en EASEL_TEST_DATA=$PWD/tests/data build/tests/test-window -k -p /0028`, and journey 15 by `-p /0028-uk` with `LC_ALL=C EASEL_CHOSEN=uk` and no `EASEL_TEST_LANGUAGE`. Scale 2: `tools/isolated.sh --scale 2 meson test -C build ui-0028-scale2`. A clean full run records its commit and the GTK, Pango, Cairo and GNOME Shell versions in `docs/specs/INDEX.md`, and names any that changed since the last record.
  - **A survivor** is one line in the spec's Check section ("survivor, accepted, reason"). It becomes a task only if it shows a user-visible bug.
- **Code that can abort the process** (a failed allocation in GLib, gdk-pixbuf or glycin) is tested in a subprocess
  (`g_test_trap_subprocess`, as `as19_save_without_memory` and `as8_open_without_memory` do), so that an abort is a failed
  assertion of the test and not a crash of the run.
- **Which allocations can fail softly:** `malloc`, `realloc` and GLib's `g_try_*` return NULL; `g_new`, `g_malloc`, `g_strdup` and
  `g_strndup` abort. A spec sentence for a memory failure names only a path that can return NULL, and a NULL test after an aborting
  allocation is dead code (0017 F2: a copy's dialog for a `g_strndup` that cannot fail softly).
- **A test-only switch** that makes a failure path reachable (a step limit, a show limit, a rich-text limit) is off by default, leaves
  the production path unchanged, and is proven used by a mutant that removes it.
- **A long case** that asserts a style or a set of flags asserts the bit the step is about, or compares with a neighbour's state:
  a whole value also holds what earlier steps of the case left (0017 T018b: a typed character inherited Underline).
- **Data that crosses a process boundary** (a clipboard write, a pipe, a socket) is tested with more bytes than a pipe holds at once
  (64 KiB on Linux), read back whole from the other end; a small payload passes a write that writes only what the pipe takes (0016:
  every copy over 64 KiB reached other apps cut off, unseen until M1).
- **A seam for a desktop service** (0016's `EaselClipboard`) mirrors the transport, and its Plan names what it cannot see: the bytes
  and the time another process gets (a pipe's size, an encode made at the request, the other app's patience), the compositor's rules
  (a clipboard set only after input), and when a cancelled call calls back (GIO: several iterations later, so the double answers a
  cancel late). Each gets a two-process probe and a manual step naming the other app, in the first task that serves the data, not
  only at the end.
- **Below 100 %:** a ui test that compares two ways of drawing the same thing (a merged line against its steps, bands against
  rows) is exact at 100 % and above; below 100 % antialiasing makes them differ, so it checks a bound the owner approved: the
  differing pixels lie on the drawing only, and the largest difference in a channel (and, where it is stable, their number) stays
  just above what the first build gave; it must fail when the drawing moves by a whole picture pixel (0014 T015).
- **Scrolling:** a ui test of anything drawn for the part in view scrolls and renders without asking the canvas to redraw (`wait_for_next_frame` queues one, which hides a missing redraw); `render_as_is` waits for the layout only.
- **Timing:** assert the path that started the work, not a count of frames; an idle loses to a pending frame.
- **Gates:** build warning-free, all tests green after each task; `tools/spec.sh lint` clean before a spec is accepted (`tools/spec.sh lint --names NNNN` additionally fails when a test named in that spec's scenario-to-test map is not registered in `tests/`; `/sdd-check` runs it, the plain lint does not, because a Plan names its tests before they are written).
- **Later:** CI in a container or the Flatpak SDK (libadwaita >= 1.8 needed); screenshot checks only if they prove worth the upkeep.

## Performance budgets
Accepted by JOD (2026-10-15) as the starting budgets. Measure in features 0001 and 0004; change a number by ADR.
Rule P5 applies: no optimization without a measurement against a budget here.
A change to a shared hot loop (a resampler, a lay, a row walk that several features call) reruns the benchmark of every feature that
uses it, compared with a build of the last commit (a git worktree), the two builds' runs interleaved so that both see the same load; never
with numbers written in a spec weeks before (0018: `scale_region` made 0015's 8000 x 8000 release 63 % slower with every test green).

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
| Text: the font card's family list, first opening to its painted frame | < 250 ms with ~300 families; later openings within two frames (JOD, 2026-10-27; measured 187 to 192 ms and 16 to 20 ms) |
| A gradient or a moved selection below 100 %, Fit to Window on 8000x8000 in 1920x1080 | median frame ≤ 33 ms, view memory ≤ 8 bytes per device pixel (ADR-0004, 0031; measured 16.6 to 16.7 ms, 375 ms before) |
| Remove background (ADR-0007, the classical method) | the preview within 0.3 s at any size; the result within 1 s at 1920x1080 and 2 s at 8000x8000, at most 1 GB beyond the picture and its undo step; the opening's analysis and the result in 0035's worker thread with its progress dialog and Cancel; measured 2026-11-10: 8000x8000 opening 263 to 315 ms (the first in a process over, hence the worker), a Tolerance change 144 to 149 ms, the result 0.94 to 0.96 s, 570 MB beyond the picture; the stability check (2026-11-10) adds one label of the copy to the opening when the automatic tolerance is above 7 (about 110 ms at 8000x8000, 30 ms at 1920x1080) and up to four when a collapse must be located (about 440 ms at 8000x8000, 100 ms at 1920x1080), so the opening at 8000x8000 is then over the 0.3 s: accepted by JOD, since it runs in the worker with the progress dialog after 0.5 s (JOD, 2026-11-09 and 2026-11-10; ADR-0006's model row withdrawn) |

A benchmark refuses to run while another Easel or test window is open (`perf-common.h`), as the ui tests do, since a second window changes its timings, and it runs in the real session, never through `tools/isolated.sh` (see Testing).

Each benchmark row comes from at least three runs, with the load average (`uptime`) noted; a run that disagrees with the others is
run again five times in a row before a cause is looked for in the code (0014: 21 against 55 ms and 24 against 16.6 ms were the
machine's state).

A benchmark that drives the window by calls runs with the pointer outside its window: a real pointer motion with no button down is a
lost release (REQ-0007-18) and ends the held gesture early (0015: a worst event of 150 to 195 ms and a release of 0 ms against 30 ms and
133 to 136 ms). An event worse than three times the median is hunted, with a breakpoint on the slow path, before its row is written,
not recorded as unexplained. A memory row says what its "before" is (the process's peak, a high-water mark shared by the cases of one
run).

A frame benchmark is read only with its window visible (a covered window gets no frame callbacks on Wayland), and the count of
presented frames is read before any frame figure (0025). A benchmark of a share (removed, kept, killed) prints its counts beside the
percentage, and a result that is perfect or arrives too easily is read twice: its rule broken on purpose, its output looked at (0019's
381 of 381 kills against a failing test; 0035's 100 % on every picture, then a wrong model failing every one).

Work another app waits for is timed where it waits: a clipboard provider's encode from the other app's request to the last byte, not
only the action that set it up (0016: a 3178 x 3099 copy's PNG took 2.2 s at the request and a browser gave up; 0.11 s at the fast
level).

The Fill and Magnifier budgets are benchmark budgets: `perf-0007` prints the numbers, `/sdd-check` reports them against the
table, and no test fails on them (timing tests flake on a loaded machine). The worst-case fill (the comb) has no time budget;
its cost is memory, 12 bytes per one-pixel span.
