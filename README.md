# Easel

A GNOME-native clone of Windows XP Paint, built spec-first: specifications are the source of truth and code is derived from them.

Status: in development. Each feature's status is in its spec (`docs/specs/`, or run `tools/spec.sh status`).

## Scope
All the tools and menus of Windows XP Paint, plus Pan and Gradient (`docs/product.md`). XP-faithful by default; a later optional
phase adds a few selected modern extras, and smoothing (antialiasing) is an option, off by default.

## Non-goals
XP visual styling; the third (Ctrl) color slot; layers; non-destructive editing; plugins; cloud features; Windows/macOS ports;
ribbon interface, editable shapes after commit, brush gallery and rulers (not planned for Phase 2).

## How we work
Work happens in VS Code with the Claude Code extension. One spec file per feature in `docs/specs/`; three steps:
`/sdd-spec NNNN` (write and settle the spec), `/sdd-build NNNN` (plan, tasks, implement test-first), `/sdd-check NNNN` (review).
Rules: `CLAUDE.md` and `docs/constitution.md`. Feature order: `docs/specs/INDEX.md`. Note surprises in `docs/learning-log.md`.

Learning path: read the constitution and change what you disagree with; run the pilot (0001) through all three steps slowly;
note what felt heavy or useful; trim the process accordingly; then continue through the INDEX.

## Inspired by
- **Microsoft Paint in Windows XP**: the original whose functionality and workflow this app follows (XP version specifically, not later Paint releases).
- **[JS Paint](https://github.com/1j01/jspaint) by Isaiah Odhner**: studied for how the XP behavior works. MIT licensed; no code is copied.
