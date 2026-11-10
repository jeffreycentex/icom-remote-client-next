# Changelog

## v1.1 — 2026-11-10

- Added a DSP tab for the IC-7300MK2 with independent Noise Reduction and
  Noise Blanker levels and a manual notch with adjustable position and width.
- Added Low Power Tune for the IC-7300MK2. It sends a 1 kHz tone for up to
  10 seconds at 5% RF power in USB-D or LSB-D and restores the previous
  power setting after the radio confirms receive.
- Expanded frequency presets from 10 to 15.
- Added a Log tab with a live view and full session log export.

## v1.0 — 2026-11-07

- Added an original radio icon to the app and desktop builds.
- Packaged Windows and Linux as single executables.
- Updated the app title and download filenames to version 1.0.

## v0.2 — 2026-11-07

- Fixed a stuck TX indicator after releasing Space during armed voice PTT. The
  client polls the radio's transmit status so a missed reply no longer leaves
  frequency and FIL1–FIL3 changes blocked after the radio returns to receive.

## v0.1 — 2026-11-04

- Restored the previous credential behavior. The app saves the API token and
  the optional remembered radio password in unencrypted local settings on exit.
  Credentials saved only in the operating system credential store must be
  entered again.
