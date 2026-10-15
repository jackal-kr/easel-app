---
name: sdd-plan
description: Step 3 of the loop. Use when a spec is accepted and JOD wants the technical approach. Creates plan.md with approach, files, interfaces, constitution check and a scenario-to-test map. No code.
---

# Plan

Precondition: `spec.md` status `accepted`. If not, stop and say so.

1. Read constitution, the spec, `docs/architecture/coding-standards.md`, `docs/quality/*` and `docs/design/*`, `docs/architecture/architecture.md` (create it from the first plan: layers, module list, build layout)
   and plans of earlier features, so this plan fits what exists.
2. Fill `plan.md`: summary, constitution check (every principle, not just the ones that pass), approach (modules, files,
   data and state, key signatures), alternatives, test strategy mapping every `AS-n` to a test or manual step, risks.
3. If a choice would be questioned later, or contradicts the constitution, write an ADR in `docs/decisions/` and stop for JOD.
3b. Check the plan against `docs/quality/performance-budgets.md`; name the budget each risky part must meet.
4. If planning shows the spec is wrong or silent, fix the spec first (loop back to `sdd-clarify`) and log it in
   `docs/learning-log.md`. Never absorb a spec gap silently into the plan.
5. Update `docs/architecture/architecture.md` if the plan changes it. Show JOD the plan summary and open risks. Stop; no code.
