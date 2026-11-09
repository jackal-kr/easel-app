# How we work

Work happens in VS Code with the Claude Code extension. One spec file per feature in `docs/specs/`; three steps:
`/sdd-spec NNNN` (write and settle the spec), `/sdd-build NNNN` (plan, tasks, implement test-first), `/sdd-check NNNN` (review).
Rules: `CLAUDE.md` and `docs/constitution.md`. Feature order: `docs/specs/INDEX.md`. Note surprises in `docs/learning-log.md`.
A retro at least every three features (`tools/spec.sh status` says when one is due): read the log's lines since the last retro,
apply each lesson to the skill, guide, tool or test it names, and add the retro's own line to the log.

Learning path: read the constitution and change what you disagree with; run the pilot (0001) through all three steps slowly;
note what felt heavy or useful; trim the process accordingly; then continue through the INDEX.
