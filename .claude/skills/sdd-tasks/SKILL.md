---
name: sdd-tasks
description: Step 4 of the loop. Use when a plan is agreed. Breaks the plan into small ordered tasks in tasks.md, each traced to REQ/AS ids and test-first. No code.
---

# Tasks

Precondition: spec `accepted`, plan agreed by JOD.

1. Split the plan into tasks that each leave build and tests green and take roughly one short session.
   Order by dependency; vertical slices before breadth. Mark independent ones `[P]`.
2. Format: `- [ ] T001 title (REQ-..., AS-...) — files; done when ...`. Every requirement and scenario must be covered by at
   least one task; every task must trace to at least one requirement (no orphan work).
3. Put the test task before the implementation task it drives. Add a checkpoint after each user-visible slice with a manual
   check JOD can do in the running app.
4. Show the list (ids and titles only). Stop for approval.
