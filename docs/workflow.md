# Workflow

## The loop (per feature)
| # | Step | Skill | Output | Gate (JOD says "go") |
|---|---|---|---|---|
| 1 | Specify | `sdd-specify` | `spec.md` draft: stories, REQ, scenarios, edge cases, open questions | read it; is it what you want? |
| 2 | Clarify | `sdd-clarify` | open questions resolved, status `clarified` | no `NEEDS CLARIFICATION` left -> `accepted` |
| 3 | Plan | `sdd-plan` | `plan.md`: approach, files, interfaces, constitution check, test strategy | approach agreed |
| 4 | Tasks | `sdd-tasks` | `tasks.md`: small ordered slices, each traced to REQ/AS | order and size feel right |
| 5 | Implement | `sdd-implement` | code + tests, one task at a time | per task or per checkpoint |
| 6 | Analyze | `sdd-analyze` | read-only consistency + coverage report; spec status `implemented` | findings handled |

Run a skill by typing `/sdd-specify <feature>` in Claude Code, or just ask for it. Subagents `spec-reviewer`,
`code-reviewer`, `test-writer` are used by the skills; you can call them directly.

## Rules of thumb
- The spec is cheap to change, the code is not. Argue with the spec before implementation.
- A spec is good when someone who has never seen the app could build the feature from it and you would accept the result.
- If implementation forces a guess, the spec had a gap: fix the spec, log it, then continue.
- Review the spec like code: `tools/check-trace.sh` lints it, `spec-reviewer` critiques it.

## Learning path
1. **Orientation.** Read `constitution.md`, change what you disagree with. Resolve the stack question (ADR-0001).
2. **Behavior notes.** Write down how XP Paint / jspaint behave, and anything you want exactly so, in `docs/reference/`: behavior, not code.
3. **Pilot: 0001 walking skeleton.** Whole loop on the smallest vertical slice (blank canvas, pencil, save PNG).
   Slow on purpose: feel each gate. Write down what surprised you in `learning-log.md`.
4. **Retrospective.** What was missing from the templates or skills? Tune them (they are yours).
5. **Scale.** Continue through `docs/specs/INDEX.md`. Optionally run specs 0002+ in parallel worktrees later.
6. **Review.** At milestones try the built app against the specs and XP Paint, and fill the measures table in the learning log.
7. **Alternatives.** Once the loop is familiar, try GitHub Spec Kit or Kiro on one feature and compare with this hand-built flow.
