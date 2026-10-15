# Performance budgets

Accepted by JOD (2026-10-15) as the starting budgets. Measure in features 0001 and 0004; change a number by ADR.
Rule P5 applies: no optimization without a measurement against a budget here.

| Area | Budget |
|---|---|
| Start to usable window | < 0.5 s on the dev machine |
| Pointer event to pixels on screen (pencil, brush) | within one frame (16 ms) on a 1920x1080 picture |
| Large picture | 8000x8000 opens and draws without freezing the UI |
| Undo memory | bounded by a user budget (default 512 MB, set in Preferences), never by step count |
| Idle | 0% CPU when nothing is happening |
