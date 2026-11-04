# 0027 Preferences buttons

Status: implemented
Reviewed: spec-reviewer, 2026-11-04
Depends on: 0020 (Preferences), 0021 (Frame time), 0023 (Language, the long test language)

## Why
Every change in Preferences applies and is saved at once, so a mistake is hard to take back, and the first page has grown to six groups (JOD, 2026-11-04). This feature gives the dialog OK, Cancel and Apply, keeps the live preview, and splits the first page in two. It draws nothing on the picture: the tint and the workspace are the view's, and P7 is untouched.

## Requirements
### The pages
- REQ-0027-01 Preferences shall have four pages, in this order: Appearance, Interface, New Picture and Drawing, and open on Appearance every time. Appearance holds, in this order, the groups Style, Workspace, Selection and Palette; Interface holds, in this order, Language and Panels. The groups' rows, and the pages New Picture and Drawing, stay as REQ-0020-03 to REQ-0020-05, REQ-0021-12 and REQ-0023-04 say, except as this feature says.
- REQ-0027-02 This feature replaces, each with a pointer note at the requirement: in REQ-0020-01, committing a typed value at a close or quit; in REQ-0020-02, three pages, no OK or Cancel, and what a row shows; in REQ-0020-03, -04, -06 and -08, and REQ-0021-12 and REQ-0023-04, a change writing at once, and the groups' place; REQ-0020-07 whole; in REQ-0020-10, the typed value at a close and the error's body. The scenarios of those requirements change with them.

### The buttons
- REQ-0027-03 The dialog shall have a row along its bottom, on every page and not scrolling with the page, holding at its right, in this order, "OK", "Cancel" and "Apply", with no tooltips. This order, XP's, differs on purpose from the app's other dialogs, which stay as they are (JOD, 2026-11-04). OK has the suggested look. Tab reaches them, and each has a mnemonic: Alt+O, Alt+C and Alt+A in English, three different letters in every language. Enter is not a key of OK: on a button it presses that button, and in a number field it commits the value (REQ-0027-08).
- REQ-0027-04 "The state to go back to" shall be the rows' values, the palette's 28 colors and each panel's shown state, as they are when the dialog opens, and again after each Apply. It holds values, not their look: after Cancel, Style System follows the desktop as it is then. Zoom and scroll are not part of it: a scroll a preview clamped stays where it is. A row differs when the value it would write differs in meaning from the state's: colors without regard to case; set against unset always differs; the color an unset swatch shows never does; while Automatic size is on, the numbers shown never do. Apply shall be sensitive while a row or the palette differs, while a number field holds text not yet committed, and while a failed write has values owing (REQ-0027-12); OK and Cancel are always sensitive.

### Preview, keep, go back
- REQ-0027-05 A change shall show at once where it shows in this run: Style, the workspace colors, Tint strength, the panels, Frame time and the palette. Everything else waits for OK or Apply, and only then counts, for this run too: New Picture's rows and JPEG quality; the rows that take effect at the next start write then and apply at that start. No change writes the settings file or the palette file. A picture, its pixels, its undo steps and its modified mark are never changed (REQ-0020-08). Use Default follows the row's value as it now stands, not the file.
- REQ-0027-06 A panel's switch moved away from the state's value shows or hides the panel to match, and the View item follows; moved back to the state's value, it puts the panel as the state has it, so a change changed back leaves no difference, shown or written. The View menu's items still change only the current run and do not move the switch or the key (REQ-0020-03).
- REQ-0027-07 OK and Apply shall commit a typed value, then write every key whose row differs, in one write of the settings file, in the order of the rows in the dialog, by the rules of REQ-0020-09; a key whose row does not differ is not touched, so an invalid value or a hand edit there stays. Apply stays open and sets the state to go back to; OK closes. With no difference neither writes.
- REQ-0027-08 A typed value shall count when the focus leaves its field, at Enter, and at OK or Apply; Enter leaves the dialog open. A press on Cancel may leave the field first, so the value may show for a moment before it goes back. The rest of REQ-0020-06 stays, with each step a change shown as REQ-0027-05 says.
- REQ-0027-09 Automatic size shall own both `width` and `height`. If the switch differs from the state, both are written in the one write: `auto`, or the numbers shown. If it does not differ and is off, each number is its own row. Turning the switch off shows the state's numbers if the state's switch was off, else the size shown at that moment, read as REQ-0020's edge cases say with the panels as previewed.
- REQ-0027-10 Cancel, Esc and the dialog's close button shall close the dialog and put back the state to go back to, everywhere it shows: the theme, the workspace, the tint, the panels with their View menu items, Frame time and the palette. They write nothing and ask nothing, and text not yet committed is dropped. Esc with a color chooser, a popover or an error of the dialog open closes only that.
- REQ-0027-11 Ctrl+W, Ctrl+Q or a close request while Preferences is open shall close, in this order, whatever is above it (a color chooser, whose color is dropped; a popover; an error) and Preferences, doing what Cancel does, with no write and no error, and then quit as REQ-0001-34 and REQ-0001-35 say. If Cancel answers the Save prompt, the app stays, with Preferences closed and the changes gone. If the app ends in any other way while the dialog is open, changes not yet kept reach no file.
- REQ-0027-13 A row shall show, when the dialog opens, what REQ-0020-02 to REQ-0020-06 show for the value of the last OK or Apply in this run or, if none, for the value read at start-up.

### Reset Palette
- REQ-0027-14 "Reset Palette", now with no ellipsis, shall ask nothing: it shows the classic 28 colors of REQ-0003-04 in the swatches at once, as a change like any other, and stays sensitive. If the swatches already hold them, it is no change. OK and Apply then remove the palette file, and no palette file is no failure; Cancel brings the colors back and leaves the file. The foreground and background colors stay (REQ-0003-10). The dialog "Reset the Palette?" goes.

### A new language
- REQ-0027-16 When OK or Apply keeps a language other than the one the app is running in, and the settings file was written, the app shall say so, over what is on top, heading "Language Changes at Next Start", body "Your changes are kept. The new language shows the next time Easel starts." and the one button "OK"; Esc is OK. It changes nothing and restarts nothing (JOD, 2026-11-04: a restart would cost the undo steps and perhaps the picture, so the app never restarts itself). The language comes at the next start, as REQ-0023-05 says.

### When a file cannot be written
- REQ-0027-12 The settings file and the palette file are independent: the settings file is written only if a settings row differs, the palette file is removed only if the palette differs, and neither depends on the other's result. Only the settings write can raise the error. A failed removal is silent (as REQ-0003-16): the swatches keep the classic colors for this run, the state moves, and it is not tried again. If the settings write fails as REQ-0020-10 says, the file shall be left exactly as it is; the values still apply where they apply in this run, and one that applies only at a later start has no effect and its row still shows it. The error shows at each press of OK or Apply that fails, with the body "The changes apply until Easel closes where they can, but settings.ini could not be written.": over Preferences after Apply, over the window after OK. The state moves as after any Apply, so Cancel does not take back a value the error said applies. After a failed Apply the values stay owing while the dialog is open: Apply stays sensitive, and the next OK or Apply tries them again with whatever else differs. A close gives up what is owing.

### What it says
- REQ-0027-15 The Help page's part on Preferences, in every language, shall say that a change the window can show shows at once, that the others wait, that OK or Apply keeps them and that Cancel takes them back (REQ-0023-09). "Interface", the new label and the new error body are in every language, and the three messages of the old dialog go from every language (REQ-0023-10). In the long test language the row of buttons and the four page titles shall show whole and the dialog shall fit, also in a window at its minimum size (REQ-0023-11).

## Scenarios
- AS-1 (REQ-0027-01) Given the dialog, Then its pages are Appearance, Interface, New Picture and Drawing, it opens on Appearance, Appearance has Style, Workspace, Selection and Palette, and Interface has Language and Panels, with Frame time in Panels; When it closes on Interface and opens again, Then it is on Appearance | Verified by: automated (ui)
- AS-2 (REQ-0027-02) Given the requirements this feature replaces, Then each has its pointer note, and 0020's, 0021's and 0023's scenarios that say otherwise are changed and pass | Verified by: automated (ui, the features' own tests)
- AS-3 (REQ-0027-03, REQ-0027-04) Given each of the four pages, scrolled to its end, Then the row shows OK, Cancel and Apply in that order at the right; Apply is insensitive; When a row changes, Then Apply is sensitive; When that row goes back to its value, Then Apply is insensitive; When Apply is pressed, Then it is insensitive and a second press does nothing; Given the focus on a number field, When Enter, Then the dialog stays open; Given only "40" typed in Tint strength and the focus still there, Then Apply is sensitive; When Apply, Then the tint is 40 and the file says 40; Given "abc" typed, When Apply, Then the field goes back and nothing is written; Given `workspace-color=#aabbcc`, When #AABBCC is chosen, Then Apply stays insensitive; Given each language, Then the row's three mnemonics differ | Verified by: automated (ui) + manual M1 (the look and the mnemonics, with the real keys)
- AS-4 (REQ-0027-05) Given Style Dark, a workspace color, Tint strength 40, Tool Box off, Frame time on and Reset Palette, Then each shows at once and neither file has changed, byte for byte; Given Width 300, JPEG quality 50, a Brush and Polski, Then neither file has changed; and in all of them the picture, its steps and its modified mark are as they were; 100 quick steps of Tint strength: the preview keeps up, the file is unchanged, and Apply writes one line; Given `workspace-color` set, When Use Default, Then the button is insensitive and the file still has the key; When Cancel and it opens again, Then the button is sensitive; When Use Default and Apply, Then the key is gone | Verified by: automated (ui)
- AS-5 (REQ-0027-06) Given `toolbox=on`, hidden from the View menu, When the switch goes off and on, Then it stays hidden and Apply is insensitive; Given `toolbox=off`, shown from the View menu, When the switch goes on and Cancel, Then it is shown, its item checked, and the row says off; Given `toolbox=on`, hidden from the View menu, When off, Apply, on and Cancel, Then it is hidden, the item unchecked and the key `off` | Verified by: automated (ui)
- AS-6 (REQ-0027-07) Given a file with comments, unknown keys, `line-width=7` and a hand edit made while the dialog is open, When Tint strength and Style change and OK is pressed, Then only those two lines differ, the hand edit is kept and `line-width=7` stays; Given a value changed and changed back, When Apply, Then the file is byte for byte as it was; Given two missing keys of one section, When Apply, Then they are added in the order of their rows | Verified by: automated (ui, core for the write)
- AS-7 (REQ-0027-08) Given "40" typed in Tint strength, When the focus leaves, or Enter, Then the tint is 40 and the file has not changed; When "45" is typed and OK pressed, Then the file says 45 | Verified by: automated (ui)
- AS-8 (REQ-0027-09) Given `width=300`, `height=200`, When the switch goes on and off, Then 300 and 200 show, Apply stays insensitive and the file is unchanged; Given `width=0`, `height=300`, When off and on, Then the file keeps them; Given Automatic size on and Tool Box previewed off, When the switch goes off and Apply, Then `width` and `height` are the size shown, both written | Verified by: automated (ui)
- AS-9 (REQ-0027-04, REQ-0027-05, REQ-0027-07, REQ-0027-10, REQ-0027-13) Given Tint strength 12, When 40, Apply, 20 and Cancel, Then the tint is 40 and the file says 40, and the row says 40 when it opens again; Given Width 300 and JPEG quality 50, When Cancel, Then the next New and the next JPEG use the old values and the rows show them; When Width 300, Apply, Width 400 and Cancel, Then the next New is 300 wide and the file and the row say 300; When a Brush, Apply, another Brush and Cancel, Then the file and the row have the first and the session's tool is unchanged; When Polski, Apply, System default and Cancel, Then the file says `pl` and the interface is unchanged; Given `line-width=7`, Then the row shows 1 pixel; Given a hand edit of Tint while the app runs, Then the row keeps the value it had | Verified by: automated (ui)
- AS-10 (REQ-0027-10) Given a picture that fits with no scrollbars, Frame time off and every row of AS-4 changed, When Cancel, or Esc, or the close button, Then the rendered window is as it was at opening, the View menu's items as they were, and both files byte for byte as they were; Given a typed "40" not committed in Tint strength, When Esc, Then the tint and the row are as they were; Given a color chooser, a popover or an error open, When Esc, Then only that closes; Given Style System and the desktop's theme changed during the dialog, When Cancel, Then the app follows the desktop as it is | Verified by: automated (ui)
- AS-11 (REQ-0027-11) Given a change not applied and a modified picture, When Ctrl+Q, Then Preferences closes, the change is gone, the file is as it was and the Save prompt shows; When Cancel there, Then the app stays with Preferences closed; the same for Ctrl+W and a close request; Given a typed value not committed, or a color chooser open, or an error open, When Ctrl+Q, Then they close, nothing is written and no error shows; Given no modified picture, Then the app quits with the file as it was; Given a change and a palette reset not applied and the app killed, When it starts again, Then both files and the app are as before | Verified by: automated (ui), and manual M1 for the kill
- AS-12 (REQ-0027-14) Given a changed palette and its file, When Reset Palette, Then no dialog opens and the classic colors show, the file still there; When Cancel, Then the colors are back and the file is unchanged; When Reset Palette and OK, Then the classic colors stay, the file is gone and the foreground and background are as they were; Given no palette file, Then no error; Given the classic colors already, When Reset Palette, Then Apply stays insensitive | Verified by: automated (ui)
- AS-13 (REQ-0027-12) Given a read-only settings file, When a row changes and Apply, Then the file is as it was, the value applies, the error with the new body shows over Preferences; When the error closes and the file is made writable, Then Apply is still sensitive; When Apply, Then the value is written and no error shows; Given the file still read-only, When Apply again, Then the error shows again; When Cancel, Then the value stays; Given the same and OK, Then the dialog closes and the error shows over the window; Given a read-only settings file and a palette reset alone, When Apply, Then the palette file is gone and no error shows; Given a read-only settings file, a Tint change and a palette reset, When OK, Then the palette file is gone and the error shows; Given a palette file that cannot be removed, When Reset Palette, Apply and Cancel, Then the classic colors stay, with no dialog, and at the next start the old colors return; Given no file and no folder, When Apply, Then they are created with the defaults and the changes; Given a file made unparseable while the dialog is open, When Apply, Then the file is as it was and the error shows | Verified by: automated (ui)
- AS-14 (REQ-0027-15) Given each language, Then the Help page's part on Preferences is translated and names OK, Apply and Cancel by the words of the buttons, and `translations` passes with "Interface" and the new body and without the old dialog's messages; Given the long test language, Then the row and the four page titles show whole on each page and the dialog fits, also in a window at its minimum size | Verified by: automated (ui) + manual M2 (JOD reads the English)

- AS-15 (REQ-0027-16) Given English, When Polski and Apply, Then the notice shows over Preferences, and when it closes Preferences is still open and the file says `pl`; When OK with nothing more, Then no notice; Given Polski and OK, Then Preferences closes and the notice shows over the window; Given a modified picture, Then after the notice it is as it was, with its steps; Given a row other than Language, or Language back at the running language, or a settings file that cannot be written, When Apply, Then no notice | Verified by: automated (ui) + manual M1 (the look)

## Edge cases
- Apply, then Cancel with nothing changed since: it closes and changes nothing.
- A key repeat of the Enter or Space that pressed OK or Cancel does nothing after the dialog closes (as Esc's repeat in REQ-0021-08), so that an open text box gets no new line. A double press on OK is one write and one close.
- The window resized while the dialog is open: as REQ-0020's edge cases.
- The dialog opened with a waiting shape, an open text box or a selection: they stay, and a tint shown on them goes back with Cancel.
- The Language row: its subtitle stays "Takes effect the next time Easel starts."; written at OK or Apply.
- `line-width=7` shows 1 pixel, so choosing 3 and then 1 before one Apply leaves the line as it is, by REQ-0027-04: making it 1 takes an Apply at 3 and an Apply at 1.

- To go back to changes that apply at once, should JOD ever want it: the pointer notes of this feature in 0020, 0021 and 0023 each quote the clause they replace, whose text still stands; remove the notes and this feature.

## Out of scope
- A confirmation at Cancel or at quit (JOD, 2026-11-04); Undo inside the dialog; a Restore Defaults button; moving rows other than the groups named; the other dialogs' buttons; restoring a scroll a preview clamped.

## Open questions

## Plan
**Approach.** The dialog stops writing as it goes. It holds a draft of the settings that its rows edit, and one record per key with the text the row would write now, the text of the state to go back to, and the text the file holds as far as the dialog knows. A change updates the draft and pushes to the window only what shows. Keep (OK, Apply) commits typed values, writes the keys that differ from the file in one write, and copies the draft to the window's settings. Every close that is not OK goes back: the dialog's "closed" handler is the one place Cancel, Esc, the close button and a quit all pass through.

**The dialog (probed, T001).** A probe in real frames, 2026-11-04. Our own `AdwDialog` with an `AdwToolbarView` (top: `AdwHeaderBar` with an `AdwViewSwitcher`; content: `AdwViewStack` of the same `AdwPreferencesPage`s; bottom bar: the row): natural 640 × 576, the row fixed. `AdwPreferencesDialog` with its child wrapped in a box: natural height 1793, as the page loses its scroll bound, and it rests on an internal child: refused. A second price: its search, unused here, goes.

**Files.** `src/ui/preferences-dialog.c` (the draft, the records, the row of buttons), `src/ui/window.{c,h}` (the preview and keep functions; the palette split from its file; a write that reports failure; the error shown by the caller), `po/*`, `data/help/*`, `tests/ui/test-window.c`, `tests/mutants.txt`.

**Interfaces.** `gboolean easel_window_write_settings (win, changes, n)`, with no dialog of its own; `easel_window_show_settings_error (win)`; `easel_window_preview_workspace (win, draft)`; `const EaselPalette *easel_window_get_palette (win)`, `easel_window_set_palette (win, palette)`, `easel_window_remove_palette_file (win)`; `gboolean easel_unit_row_is_typed (row)`; in the dialog `keep`, the one place a change comes to count, and `go_back`, called only from the "closed" handler; a panel's shown state read from its action.

**What grows.** Nothing beyond one record per key and two copies of the settings and of the palette while the dialog is open.

**Risks.** R1 A close by `adw_dialog_force_close` emits "closed": `/0020/as1_quit_closes_it` checks it, T007. R2 `gtk_widget_get_ancestor (ADW_TYPE_VIEW_STACK)` of a page in tests, and `AdwViewSwitcher` with four titles in the long language: T008. R3 Enter or Space repeating after OK or Cancel: the window's own key-down mark, T002. R4 Mnemonics shared with other dialogs change in five languages, which is harmless there: T008.

**Also found while building.** The four page titles in the header were cut in the long language, so the switcher has its own bar, with the icon over the title (the header keeps the title "Preferences"). The theme comparison is by value: libadwaita's stylesheet, reloaded after dark and back, redraws its own text and icons with 4139 pixels different, with nothing of ours in it.

**Scenario map.**
| AS | Test |
|---|---|
| 1 | ui `/0027/as1_pages` |
| 2 | the changed cases of `/0020`, `/0021`, `/0023` |
| 3 | ui `/0027/as3_buttons`, `/0023-lang/as14_mnemonics_differ`, manual M1 |
| 4 | ui `/0027/as4_preview` |
| 5 | ui `/0027/as5_panels` |
| 6 | ui `/0027/as6_keep_writes_what_differs` |
| 7 | ui `/0027/as7_typed_value` |
| 8 | ui `/0027/as8_automatic_size` |
| 9 | ui `/0027/as9_apply_then_cancel` |
| 10 | ui `/0027/as10_cancel_goes_back` |
| 11 | ui `/0027/as11_quit_drops`, manual M1 |
| 12 | ui `/0027/as12_reset_palette` |
| 13 | ui `/0027/as13_write_fails` |
| 15 | ui `/0027/as15_language_notice`, manual M1 |
| 14 | ui `/0023-lang/as9_help_page` extended, `translations`, `/0023-long/as11_dialogs`, manual M2 |

Constitution: P6 holds, as the dialog still edits the same file; P7 is untouched, as nothing here writes a pixel.

## Tasks
- [x] T001 The own dialog: four pages and the row (REQ-0027-01, -03; AS-1, AS-3's look) - done when `as1_pages` passes, red first
  Result: `as1_pages` failed first (`"New Picture" == "Interface"`). A probe before it chose the dialog, in the Plan.
- [x] T002 Draft, preview, keep and go back (REQ-0027-04, -05, -07, -08, -10, -13; AS-3, AS-4, AS-6, AS-7, AS-9, AS-10) - done when their cases pass, red first
  Result: `as3_buttons` failed first (`no button _OK`), and `as4_preview`, `as6_keep_writes_what_differs`, `as7_typed_value`, `as9_apply_then_cancel`, `as10_cancel_goes_back` each on their own, from no `Apply` or from a file written at once. `as9` also showed an old bug: the Language row did not show a kept language when opened again, as the row never told the settings. Fixed.
- [x] T003 The panels (REQ-0027-06; AS-5) - done when `as5_panels` passes, red first
  Result: `as5_panels` failed first (`no button _Apply`).
- [x] T004 Automatic size (REQ-0027-09; AS-8) - done when `as8_automatic_size` passes, red first
  Result: `as8_automatic_size` failed first (`no button _Apply`).
- [x] T005 Reset Palette (REQ-0027-14; AS-12) - done when `as12_reset_palette` passes, red first
  Result: `as12_reset_palette` failed first at a NULL button, the label having its ellipsis.
- [x] T006 A failed write (REQ-0027-12; AS-13) - done when `as13_write_fails` passes, red first
  Result: `as13_write_fails` failed first (`no button _Apply`).
- [x] T007 A quit (REQ-0027-11; AS-11) - done when `as11_quit_drops` passes, red first
  Result: `as11_quit_drops` failed first: `tint=30` written where `tint=12` was wanted. A close that drops is `go_back` in the "closed" handler, and the quit hook closes an error above Preferences first.
- [x] T008 Strings, mnemonics, Help, the long language (REQ-0027-15, -03; AS-14) - done when `translations` and all language runs pass, red first
  Result: Red first: `as14_mnemonics_differ` in de, es, fr, ro and ru, `as9_help_page` in six languages (`the page does not name OK`), `as12_no_english` (`"Reset Palette" has the English word Reset`), and `as11_dialogs`, with the titles ellipsized at 99 pixels. Fixed: other letters in those five, which the other dialogs share; Help in all twelve; the switcher in its own bar. New text in the eleven `.po` files, which stay `X-Easel-Reviewed: no`.
- [x] T009 The changed cases of 0020, 0021 and 0023, each shown old and new (REQ-0027-02; AS-2) - done when those features pass
  Result: Changed only where a rule changed; old and new in the list below. No rule moved in 0022, and its tests needed nothing.
- [x] T010 Manual M1 (the look, the mnemonics with real keys, a real double close, Enter's repeat, the kill) - done when JOD has seen them
  Result: JOD, 2026-11-04: the look, preview and Cancel, Esc and the close button, the mnemonics and Enter, Apply then Cancel, Reset Palette, Enter held on OK with a floating selection, the kill, and the notice over Preferences with a real language change, seen in a screenshot: all ok.
- [x] T013 The notice of a new language (REQ-0027-16; AS-15) - done when `as15_language_notice` passes, red first
  Result: first built as an offer to restart, red and green, then taken out at JOD's word (2026-11-04: a restart costs the undo steps and perhaps the picture). The notice's test failed first (`assert_alert`: the old heading, "Restart Easel Now?"). Nothing of the restart remains, in `main` or the close path.
- [x] T011 Manual M2 (JOD reads the English Help text) - done when JOD has read it
  Result: JOD, 2026-11-04: the English Help text read, ok. The translations stay unreviewed.
- [x] T012 The core mutants and `tools/mutate.sh --core` once, in the background - done when its summary line is here
  Result: `430 mutants: 425 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 163 s`: the same four survivors as in 0023's Check, nothing here touching `src/core`. The new rules' 16 ui mutants first left 3: the palette file removed by any keep and Enter's repeat after OK, each given a test, and "OK takes back what it kept", equivalent, as a keep has already moved the state: its flag was redundant and went. Then 4 of 4 killed in `REQ-0027-03`, `-07` and `-14`.

**The changed cases of T009, old and new.**
- `/0020/as1_quit_closes_it`: old, a typed 30 at a close request gives `tint=30`; new, `tint=12` (REQ-0027-11).
- `/0020/as2_opens_on_appearance`: old, a typed 33 and a close keep 33; new, kept by OK (REQ-0027-07). The page is read from the view stack: the dialog's kind, no rule.
- `/0020/as3_style_and_workspace`: old, each style and color is in the file at once; new, after Apply (REQ-0027-07). What shows at once is unchanged.
- `/0020/as5_panel_switches`: old, `toolbox=off` at the switch; new, after Apply.
- `/0020/as6_new_picture_rows`: old, each size and color in the file at once; new, after Apply.
- `/0020/as7_drawing_rows`: old, each key in the file at once; new, after one Apply.
- `/0020/as8_number_fields`: old, 100 steps leave one line of the last value; new, kept by one Apply.
- `/0020/as9_reset_palette`: old, asks, and Reset removes the file; removed, as REQ-0020-07 is replaced: `/0027/as12_reset_palette`.
- `/0020/as12_error_once`: old, the error once for each opening; removed, as that rule is replaced: `/0027/as13_write_fails`.
- `/0021/as12_frame_time`: old, `frame-time=on` at the switch; new, after Apply.
- `/0023/as4_language_row`: old, `language=pl` and `language=auto` at the choice; new, after Apply.
- `/0023-long/as11_dialogs`: a page shown through the view stack, and the search page no longer there to skip; the row and the titles added.

## Check
- Date and commit: 2026-11-04, on `95fb541` with this feature uncommitted.
- Tests: `meson test` 104 of 104 through `tools/isolated.sh` (`ui-0027` 13 cases), the four scale 2 runs pass.
- Mutants: `--core`: `430 mutants: 425 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 163 s`, and no file under `src/core` changed since. 0020's and 0027's own: `39 mutants: 37 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 2 errors in 65 s`; the two errors were mutants of mine that did not build, corrected, and `REQ-0027-12` run again with none surviving.
- Warnings: 0, on a true rebuild of 120 C files.
- Scenarios: each AS has its case in the Plan's map; manual M1 and M2 are JOD's, 2026-11-04. Results are in each task: T001 to T013, none a characterization, all with a first failure.
- Survivor, accepted: REQ-0023-06, Romanian's few stopping before 19: AS-5's numbers have no 19, and the rule as written is right.
- Survivor, accepted: REQ-0023-02, an empty translation showing as nothing: `translations` refuses an empty message before it ships.
- Survivor, accepted: REQ-0023-06, a file with fewer forms than the rule running off its end: `translations` refuses such a file before it ships, and no test feeds the reader one.
- Survivor, accepted: REQ-0023-05, a cut file read past its end: only a catalog damaged after install reaches it, and no test feeds the reader one.
- F1, fixed: after a failed write of a new language, the retry that succeeded showed no notice, as the state had already moved. `language_changes` now compares with what the file holds. Red: `as15_language_notice` at the retry.
- F2, from the build, fixed: the Language row did not show a kept language when opened again (T002).
- F3, from the build, fixed: any keep could remove the palette file had the check been wrong, and nothing guarded it; a case does now (T012).
- F4, noted: the spec's word on Apply and text was reworded after acceptance, old and new shown to JOD: "holds text that would become a different value" is now "holds text not yet committed".
- F5, noted: REQ-0020-02 says "preferences dialog", and the app now has its own dialog of the same look; the Plan's probe says why. Its search, never used, went.
- Known gaps (also in `docs/phase2-plan.md`). Cases of a scenario with no test of their own: in AS-10, Esc with a chooser, a popover or an error open (Esc itself tried by JOD, in the plain case); in AS-11, a quit with a color chooser open; in AS-13, a truly read-only file (an unparseable one takes the same way out), a Tint change with a palette reset at OK, a palette file that cannot be removed, and a missing folder, as against a missing file; in AS-10, Style against a desktop theme that changes during the dialog. The pointer notes of 2026-11-03 in REQ-0002-19, REQ-0004-15, REQ-0005-03 and REQ-0005-14 still say "a change", which now means a kept one. The eleven translations of this feature's strings are unread, as the others.
- Verdict: clean after F1. `Status: implemented` (JOD, 2026-11-04).
