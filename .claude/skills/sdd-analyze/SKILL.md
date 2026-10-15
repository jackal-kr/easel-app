---
name: sdd-analyze
description: Step 6 of the loop, also usable any time. Read-only consistency and coverage check across constitution, spec, plan, tasks, tests and code. Use after implementation or before changing a spec.
---

# Analyze (read-only; do not edit anything except by proposing)

1. `tools/check-trace.sh` and `tools/spec-status.sh`.
2. Coverage matrix: for every REQ and AS in the spec -> task -> test or manual check -> passing? Flag any requirement
   without code or test, any test without a requirement, any task without a trace.
3. Drift: behavior in code not in the spec; plan items not built; spec statements contradicted by the plan or tests.
4. Constitution violations (layering, warnings, S-rules, tests shipped with the change).
5. Launch `code-reviewer` on the diff for correctness; do not duplicate its work.
6. Report: findings ranked by severity with file and id references, then a one-line verdict. If clean and JOD agrees,
   propose `Status: implemented` and propose entries for `docs/learning-log.md` (gaps found, rework, effort).
