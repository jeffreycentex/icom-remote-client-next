# Changelog

## v1.2 — 2026-11-15

- Added Data and FLAMP tabs using the verified KiwiSDR Remote Client decoder
  implementation. Supports MFSK16/32/64, Contestia 4-250, RSID Auto, live
  decoder squelch, receiver audio and audio-file decoding.
- Added FLAMP block retention across decoder restarts, raw binary reception,
  automatic and manual saving, safe collision handling and save retries.
  Partial transfers remain in memory until the application closes.
- Bundled the native decoder and its license/provenance resources. Added
  native build and test steps for macOS, Windows and Linux.
- Added two simultaneous receive audio outputs with independent gain,
  saved selections, device recovery and transmit-route checks.
- Enabled connected DATA MOD and DATA OFF MOD switching with PTT release,
  confirmed readback and restoration tracking, while receive audio continues.
- Changed TUNE to toggle the antenna tuner on/off. Low Power Tune and the
  new Powered Tune send a 1 kHz tone, start antenna tuning and continue for
  five seconds in USB, LSB, AM or FM with DATA enabled or disabled.
- Added a Custom receive filter that selects FIL1 and applies a SciPy
  bandpass, plus an independent audio notch configured on the DSP tab.
- Updated setup instructions and added decoder, FLAMP, audio routing and
  connected modulation-source regression tests.


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
