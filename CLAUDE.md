# Easel

GNOME-native clone of Windows XP Paint, built spec-first. Owner: JOD (direct, concise answers; small incremental steps; no token
waste). Goal: a good app and learning spec-driven development; note surprises in `docs/learning-log.md`.

## The loop (one file per feature: `docs/specs/NNNN-slug.md`)
1. `/sdd-spec NNNN` — write the spec (behavior only), settle open questions. JOD says "go" -> `Status: accepted`.
2. `/sdd-build NNNN` — add Plan (JOD: "go"), add Tasks (JOD: "go"), then implement task by task, test first.
3. `/sdd-check NNNN` — read-only review against the spec; then `Status: implemented`.
Tiny change: adjust the scenarios in the existing spec, then code.

## Models (JOD switches with `/model`; remind him, do not switch silently)
At the start of each loop step, say in one line which model fits it. Say it only when the model in use is not the right one, and once per step.
- **Opus**: `/sdd-spec`, the Plan and the Tasks of `/sdd-build`, `/sdd-check`, retros, and any failure whose cause is not obvious.
- **Sonnet**: implementing the approved Tasks test-first, mechanical edits (fixtures, renames, docs wording), running tools and tests, drafting commit messages.
- Switch at step boundaries, not in the middle of a task. If an implementation task turns out to need design judgment, stop and tell JOD to switch to Opus.

## Rules (full text in `docs/constitution.md`; it wins on conflict)
1. The spec is the source of truth. No code without an accepted spec. Specs say what, the Plan says how.
2. Every requirement has an ID (`REQ-NNNN-xx`) and a scenario (`AS-n`, Given/When/Then, `Verified by:`).
3. Ambiguity is a spec bug: stop, fix the spec, add a line to `docs/learning-log.md`. Never guess in code.
4. Specs come from observable behavior (JOD's descriptions, `docs/reference/`, XP Paint, jspaint: behavior only, MIT,
   https://github.com/1j01/jspaint), never from the source code of an existing implementation.
5. Small slices; build warning-free and tests green after each task.
6. **Git: never commit or push unless asked.** Imperative messages, body says why. No `Co-Authored-By` or AI trailers, ever.

## Where things are
- `docs/constitution.md` principles | `docs/product.md` vision, scope | `docs/design.md` UI and interaction
- `docs/engineering.md` coding standards, architecture, testing, performance | `docs/decisions/` ADRs
- `docs/reference/` behavior notes (`new-behaviors.md`, `xp-behaviors.md`) | `docs/learning-log.md`
- `docs/specs/INDEX.md` feature map and milestones | `docs/specs/_template.md`
- `tools/spec.sh new NNNN slug | status | lint` (also VS Code tasks)

## Commands (valid once feature 0001 is built; stack per ADR-0001, accepted)
    meson setup build && ninja -C build    # must stay warning-free
    meson test -C build --print-errorlogs
