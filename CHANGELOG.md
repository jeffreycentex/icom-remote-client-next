# Changelog

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
