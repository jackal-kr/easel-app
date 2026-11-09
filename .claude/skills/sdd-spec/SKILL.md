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
   A drawing feature's spec states how it meets P7 (the constitution's pixel contract) points 1 to 4: exact colors, a view that only magnifies, preview and put-down by the same code, the same input giving the same pixels; a requirement that cannot meet one names the exception and the reason.
   A requirement about how something looks names the reference it follows. A control that repeats an existing control's meaning reuses its look; say so, and ask only where a mock-up draws it otherwise.
   A mode or option whose name or tooltip names an effect on the picture is checked against every place the picture changes (a hole, a clear, a put-down) before the spec is accepted.
   A control the user can reach from the keyboard has its key and its tooltip in the spec, decided with the control, not after it.
   Every key a spec names is checked against the desktop's own shortcuts before the spec is accepted (`gsettings list-recursively` of
   `org.gnome.desktop.wm.keybindings`, `org.gnome.shell.keybindings`, `org.gnome.settings-daemon.plugins.media-keys` and
   `org.gnome.mutter.keybindings`); a key the desktop takes is changed in the spec, not found at M1 (0018: Shift+Ctrl+R).
   Every way a control can change a value (its buttons, the arrow keys, the Page keys, the wheel, typing) is listed in the spec with
   the value's mixed and limit states; a wheel over a floating control is checked against the view's own scrolling.
   A control whose "nothing chosen" or "the same picked again" matters is probed with the widget meant for it before the spec names
   that widget (GtkDropDown cannot unselect, nor report a pick of the selected row).
   For any tool or control that holds state between two events (a press and its release, a drag and its end, a dialog and
   its answer), the spec needs a scenario for the second event never arriving, and one for the view (zoom, scroll, window size) changing while it is held.
   A rule for a failure (memory, I/O) that names one event says what happens to the thing in every other state it can be in (held, waiting, unshown).
   A memory limit named in a rule says whether it counts each allocation or a total, and the Plan's "What grows" says the same.
   A new region of the canvas that takes presses (a marker, a handle) is checked, with its hit area computed, against the press coordinates of every scenario of the feature; a rule that one hit area wins over another is tried at the edges of both areas, also where one reaches inside the picture.
   A reference to a spec that is not built yet uses the feature name, not its number, because roadmap numbers move.
   A scenario that needs something to be missing (a font, a file, a device) names one no system can have, not a real one.
   If this feature changes a requirement of an implemented spec, add a pointer note to that requirement.
   A change to an implemented behavior read from a video or a screenshot is shown to JOD as a still frame with the reading written
   under it, and confirmed, before any spec changes (0012/0032: a misread screencast cost a core change and eleven translations, then a revert).
   A label the feature adds to a set shown together (a menu, a row of buttons, a dialog's responses) has a mnemonic no other label of
   that set uses, in every language; the set's test checks it (0027, 0035).
   A requirement that says "exact" bounds every quantity it computes with, and the Plan checks that range (0033).
   A drag that shapes something says which part of the shape follows the pointer (a point on it, its peak, its end), shown to JOD on a picture (0033).
   A command specified "as another does" is checked for every rule the other met only by its nature, such as being per pixel (0034).
   A feature that needs a large or optional dependency says what the app does without it; a learned model is judged on inputs whose
   truth is known by construction, away from the edges, and its budgets are set by JOD in an ADR (0035).
   A look is judged in the app's own layout at real size before it is accepted, not on a sheet blown up (0029); a scenario that
   names a window or area size is checked against the sizes the window can reach (0025).
4. Take behavior from XP Paint knowledge, jspaint (behavior, not code), JOD's descriptions and `docs/reference`; never from the
   source code of an existing implementation. If behavior is unknown or has several reasonable answers, do not decide:
   add `- [NEEDS CLARIFICATION: question and the options you see]`. A number in a requirement, a scenario or an edge case that comes
   from arithmetic is computed with a script for every value it names, not estimated or argued (0031, 0032). A rule copied from the reference behavior is tried on the simplest picture a user makes (one color, one pixel) before it is accepted. A direction on the pixel grid is said as larger or smaller x or y, never above or below, and a statement that two things are the same is checked with the script that makes the numbers, in both directions.
5. Run the `spec-reviewer` agent once and merge its findings.
6. For a menu, dialog or bar, and for anything drawn on the canvas (an outline, a preview, a marker), ask the user for a reference
   screenshot or mock-up before drafting (for a drawing, at a high zoom), and when a widget choice
   changes a rule (a spin row clamps where a text row disables OK), say so in the same question.
   Ask JOD the open questions with AskUserQuestion: at most 4 per round, a recommended option first, concrete examples;
   for a default the user sees, show the result on JOD's own screen (e.g. 1920x1200 -> window 1440x900 -> picture 1024x768).
   Write each answer into the spec as a requirement or edge case and delete the question.
7. `tools/spec.sh lint`. Show JOD the short spec. When JOD says "go" and lint is clean: `Status: accepted`, with a line
   `Reviewed: spec-reviewer, <date>` under `Status:` naming the run of step 5 (the lint refuses `accepted` without it). Stop: no plan, no code.
