# Easel

GNOME-native clone of Windows XP Paint, built with **spec-driven development**.
Owner: JOD (direct, concise answers; small incremental steps; no token waste). Goal is twofold: a good app
and learning how SDD works. Record what we learn in `docs/learning-log.md`.

## Rules (full text: docs/constitution.md; the constitution wins on conflict)
1. **Specs are the source of truth.** Order of work per feature: specify -> clarify -> plan -> tasks -> implement -> analyze.
   Skills: `sdd-specify`, `sdd-clarify`, `sdd-plan`, `sdd-tasks`, `sdd-implement`, `sdd-analyze`.
2. **No code without an accepted spec, plan and tasks** for that feature. Wait for JOD's "go" at each gate.
3. **Specs say what/why, never how.** Plans say how. Every requirement has an ID (`REQ-NNNN-xx`) and a scenario (`AS-n`).
4. **Ambiguity is a spec bug.** Stop, fix the spec, add a line to `docs/learning-log.md`. Never guess in code.
5. **Specs come from observable behavior** (JOD's descriptions, `docs/reference/`, XP Paint, jspaint: behavior only,
   MIT, https://github.com/1j01/jspaint), never from the source code of an existing implementation.
6. Small slices; build warning-free and tests green after each task.
7. **Git: never commit or push unless asked.** Imperative messages, body says why. No `Co-Authored-By` or AI trailers, ever.

## Where things are
- `docs/constitution.md` principles | `docs/workflow.md` the loop and learning path | `docs/learning-log.md`
- `docs/product/` vision, roadmap | `docs/design/` ui-ux, interaction-model | `docs/architecture/` architecture, coding-standards
- `docs/quality/` testing-strategy, performance-budgets | `docs/decisions/` ADRs | `docs/reference/` behavior notes
- `docs/specs/INDEX.md` feature map | `docs/specs/NNNN-slug/{spec,plan,tasks}.md` one folder per feature
- `.claude/rules/` path-scoped rules (specs, core, ui) | `.claude/skills/` the SDD loop | `.claude/agents/` reviewers
- Tools: `tools/new-feature.sh NNNN slug`, `tools/spec-status.sh`, `tools/check-trace.sh` (spec lint) — also VS Code tasks.

## Commands (valid once feature 0001 is planned; stack per ADR-0001, accepted)
    meson setup build && ninja -C build    # must stay warning-free
    meson test -C build --print-errorlogs
