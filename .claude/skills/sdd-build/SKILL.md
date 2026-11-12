---
name: sdd-build
description: Step 2 of the loop. Use when a spec is accepted. Adds a short Plan and Tasks to the spec file, then implements task by task, test first. Stops for JOD's go after the plan and after the tasks.
---

# Build

Precondition: `Status: accepted` in the spec. Otherwise stop and say so.

1. **Plan.** Read `docs/engineering.md` and `docs/design.md`. Fill the spec's `## Plan`: approach in a few lines, files/modules,
   key interfaces, a map from every `AS-n` to a test or manual check, one line on the constitution check, risks.
   A failure-path scenario (write fails, file unreadable) always gets an automated test, never a manual check only.
   A test that a write is never half-written starts from an existing file, since only then do atomic and in-place writes differ.
   A statement in the Plan about how a library behaves (including what happens when an allocation fails) goes in the risks with the test or probe that checks it, run before the code relies on it.
   A preview that must equal the drawn result below 100 % is drawn through the same texture path and filter as the picture, and any plan to sample outside the renderer goes in the risks with a rendering probe run first.
   A probe that measures a widget measures it after it has been shown in a real frame; a number from a hidden widget is not used.
   A change to the layout, or to a flag or state the feature extends, lists every reader of it (sizes as well as offsets), in code and in tests; a flag with a meaning is not reused for a new kind of thing.
   A style rule that must follow the theme lives in `style.css` or is reloaded on `notify::dark`; a media query in the app's own provider is probed where it is used.
   For a data structure that grows with use, say what it stores per item and what it copies.
   A buffer that grows by doubling and is handed to the history is shrunk to its length first.
   A Plan's peak-memory figure lists every buffer alive at the peak and adds them (0041: about 1.3 GB said, 1650 MB measured).
   A put-down, cancel or commit is placed at the one function every path to it goes through, and the Plan names that function.
   A risk that says it is "checked in AS-n" names the task that writes the check.
   Data the feature hands to another process is listed in the Plan with who waits for it, a test above a pipe's size, its time at the request, and a two-process probe in the first task that serves it; the Plan names what a seam cannot see (`docs/engineering.md`, Testing).
   A signal handler's return type is checked against the signal's.
   Write an ADR in `docs/decisions/` only if a choice contradicts the constitution or would be questioned later.
   Show the plan. Wait for JOD's "go".
2. **Tasks.** Fill `## Tasks`: small ordered slices, each `- [ ] T001 title (REQ-..., AS-...) - done when ...`. Every REQ and AS
   is covered, no task without a trace, a manual check at each user-visible slice. If `docs/engineering.md` has a budget for
   what this feature touches, add a task to measure it and record the numbers in the Plan; it comes right after the first task
   that produces the cost, not with the gate.
   A task likely to pass already through shared code says so in the Tasks list and names the break that proves its test.
   Put that task in the same group as any speed fix that relies on how a library behaves, and name the fallback in the Plan.
   The first task of a feature that changes the look of a panel ends with a manual look at that panel, before the other
   tasks build on it. A task that changes a core rule the ui depends on carries the ui change as well, so its group's gate can pass. Show ids and titles. Wait for "go".
3. **Implement**, one task at a time:
   - write the test from the scenario text first, run it and see it fail before writing the code (and say so in the task
     report), then the smallest code that passes; a ui test of anything that
     moves the view also checks where the picture is drawn, by rendering the window; a new case also passes when run alone;
   - a step or buffer kept under a memory budget has a test that compares the memory held (capacity) with the memory counted, not only the length, and so does every allocation a task adds that the limit counts (a list that gains a column, a flag array);
   - a refusal test over a path of several allocations sweeps the limit to find the least limit of each stage and asserts their order, and a limit given to a stage that a stricter one always refuses first is an equivalent mutant, removed with the reason (0042: the outline needed 72 bytes against the core's 48); a "nothing changed" test compares the state before the action with the state after it, never a baseline it assumed (a moved float had left Undo enabled);
   - a key the feature handles that is also an application accelerator (Ctrl+Z, Ctrl+A) is tested with that accelerator's action
     enabled (a picture with undo steps), through the action it activates: the accelerator takes the key first (0038 F1);
   - a size given on the screen in logical pixels gets one case at scale 2 (`ui-NNNN-scale2`): the zoom counts device pixels (0038 F2);
   - a memory test that separates two allocations sets its limit between their sizes, and each break is seen to fail at its own one;
   - a list of cases in a spec's paragraph (every hold a lost release ends, every area a wheel works over) gets a test per case, as a
     requirement's would (0036: the holds were listed, the lost release tested for two, a text box's move left held);
   - a guard put in front of another program's parser walks the input as that parser does (fill bytes, junk, resync), not as the
     format says, and is fuzzed again with the crash files as seeds (beta gate: the first EXIF guard missed both);
   - a test of an input rule calls the function the signal callback calls, with the event's modifier state, never a function deeper down;
   - a numeric table or conversion in src/core gets an oracle test against an independent reference (libm, a standard's formula) over its range, not only determinism (0035: a wrong cube root passed every test);
   - a random oracle's inputs include the empty and the single case (no points, one point, zero length) (0039: 2000 figures passed, a polygon with no corner crashed);
   - a check that something stays the same (a size, a least height, a width) compares with the state before the change or the tool at start, not only the new cases with one another (0039: every tool's least height rose alike and the test passed);
   - a full or gate run that takes minutes has nothing built in its build folder beside it (0039: a background gate ran binaries rebuilt under it);
   - a structure that gains a second form (a mask, now layers) has every reader of each field of the old form found by grep, a NULL check included, not only the callers the Plan lists (0040 F1: `mask != NULL` stood for "there is ink");
   - a view that must equal a stack of translucent layers is drawn as that stack of the renderer's own nodes, never as one color computed by hand (0040 F2: the ground's alpha was dropped);
   - a reference value a test reads from the render is read inside the area the rule covers, and asserted to be what it is meant to be before it is used (0040: the highlight's color read where no highlight was);
   - a dialog's or card's OK, Apply or Cancel is tested by activating its real button (and Esc by its real close), reading the result
     only after the dialog has closed, never by emitting the response or calling the apply function (0035: OK kept nothing, its tests passed);
   - a modifier that decides an action (Ctrl, Shift) is read from the event of the press, and the manual check lists each such action
     to try with the real mouse, since no ui test sends real events;
   - where the keys go after a popover, dialog or menu closes is a manual check in the real session in the task that adds it, not
     only at the feature's M1: the isolated session does not return the focus as the desktop does (0017: GTK gave it back twice there);
   - a test that sets a state to reach a case (a zoom, a tool, the colors) asserts that state before it relies on it;
   - a test that closes a window keeps a weak pointer to it and asserts that it is still there before it touches it;
   - a probe or helper that sets the desktop's clipboard acts from a click and is checked from outside (a second process, `xclip`) before JOD is asked to use it;
   - a test helper that waits asks for what it waits for, and a case that ends far slower than its work is looked into: a wait that runs out its deadline passes in silence;
   - a value in a test that the spec gives no number for (a stretch's source pixel, a free function of an image type) is read from the spec's rule or the header first, not guessed: 0042's first run died on a wrong destructor and a wrong source pixel;
   - a test that changes a process-wide setting (a GTK setting, an environment variable) sets it back to the test program's start value, not to what it takes for the default;
   - a pure function and its test are written in two separate edits, with a build and a red run between them;
   - when work from a later task is pulled forward, its tests are written and run red before that code too;
   - a test of a layout or drawing rule checks what the window lays out or draws, at a scrolled position too, never a value computed from the same constant;
   - `ninja -C build` must be warning-free and the tests green as the Testing section of `docs/engineering.md` says for a task
     and for the end of a group, and how to run the suites is in that section too (the ui tests and the mutation tool run through
     `tools/isolated.sh`, the benchmarks in the real session). A test that fails once and then passes is logged with its values and how it was hunted,
     never just re-run until green;
   - after a task run the feature's ui cases and the core and io tests, no `tools/mutate.sh`; at the end of the spec run `tools/mutate.sh --core` once, in the background (`docs/engineering.md`, What to run when); a kill counts only when the same case fails twice: the tool repeats it and reports `UNREPEATED` otherwise;
   - a claim (in a Result, a note or the scenario-to-test map) that a test covers a requirement names the mutant or the manual break that
     made that test fail; a requirement met by an absence (nothing is pushed, nothing is drawn) is proved by a break written into the spec:
     the old line, the inserted line and the tests that failed (0018: two runs were named for REQ-0018-08 that did not observe it);
   - a test of a flip, a turn or a mirror uses an asymmetric picture; a test that passes at its first run after the code exists is made
     red by taking the code out or breaking it, and that run is reported (0018 T006: a symmetric oracle passed with the mirror missing);
   - a change to a shared hot loop reruns the benchmark of every feature that uses it, against a build of the last commit (a git worktree),
     the two builds' runs interleaved (`docs/engineering.md`, Performance budgets);
   - a task that adds a rule under `src/core` adds its mutants in the same task; a task that moves or renames code a mutant quotes runs
     `tools/mutate.sh --lint` before it is done (0029, and 0035 T003: eight mutants found stale only at the gate);
   - a test seam (a hook, an environment variable, a fake) gets at least one test that goes around it through the user's path, or the
     Plan names the manual check that does (0023: the language hook hid a wrong plural rule);
   - a call that starts another process or opens a launcher gets its two-line probe in the task that adds it (0021: "Could Not Open Help" at M1);
   - a rule over a set of strings (mnemonics) gets its test before the strings change, and a translated string a test names is found
     with grep before it changes (0035);
   - a file the app finds at its install path also has a path for the uninstalled build, and a build's dependency path is meson's own
     `pkg_config_path` (0035);
   - a rule with a rounding in it gets one test value between two steps (0035: "2 per whole delta-E" against "1 per half");
   - a method judged with a margin (shares away from an edge) also gets one test with no margin on a picture whose truth is exact (0035);
   - a feature whose output another tool reads gets a ui test on the real path through that tool: the real button, then the next tool
     (0035: OK kept nothing; Fill after a removal filled patches; both passed every core test);
   - a layout that holds translated text is checked over every real language, the long test language being a guard, not a proof (0023);
   - a mutant is run against the test that can observe its change; one with no visible effect in the window is pointed at the core test and noted as equivalent for the ui;
   - a test of an Undo, a restore or a crop uses an input whose result differs from the input itself (a flipped float), or it proves nothing (0041: the unflipped float's crop was the picture, and a budget mutant survived);
   - a surviving mutant means the case is untested, not that the line is redundant: write the input that needs the line before deleting it (0041: the redraw of a same-size crop, deleted after a survivor, was the check's one user-visible bug);
   - a test of a gate (what a key or an action does nothing in) first asserts the state that makes the gate the cause (the selection exists, the item is enabled), or it tests only the item's insensitivity (0041: the quit prompt);
   - the task that adds a menu item carries its translations, mnemonics, Help row and the update of every test that reads the menu's structure (grep the tests for the section strings), so the suites stay green (0041: pulled forward from two later tasks);
   - a survivor is one line in the Check section ("survivor, accepted, reason") and becomes a task only if it shows a user-visible bug;
   - decide routine things yourself and record them; ask JOD only for product decisions or irreversible ones;
   - tick the task and report in one line, and fill its `Result:` line in the spec, written from the test file, never from the task's
     "done when" (0035 T005 claimed floating, stretched and free-form cases its ui test did not have): the cases by name, the test that failed first and its
     message, or `characterization: <why there was no red run>` (a test that passes on its first run is reported as that),
     and for a manual step one line of what JOD saw;
4. **Spec gap rule.** A wording fix to an accepted spec that changes no behavior goes to JOD first, like any other change of the spec. If you need to decide something the spec does not say, stop. Propose wording, get JOD's yes, update the spec,
   add a line to `docs/learning-log.md`, continue. Never guess in code.
5. At a manual check ask JOD to try it. UI feel cannot be verified headless; say so. Name the active tool in every step,
   and warn when a normal action has a surprising effect.
6. Never commit or push unless asked; no Co-Authored-By or AI trailers. No refactoring outside the task.

A tiny change needs no Plan or Tasks: add or adjust scenarios in the existing spec, then code.
When all tasks are ticked, bring the Plan's interfaces and the scenario-to-test map to the names as built, then suggest `/sdd-check`.
