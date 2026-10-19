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
   A probe that measures a widget measures it after it has been shown in a real frame; a number from a hidden widget is not used.
   A change to the layout lists every reader of it, sizes as well as offsets, in code and in tests.
   A style rule that must follow the theme lives in `style.css` or is reloaded on `notify::dark`; a media query in the app's own provider is probed where it is used.
   For a data structure that grows with use, say what it stores per item and what it copies.
   A buffer that grows by doubling and is handed to the history is shrunk to its length first.
   A put-down, cancel or commit is placed at the one function every path to it goes through, and the Plan names that function.
   A risk that says it is "checked in AS-n" names the task that writes the check.
   A signal handler's return type is checked against the signal's.
   Write an ADR in `docs/decisions/` only if a choice contradicts the constitution or would be questioned later.
   Show the plan. Wait for JOD's "go".
2. **Tasks.** Fill `## Tasks`: small ordered slices, each `- [ ] T001 title (REQ-..., AS-...) - done when ...`. Every REQ and AS
   is covered, no task without a trace, a manual check at each user-visible slice. If `docs/engineering.md` has a budget for
   what this feature touches, add a task to measure it and record the numbers in the Plan.
   Put that task in the same group as any speed fix that relies on how a library behaves, and name the fallback in the Plan.
   The first task of a feature that changes the look of a panel ends with a manual look at that panel, before the other
   tasks build on it. A task that changes a core rule the ui depends on carries the ui change as well, so its group's gate can pass. Show ids and titles. Wait for "go".
3. **Implement**, one task at a time:
   - write the test from the scenario text first, run it and see it fail before writing the code (and say so in the task
     report), then the smallest code that passes; a ui test of anything that
     moves the view also checks where the picture is drawn, by rendering the window; a new case also passes when run alone;
   - a step or buffer kept under a memory budget has a test that compares the memory held (capacity) with the memory counted, not only the length;
   - a test of an input rule calls the function the signal callback calls, with the event's modifier state, never a function deeper down;
   - a test that sets a state to reach a case (a zoom, a tool, the colors) asserts that state before it relies on it;
   - a test that closes a window keeps a weak pointer to it and asserts that it is still there before it touches it;
   - a pure function and its test are written in two separate edits, with a build and a red run between them;
   - when work from a later task is pulled forward, its tests are written and run red before that code too;
   - a test of a layout or drawing rule checks what the window lays out or draws, at a scrolled position too, never a value computed from the same constant;
   - `ninja -C build` must be warning-free and the tests green as the Testing section of `docs/engineering.md` says for a task
     and for the end of a group, and how to run the suites is in that section too (the ui tests and the mutation tool run through
     `tools/isolated.sh`, the benchmarks in the real session). A test that fails once and then passes is logged with its values and how it was hunted,
     never just re-run until green;
   - tick the task and report in one line, and fill its `Result:` line in the spec: the test that failed first and its
     message, or `characterization: <why there was no red run>` (a test that passes on its first run is reported as that),
     and for a manual step one line of what JOD saw;
4. **Spec gap rule.** A wording fix to an accepted spec that changes no behavior goes to JOD first, like any other change of the spec. If you need to decide something the spec does not say, stop. Propose wording, get JOD's yes, update the spec,
   add a line to `docs/learning-log.md`, continue. Never guess in code.
5. At a manual check ask JOD to try it. UI feel cannot be verified headless; say so. Name the active tool in every step,
   and warn when a normal action has a surprising effect.
6. Never commit or push unless asked; no Co-Authored-By or AI trailers. No refactoring outside the task.

A tiny change needs no Plan or Tasks: add or adjust scenarios in the existing spec, then code.
When all tasks are ticked, suggest `/sdd-check`.
