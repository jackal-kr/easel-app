---
name: sdd-check
description: Step 3 of the loop, also usable any time. Read-only check of a feature against its spec, the constitution and the code. Use after building, or before changing a spec.
---

# Check (read-only, except the Check section of the spec; propose, do not edit anything else)

1. `tools/spec.sh lint`, `tools/spec.sh lint --names NNNN` and `tools/spec.sh status`.
2. Coverage: for every REQ and AS -> task -> test or manual check -> passing? Flag any requirement without code or test, any test
   without a requirement, any task without a trace. Run everything as the Testing section of `docs/engineering.md` says for
   `/sdd-check`, and how to run the suites is in that section too (all tests, the whole mutation list; through `tools/isolated.sh`); a rule that guards against data loss and has no mutant in
   `tests/mutants.txt` gets one, a mutant that survives is a finding, and so is one killed only by a crash (add an
   assertion). If none does, first check whether any input could observe the change; an equivalent mutant is not a finding.
   A claim that a test covers a requirement is checked by the mutant or break it names; a claim that names none is a finding. A clause
   that names several cases (marked and floating, a success and its failure) has a test per case; a case without one is a finding (0018 F2).
   A background run is awaited by its process id or by the summary line `tools/mutate.sh` prints, and its counts are copied from that summary line, never from a search of the log.
   A crashed mutant is reported with its saved log, `.scratch/mutate/crash-N.log`, copied out first (a later run of `mutate.sh` empties that folder), and is run again alone three times first, to tell the mutant's own flaw from a known flake; one that is inconclusive (no snapshot) is run again once by the tool, and a second one fails the run.
   Read the `Result:` line of every task: copy them into Check, report every `characterization` task as a finding, and
   report a manual step (or a task) without a `Result:` as a finding.
3. Drift: behavior in code that is not in the spec; Plan items not built; spec statements contradicted by tests. Read every
   user-visible string (headings, bodies, tooltips, labels), every conversion (units, rounding, clamps, mappings) and every fallback
   in the code against the spec, and list what the code does that the spec does not state (0017's F2, two memory dialogs, and F8,
   the paste reader's sizes, had this shape).
4. Constitution: layering, warnings, tests shipped with the change, no AI trailers in commits.
5. Read `git diff` for correctness: pixel bounds and overflow, GTK object lifetime, one undo step per user-visible edit.
   A step or buffer kept under a memory budget is checked for the memory held (capacity) against the memory counted, not only the length; every function that pushes a log to the history, and every allocation the feature adds that the limit counts, has its own such test.
6. Report findings ranked by severity with file and id references, then a one-line verdict. If clean and JOD agrees, propose
   `Status: implemented` and any lines worth adding to `docs/learning-log.md`.
7. Last, write the `## Check` section of the spec (see `docs/specs/_template.md`): date, commit, tests, mutants, warnings, and
   each finding with how it was settled. A repeated check replaces the section and keeps the earlier findings in it.
