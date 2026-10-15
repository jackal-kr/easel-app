---
name: sdd-implement
description: Step 5 of the loop. Use when tasks.md is approved. Implements one task at a time, test first, keeping build warning-free; stops at checkpoints and whenever the spec turns out to be ambiguous.
---

# Implement

Preconditions: spec `accepted`, plan agreed, `tasks.md` approved. Otherwise stop.

Per task, in order:
1. Re-read the task's REQ/AS ids in the spec.
2. Write the failing test (or manual check note) first, then the smallest code that passes. Match constitution style.
3. Run `ninja -C build` (no warnings) and `meson test -C build --print-errorlogs`. Fix before moving on.
4. Tick the task (`- [x]`). One short line to JOD: what changed, what passed.
5. **Spec gap rule**: if you need to decide something the spec does not say, stop. Propose spec wording, get JOD's yes,
   update the spec (and plan/tasks if affected), add a row to `docs/learning-log.md`, then continue.
6. At a checkpoint stop and ask JOD to try it. UI feel cannot be verified headless; say so plainly.
7. Never commit or push unless asked; never add Co-Authored-By or AI trailers. Do not refactor outside the task.
When all tasks are ticked, run the `sdd-analyze` skill.
