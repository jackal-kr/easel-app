---
name: sdd-check
description: Step 3 of the loop, also usable any time. Read-only check of a feature against its spec, the constitution and the code. Use after building, or before changing a spec.
---

# Check (read-only; propose, do not edit)

1. `tools/spec.sh lint` and `tools/spec.sh status`.
2. Coverage: for every REQ and AS -> task -> test or manual check -> passing? Flag any requirement without code or test, any test
   without a requirement, any task without a trace. For each rule that guards against data loss, break it in a scratch copy
   and confirm a test fails; a rule that no test catches is a finding. If none does, first check whether any input
   could observe the change; an equivalent mutant is not a finding.
3. Drift: behavior in code that is not in the spec; Plan items not built; spec statements contradicted by tests.
4. Constitution: layering, warnings, tests shipped with the change, no AI trailers in commits.
5. Read `git diff` for correctness: pixel bounds and overflow, GTK object lifetime, one undo step per user-visible edit.
6. Report findings ranked by severity with file and id references, then a one-line verdict. If clean and JOD agrees, propose
   `Status: implemented` and any lines worth adding to `docs/learning-log.md`.
