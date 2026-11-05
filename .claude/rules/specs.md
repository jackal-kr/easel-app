---
paths:
  - "docs/specs/**"
---
- A spec describes observable behavior only: no files, functions, libraries or data structures (those belong in its Plan section).
- Requirements use "shall" and an ID `REQ-NNNN-xx`; each has a scenario `AS-n` ending in `Verified by:`.
- Unknowns become `- [NEEDS CLARIFICATION: ...]`, never a guess. Run `tools/spec.sh lint` after editing.
- Derive behavior from observable behavior, never from the source code of an existing implementation.
