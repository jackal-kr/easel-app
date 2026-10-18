# NNNN Title

Status: draft | accepted | implemented
Depends on: (feature numbers)

## Why
Problem or value, for whom. Two or three sentences.

## Requirements
Observable behavior only, never files or libraries. Forms: "When X, the app shall Y", "While X, ...", "If X, then ...".
Number as `- REQ-NNNN-01 ...`.
A size or cost bound, or a numeric guarantee, states its assumptions and limits, and must hold for the smallest case
(one item), including any fixed cost per container.
A drag states where the grab point sits relative to what moves, and maps pointer movement, not position, to the result.
A requirement about a whole class (any dialog, any file, any picture) lists the earlier requirements that already say something about its members, and says only what is new.
A requirement that allocates (a picture, a buffer) says what happens when the memory cannot be had, as REQ-0004-26 does.

<!--
    - REQ-NNNN-01 When the user presses the primary button on the canvas with the pencil, the app shall set the pixel under the pointer to the foreground color.
-->

## Scenarios
Each cites the requirement(s) it covers and how it is verified (automated test or manual check).
A rule that picks from a fixed list has a scenario with a realistic input (a photo bigger than the window), not only round numbers.

<!--
    - AS-1 (REQ-NNNN-01) Given a blank canvas, When ..., Then ... | Verified by: automated (core | io | ui: the layer that owns the behavior) / manual M1
-->

## Edge cases
Boundaries, empty states, failures, cancellation, very large input.
- Text the user types (names, values): empty, leading or trailing dots, case, spaces.
- A rule that changes the user's input after they confirmed it (e.g. adding an extension): re-confirm what changed.
- The same action again while its dialog or prompt is already open.

## Out of scope

## Open questions
Open items start a line with `- [NEEDS CLARIFICATION` and a question. Resolve all before `accepted`.

## Plan
(added by /sdd-build after the spec is accepted: approach, files, key interfaces, scenario-to-test map, risks)

## Tasks
(added by /sdd-build after the plan is agreed: `- [ ] T001 title (REQ-..., AS-...) - done when ...`)
(filled in when the task is ticked, a line under it: `  Result: ...`. A code task: the test that failed first and its message,
or `characterization: <why there was no red run>`. A manual step: one line of what JOD saw.)

## Check
(written by /sdd-check as its last step, about ten lines)
- Date and commit: YYYY-MM-DD, `abc1234`
- Tests: `meson test` N of N pass (the cases of this feature: M)
- Mutants: K killed, C killed by crash (test gap), S survived, as `tools/mutate.sh` reports (seconds)
- Warnings: 0
- Findings, each with how it was settled (fixed in T0NN, decided by the owner, left open and why)
