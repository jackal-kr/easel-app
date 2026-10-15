---
name: sdd-specify
description: Step 1 of the loop. Use when starting a feature or when JOD says "spec X". Creates or rewrites docs/specs/NNNN-slug/spec.md (what and why, no technology) from docs/specs/INDEX.md and JOD's intent.
---

# Specify

Input: feature number or name from `docs/specs/INDEX.md` (ask only if it is not there).

1. Read `docs/constitution.md`, `docs/product/vision.md`, `docs/reference/*`, `docs/design/*` (reference them, do not repeat them), and specs this feature depends on.
2. If the folder does not exist: `tools/new-feature.sh NNNN slug`.
3. Fill `spec.md` from the template: Why, user stories (prioritized), numbered requirements (`REQ-NNNN-xx`, EARS-style
   "When/While/If ... the app shall ..."), Given/When/Then scenarios (`AS-n`) each citing REQ ids and `Verified by:`,
   edge cases, out of scope.
4. **Behavior only.** No files, functions, libraries, data structures. If you catch yourself writing one, it belongs in the plan.
5. Gather behavior from XP Paint knowledge, jspaint (behavior, not code), JOD's descriptions and `docs/reference`.
   Never derive behavior from source code. Where behavior is unknown or has several reasonable answers, do not decide:
   add `- [NEEDS CLARIFICATION: question, with the options you see]` under Open questions.
6. Keep it short: one screen per story. Run `tools/check-trace.sh`, then show JOD the spec and the open questions. Stop.
   Status stays `draft`. Do not plan or code.
