# NNNN Title

Status: draft | accepted | implemented
Depends on: (feature numbers)

## Why
Problem or value, for whom. Two or three sentences.

## Requirements
Observable behavior only, never files or libraries. Forms: "When X, the app shall Y", "While X, ...", "If X, then ...".
Number as `- REQ-NNNN-01 ...`.

<!--
    - REQ-NNNN-01 When the user presses the primary button on the canvas with the pencil, the app shall set the pixel under the pointer to the foreground color.
-->

## Scenarios
Each cites the requirement(s) it covers and how it is verified (automated test or manual check).

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
