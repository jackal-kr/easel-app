---
paths:
  - "docs/specs/**"
---
- spec.md describes observable behavior only: no files, functions, libraries or data structures (those belong in plan.md).
- Requirements use "shall" and an ID `REQ-NNNN-xx`; each has a scenario `AS-n` with `Verified by:`.
- Unknowns become `- [NEEDS CLARIFICATION: ...]`, never a guess. Run `tools/check-trace.sh` after editing.
- Derive behavior from observable behavior, never from the source code of an existing implementation.
