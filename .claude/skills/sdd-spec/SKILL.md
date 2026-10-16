---
name: sdd-spec
description: Step 1 of the loop. Use when starting a feature or when JOD says "spec X". Creates or tightens docs/specs/NNNN-slug.md (behavior only), asks the open questions, and gets it to Status accepted.
---

# Spec

Input: feature number or name from `docs/specs/INDEX.md`.

1. Read `docs/constitution.md`, `docs/product.md`, `docs/reference/*`, `docs/design.md` (reference it, do not repeat it)
   and the specs this one depends on. If the spec file already exists, compare it with its INDEX row first; the row may
   have changed since the draft. If the spec settles a phrase that `design.md` or another doc words differently, fix that doc
   in the same change.
2. If the file does not exist: `tools/spec.sh new NNNN slug` (slug from the INDEX).
3. Fill Why, Requirements (`REQ-NNNN-xx`, "When/While/If ... the app shall ..."), Scenarios (`AS-n`, Given/When/Then, cite the
   REQ ids, end with `Verified by:`), Edge cases, Out of scope. **Behavior only**: no files, functions, libraries, data structures.
   A file the user edits by hand needs its syntax in the spec (comments, spaces, case, repeated keys, bad lines), not only its keys.
   If this feature changes a requirement of an implemented spec, add a pointer note to that requirement.
4. Take behavior from XP Paint knowledge, jspaint (behavior, not code), JOD's descriptions and `docs/reference`; never from the
   source code of an existing implementation. If behavior is unknown or has several reasonable answers, do not decide:
   add `- [NEEDS CLARIFICATION: question and the options you see]`. A number in a requirement that comes from arithmetic is
   computed with a script, not estimated.
5. Run the `spec-reviewer` agent once and merge its findings.
6. For a menu, dialog or bar, ask the user for a reference screenshot or mock-up before drafting, and when a widget choice
   changes a rule (a spin row clamps where a text row disables OK), say so in the same question.
   Ask JOD the open questions with AskUserQuestion: at most 4 per round, a recommended option first, concrete examples;
   for a default the user sees, show the result on JOD's own screen (e.g. 1920x1200 -> window 1440x900 -> picture 1024x768).
   Write each answer into the spec as a requirement or edge case and delete the question.
7. `tools/spec.sh lint`. Show JOD the short spec. When JOD says "go" and lint is clean: `Status: accepted`. Stop: no plan, no code.
