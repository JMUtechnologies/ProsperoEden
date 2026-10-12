# Proposal: reviewed game presets and compatibility reports

Players should be able to open a game's settings, inspect its tested preset and
press Confirm, rather than guess combinations of resolution and accuracy switches.
Testers should be able to start a compatibility report from the same page.

## Prototype behavior

Library > Game settings > Recommended settings opens a scrollable preview with
three actions: Confirm settings, Restore previous settings, and Report tested
settings. Applying/restoring takes a second Cross press. There is no automatic
application at game launch. The existing per-title/profile settings store is used.

The bundled database is `headless/game_presets.h`. Each entry names the title ID,
qualified Eden version, evidence, candidate/tested status, and explicit changes.
A mismatched Eden version disables Apply but keeps Restore and reporting available.
Unknown games have no preset but can still be reported.

Apply snapshots only preset-owned fields and writes the snapshot and changed
settings atomically together. Reapplying preserves the original snapshot. Restore
puts those fields back, including missing values that previously inherited defaults.
Other per-game fields (audio, controller mapping, language, mods) and global settings
stay as they are, including unrelated changes made after applying. A restore does
replace later edits to preset-owned fields; its confirmation describes that scope.
Malformed settings/snapshots and failed writes refuse the operation.

There are no approved presets yet. Jamboree is an **experimental candidate** based
on a tester's original PS5/firmware 4.50 running Eden 1.000.110. The user reported
smoother play with logging off and async shaders on. The game update file was
identified as 2.3.0. The exact scene, final graphics and a repeatable controlled
comparison remain unconfirmed. This is not a full-speed or compatibility claim.
It cannot automatically select a hardware/game-update-specific preset yet; the
notes explain the qualification and untested combinations.

## Reporting and publishing

Report tested settings creates a QR code for a prefilled GitHub issue form
(`.github/ISSUE_TEMPLATE/compatibility_settings.yml`). Scan it with a phone, review
and complete the report, sign into GitHub if needed, and submit. The app does not
make an authenticated upload and never needs a GitHub token. This is an in-app
entry point with a phone/GitHub handoff, not direct console submission.

The URL includes only title ID, Eden version, and an allowlisted snapshot of the
current effective video/performance settings. No file paths, game files, keys,
saves, profile names, source addresses, credentials or raw logs are included.
It snapshots current settings rather than attesting what a previous session ran.
The tester must check it and supply the game/update, console/firmware/enabler,
repeatable scene, duration, FPS, graphics and sound results. No Works result is
preselected. Optional evidence is attached manually after review.

The issue form must first be merged into the upstream default branch for the
in-app upstream URL to work. Preview scans do not submit issues.

Maintainers review reports and add/update entries through pull requests. Entries
ship with app updates and work offline; there is no service dependency or automatic
download/acceptance of community submissions. The same list can be documented in
`docs/GAME_COMPATIBILITY.md`. A working report is not automatically a recommended
preset. Suggested promotion criteria:

- A named scene can be repeated with the same game/update and Eden version.
- Native FPS/frame pacing are recorded separately from generated frames.
- Correct graphics, sound, input, and at least a representative gameplay session
  are confirmed; crashes/relaunch differences and compromises are recorded.
- Console model and firmware are explicit. Other hardware/update combinations
  stay untested until someone checks them.
- A maintainer reviews evidence, ideally including an independent confirmation.

## Validation and work before release

Host ASan/UBSan tests exercise apply/restore, missing/inherited fields, preservation
of other settings, repeated apply, profile isolation, malformed input, write failure,
unknown title/version refusal and redacted URL construction. The Linux launcher
preview exercises confirmation/apply/restore/report navigation using synthetic
sample games and an in-memory service; rendered screens have no GL errors.

This prototype has not been cross-built or installed on a PS5. It has no verified
presets yet. The new UI text falls back to English; translation-catalog completeness
currently fails for those additions. Full native build/console acceptance, actual
phone scanning of the long prefilled URL, and the GitHub form after merging remain
required before release. Future work can add hardware/update matching and an
in-app overview/filter of the compatibility list.
