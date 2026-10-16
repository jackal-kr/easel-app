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
   A statement in the Plan about how a library behaves goes in the risks with the test or probe that checks it, run before the code relies on it.
   For a data structure that grows with use, say what it stores per item and what it copies.
   Write an ADR in `docs/decisions/` only if a choice contradicts the constitution or would be questioned later.
   Show the plan. Wait for JOD's "go".
2. **Tasks.** Fill `## Tasks`: small ordered slices, each `- [ ] T001 title (REQ-..., AS-...) - done when ...`. Every REQ and AS
   is covered, no task without a trace, a manual check at each user-visible slice. If `docs/engineering.md` has a budget for
   what this feature touches, add a task to measure it and record the numbers in the Plan.
   Put that task in the same group as any speed fix that relies on how a library behaves, and name the fallback in the Plan. Show ids and titles. Wait for "go".
3. **Implement**, one task at a time:
   - write the failing test from the scenario text first, then the smallest code that passes; a ui test of anything that
     moves the view also checks where the picture is drawn, by rendering the window;
   - `ninja -C build` must be warning-free, and the tests of the feature you touched (`meson test -C build --print-errorlogs ui-NNNN`, core and io)
     green after a task; the full run, and `tools/mutate.sh REQ-...` of the group's requirements, at the end of a group. A test that fails
     once and then passes is logged with its values and how it was hunted, never just re-run until green;
   - tick the task and report in one line.
4. **Spec gap rule.** If you need to decide something the spec does not say, stop. Propose wording, get JOD's yes, update the spec,
   add a line to `docs/learning-log.md`, continue. Never guess in code.
5. At a manual check ask JOD to try it. UI feel cannot be verified headless; say so. Name the active tool in every step,
   and warn when a normal action has a surprising effect.
6. Never commit or push unless asked; no Co-Authored-By or AI trailers. No refactoring outside the task.

A tiny change needs no Plan or Tasks: add or adjust scenarios in the existing spec, then code.
When all tasks are ticked, suggest `/sdd-check`.
