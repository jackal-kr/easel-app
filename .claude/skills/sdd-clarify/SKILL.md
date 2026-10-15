---
name: sdd-clarify
description: Step 2 of the loop. Use on a draft spec with open questions, or when JOD asks to tighten a spec. Resolves NEEDS CLARIFICATION items and hidden ambiguity, then moves the spec to accepted.
---

# Clarify

1. Read the spec. First launch the `spec-reviewer` subagent on it; merge its findings (ambiguous words, untestable
   requirements, missing edge cases, technology leaking in, requirements without scenarios).
2. List every open question, plus ambiguities the reviewer found. Rank by impact on behavior.
3. Ask JOD with AskUserQuestion, at most 4 questions per round, options with a recommended default first.
   Prefer questions with a concrete example ("what happens when ... ?") over abstract ones.
4. Write each answer into the spec as a requirement or edge case (not as a note), delete the resolved question.
5. Run `tools/check-trace.sh`. When clean and JOD agrees: `Status: accepted`. Otherwise `clarified`.
6. Log any case where the original spec was silent on something important in `docs/learning-log.md` (spec gaps table).
