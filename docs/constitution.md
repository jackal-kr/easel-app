# Constitution

Non-negotiable principles. Specs, plans and code are checked against this file (`sdd-plan` has a
constitution check). Changing a principle needs an ADR in `docs/decisions/`. JOD owns this file; edit freely.

## Product
- **P1** XP Paint's functionality and workflow, GNOME-native look (GTK 4 + libadwaita). No XP visuals.
- **P2** XP's hidden behaviors are features (`docs/reference/xp-behaviors.md`).
- **P3** No XP-era limits (e.g. undo depth is bounded by a memory budget only).
- **P4** Hard-edged pixels by default (no antialiasing); at 100% zoom 1 image pixel = 1 device pixel.
- **P5** Keep it simple: no threads, caches or fancy rendering until a measurement justifies them.
- **P6** Every user-adjustable default is a setting in a configuration file, not a constant in code; the Preferences dialog edits the same settings.

## Process
- **S1** Specs are the source of truth; code is derived. When code and spec disagree, one is wrong: decide which, fix the spec first.
- **S2** No implementation without an accepted spec, plan and task list. Exempt: typos, formatting, comment-only edits.
- **S3** Every requirement has an ID (`REQ-NNNN-xx`) and at least one scenario (`AS-n`, Given/When/Then);
  every scenario names how it is verified: an automated test or a numbered manual check.
- **S4** Specs describe observable behavior (what/why). Technology, files and structures belong in the plan.
- **S5** Ambiguity is a spec bug: stop, fix the spec, log it in `docs/learning-log.md`.
- **S6** Specs derive from observable behavior (XP Paint, jspaint, JOD's descriptions), never from the source code of an existing
  implementation. This is what makes spec gaps visible. Relax only by ADR.
- **S7** Small slices; after each task the build is warning-free and tests pass.
- **S8** Git: never commit or push unless asked; no AI co-author trailers.
- **S9** Honesty: say plainly where AI wrote code and what JOD has reviewed; no fake review traces.

## Technical (ADR-0001, accepted)
- C, GTK 4, libadwaita >= 1.8, Cairo, Meson/Ninja, Ubuntu 26.04. License GPL-3.0-or-later.
- Layering: `src/core` pure C (libc only) and headless-testable; `src/ui` turns input into core calls.
- Pixels `uint32_t` 0xAARRGGBB premultiplied (revisit in the first plan if a spec makes it unnecessary).
- Warnings are bugs. Tests ship with the change; a bug fix starts with a failing test.
- Details live in `docs/architecture/coding-standards.md`, `docs/architecture/architecture.md`,
  `docs/quality/testing-strategy.md` and `docs/quality/performance-budgets.md`. UX rules live in `docs/design/`.
