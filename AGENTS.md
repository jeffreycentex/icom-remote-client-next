# AGENTS.md

Project instructions and engineering reference for Icom Remote Next. See README.md for end-user setup and docs/API.md for the HTTP contract.

## Project

Cross-platform (macOS / Windows / Linux) Python desktop client for Icom's
RS-BA1 LAN remote protocol. Target radio: the owner's **IC-7300MK2**
(built-in LAN, no shack PC), **CI-V address B6h**. The IC-705 (A4h) is the
secondary profile. Modelled on the owner's
[kiwisdr-remote](https://github.com/jeffreycentex/kiwisdr-remote): same
architecture, GUI style and reused modules.

Why not kappanhang (Go) itself: it is Linux-only (PulseAudio, pty, terminal
UI), and adding macOS/Windows audio plus a GUI was more work than porting the
protocol into the existing Python app design.

Out of scope by decision: virtual serial port, TCP serial server, waterfall /
scope, CW/RTTY modes.

## Working conventions

- This repository is `jeffreycentex/icom-remote-client-next`. Keep the original client checkout separate and verify the Git remote before pushing.
- Make small changes through the existing service boundary and verify the normal GUI entry point.
- Use the fake radio for transmit, tune, power and menu-write tests. Live checks are receive-only unless the user explicitly authorizes transmission or a radio state change. Use temporary state and disable both MOD management options for receive-only checks.
- Keep real credentials out of source, test fixtures, diagnostics and logs. Ask for relevant sanitized log lines rather than complete packet captures.

## Status

| Step | Content | State |
|---|---|---|
| 1 | Login, 3 UDP streams, RX audio, CI-V read; `python -m icomremote.probe` | verified on IC-7300MK2 |
| 2 | GUI, freq/mode/filter, S-meter, rigctld | verified |
| 3 | TX audio, PTT safety, DATA MOD + DATA OFF MOD switch/restore, voice PTT (armed Space), recording | verified (spots on PSKReporter; DATA MOD restore confirmed) |
| 4 | RF power, TUNE, Output/SWR readout | TUNE + PTT verified; readout redesigned after feedback |
| 5 | Standby detection | Session power logic covered by tests. GUI power button removed by request. |
| 6 | PyInstaller builds + CI | CI green on macOS, Windows, Linux (tests incl. GUI, build, built-app self-check); owner to test the .app and a Windows build on hardware |
| 0.9 | IC-9700 / IC-7610 profiles, per-model menu table (`civ.MODELS`), Linux audio docs, README uses `run_icomremote.py` | tagged `v0.9`; **IC-705 confirmed working by the owner** (2026-10-31); IC-9700 / IC-7610 fake radio only |
| API | Remote control API v1 (kiwisdr-remote compatible subset) for FLDigi Net Scheduler: status, tune, record/start, record/stop; `ctl` client | verified by the owner (scheduler, token) |
| QT | Quick-tune buttons (`quicktune.txt`) | verified by the owner |

## Spec decisions (from the owner)

- **Modes**: LSB, USB, AM, FM and their DATA variants (USB-D …). Nothing else.
- **Filters (option A)**: FIL1/2/3 buttons. rigctl passband is ignored on set;
  `m` reports the selected filter's real width (CI-V 1A 03; FM fixed 15/10/7 kHz).
- **rigctl modes**: `PKTUSB/PKTLSB/PKTAM/PKTFM` -> DATA modes. Plain `USB/LSB`
  -> USB-D/LSB-D by default (checkbox to turn off). fldigi offers USB and
  USB-Data; default to USB-D was the owner's call.
- **Transmit** (safety rules, owner-approved):
  - Data modes: PTT button (toggle), rigctl `T 1`, or armed Space.
  - Voice modes: **only** the space bar while "Voice PTT armed" is on. PTT
    button and rigctl are refused in voice modes (`RPRT -9`).
  - Arming: off at every start and every disconnect. While armed, Space is
    PTT anywhere in the window **including text fields** (owner couldn't
    leave a text field on macOS); arming clears focus.
  - Key-down asks the radio for its DATA state and decides from the radio's
    answer, never from the app's cached state.
  - Unkey on: timeout (default 180 s), voice<->data switch (app or radio),
    disconnect/quit/connection loss, TX input change, rigctl client that
    keyed disconnects, Space release, disarm, window losing focus.
  - TUNE refused while keyed; PTT refused while tuning; tune stopped after
    30 s or on disconnect.
- **One transmit input** ("Audio from") for data and voice; owner switches
  device as needed (two separate inputs made the window too tall).
- **DATA MOD (1A 05 00 85) and DATA OFF MOD (1A 05 00 84)**: switched to LAN
  while connected and **must be restored**. Original written to
  `restore.json` (atomic, fsync) *before* touching the radio; restored and
  read back on disconnect; a record left by a crash is honoured at the next
  connection; manual "Restore now". Owner has had trouble with MOD settings
  before; treat this path as critical.
- **Recording**: received audio only; WAV (48 kHz) or M4A/MP4 (PyAV).
- **Meters**: bar = S-meter always. Separate "Output NN %  SWR N.N" readout
  next to PTT timeout with a dot: green < 1.5, yellow < 2.5, red >= 2.5,
  grey off-air. (A bar that switched to Po/SWR on TX confused the owner.)
- **Layout**: Radio contains connection, tuning, presets and both PTT controls. Audio contains device and routing settings. Recording and control contains recording, CAT and HTTP settings. Tabs use direct layouts with no scroll areas. Verify fit with ten presets and the connection error banner. Display the connection failure once, in the header. Credential feedback shares a fixed-height slot beside Exit, with password on Radio and token on Recording and control.
- **Presets**: up to 10 frequency-only buttons from a file containing `name, kHz` per line, names max 10 characters. The default is `quicktune.txt` in the state directory. Import remembers the chosen path. The file is checked every 0.5 seconds. Tests inject a temporary `quicktune_path`.
- **Standby**: show state and disable tuning and TX. Use the radio's physical power control. Preserve session-level power handling and its regression tests.
- `samplerate` removed from requirements (Windows build pain; linear
  resampling is inaudible at 48 kHz). PyAV stays for M4A.

## Remote control API (docs/API.md)

- Port of kiwisdr-remote's API v1. **Owner's scope: frequency, mode and
  recording only.** No `/connect` / `/disconnect` (404 with a hint), no
  session policy (`session: null`), no PTT / power / MOD endpoints.
  **Control only: it must not interfere with the app's transmit** (PTT
  button, armed Space, rigctld `T`); owner's clarification. The only TX
  interaction is API `/tune` being refused while on air.
- `/tune` is **refused (409 `transmitting`) while keyed, keying, tuning or the
  radio reports TX**; the check and the change run together on the session event loop
  (`test_tune_never_reaches_the_radio_while_keyed`). Plain USB/LSB follow the
  rigctl "plain = data" checkbox (one setting for both). Passband ignored.
- Default port 7373 = KiwiSDR Remote's, so the scheduler needs no change.
- `service.IcomService` owns the `RadioSession`; the GUI records through it
  and follows API recordings via the session's `recording` event.
- Recording gap fill: silence for the time without audio, only after a
  connection ended or standby (`_rec_gap_pending`), not for in-session jitter.

## Architecture

```
icomremote/
  gui.py        PySide6 window. Events from other threads arrive via a Qt
                signal (_Bridge). App-wide event filter handles Space.
  session.py    RadioSession: one worker thread running asyncio; owns the
                three streams; reconnect loop; control API (thread-safe);
                PTT/TX/DATA MOD/tuner logic; audio sinks.
  rigctld.py    hamlib NET rigctl (model 2) server, threaded TCP; from
                kiwisdr-remote. Tracks which client keyed PTT.
  service.py    IcomService: shared control boundary for GUI, CAT and HTTP.
  commands.py   Bounded command worker for ordinary mutations.
  network.py    Tracks and closes active TCP clients and bounds connections.
  secrets.py    Native credential provider selection and storage.
  api.py        HTTP/JSON API server (stdlib), from kiwisdr-remote.
  ctl.py        ApiClient + `ctl` command line.
  civ.py        CI-V framing/BCD, decoder -> (kind, value) events, meter
                scales, filter widths, command builders, menu item tables.
  state.py      modes, hamlib mapping, per-model freq limits, profiles.
  txaudio.py    TxFramer (resample/gain/limit/20 ms frames) + PortAudio input.
  sinks.py      SoundCardSink (PI-controlled clock-drift lock), WavSink,
                M4aSink; from kiwisdr-remote.
  restore.py    on-disk record of radio settings to put back.
  quicktune.py  quick-tune file parser (quicktune.txt, settings dir).
  timerres.py   timeBeginPeriod(1) on Windows while connected.
  probe.py      CLI connection test with --debug packet hex dump.
  rsba1/        protocol: packets.py, passcode.py, seqbuf.py (reorder +
                retransmit), stream.py (UDP base), streams.py (control /
                serial / audio).
tests/          fake_rsba1.py (fake radio over real UDP) + unit, end-to-end,
                TX, step-4 and GUI tests.
```

Threads: Qt GUI, asyncio radio session, bounded application command worker, recording writer, PortAudio callbacks, TCP handlers and service ticker. GUI and CAT mutations go through IcomService. PTT goes directly through the service to the session loop so a recording drain cannot delay key-up. Never synchronously submit service commands from a session or recording-worker callback. Cross into the session with its thread-safe control interface.

- The radio loop owns PTT, its client owner, actual keying and the deadline. Repeated presses never extend the deadline. A second client cannot take over a held key. Leaving Radio disarms voice and releases held Space.
- Stop each TCP server by closing its listener and active sockets. CAT is loopback only, rejects HTTP requests and releases clients' PTT on shutdown. Preserve HTTP Host checks, token and JSON validation, size limits and connection timeouts.
- Recording uses a bounded FIFO writer. Stop drains accepted audio before container finalization. On overflow report an error. Never wait for the writer while holding the sink lock. Queue reconnect silence compactly. Use exclusive file creation for every WAV/AAC segment and bound filename template expansion.

## Credentials and connection diagnosis

- Next uses QSettings identity `IcomRemoteNext` / `Icom Remote Next` and separate state. `ICOM_REMOTE_STATE_DIR` overrides the restore/log/preset directory, not the QSettings identity. Inject temporary QSettings for GUI tests.
- Use native macOS Keychain, Windows Credential Manager, Linux Secret Service/KWallet/libsecret. Filter backend chains to native providers. QSettings stores opaque identifiers only. An unreadable credential must survive exit with an empty field. Saving is immediate on Remember and password edit completion, and the API token has a Save button.
- Keychain may ask for access on app startup when it reads credentials, especially after replacing a build. A temporary native round trip has passed in both source and the macOS bundle. Windows/Linux provider selection has simulated test coverage.
- A control handshake timeout on UDP 50001 happens before login. Check the route and UDP reachability. Ping confirms IP reachability only. Check the app log for a later successful retry or a radio-in-use reply before starting another session. Fixed local ports are a diagnostic option and can collide with other sockets.
- Successful live receive-only check on IC-7300MK2 B6h at 3.595 MHz USB-D verified GUI readback, HTTP status, a 48 kHz mono WAV and clean shutdown. Avoid treating that check as TX coverage.

## RS-BA1 protocol notes

Ported from kappanhang (MIT; notice in THIRD_PARTY_NOTICES.md). Findings from
the real IC-7300MK2 that differ from or add to kappanhang:

- Ports 50001 control, 50002 serial (CI-V), 50003 audio. Handshake per
  stream: 0x03 -> 0x04 (radio sid), 0x06 -> 0x06. Radio sends answers twice.
- Login (0x80) -> 0x60 reply (`ff ff ff fe` at 48 = bad credentials) -> auth
  0x02 and 0x05 -> radio sends **capabilities (0xA8, every login)**, then an
  **unsolicited 0x90 status with in-use byte 96 = 0 (radio free)**, then the
  auth reply. Treating that 0x90 as a refusal was the step-1 bug.
- 0x90 status layout: byte 96 in-use flag, 100..115 client computer name,
  132..135 its IPv4. Our stream request is granted when a 0x90 arrives with
  in-use 1 **and our client name `icom-pc`**. In-use 1 with another name =
  radio held by another client (report name and IP).
- Capabilities: radio name `IC-7300MK2`, CI-V B6h at offset +82 of the radio
  record, audio `ICOM_VAUDIO`.
- Audio: 48 kHz s16le mono, about 10 ms per RX packet; TX sent as 1364 +
  556 byte packets per 20 ms frame; TX buffer 300 ms (larger breaks TX).
- Measured: RTT 6-18 ms over the owner's VPN, 0 packet loss in 20 s.
- Re-login too often can make radios drop audio: 5 fast retries, then 65 s.
- Password is blanked in packet hex dumps (`packets.redact`); keep it that way.

## Per-model menu items (`civ.MODELS`)

Never guess these: wrong item numbers write the wrong menu setting.

| Model | DATA MOD | DATA OFF MOD | LAN MOD level | Power OFF setting | LAN value | Value order | Tuner (1C 01) | Source |
|---|---|---|---|---|---|---|---|---|
| IC-7300MK2 | 00 85 | 00 84 | 00 83 | 01 09 | 05 | MIC, USB, ACC, MIC+USB, MIC+ACC, LAN | yes | Icom IC-7300MK2 CI-V guide |
| IC-705 | 01 19 | 01 18 | 01 17 (WLAN) | 00 73 | **03** (WLAN) | MIC, USB, MIC+USB, WLAN | yes | Icom IC-705 CI-V guide 2020-07-21 |
| IC-9700 | 01 16 | 01 15 | 01 14 | 01 46 | 05 | MIC, **ACC, MIC+ACC, USB**, MIC+USB, LAN | **no** | IC-9700 CI-V guide v2 (rigpix) |
| IC-7610 | — | — | — | — | — | — | yes | official guide not reachable; items left None, never written |

To add a model: take the numbers from its Icom CI-V reference guide, add a
`civ.MODELS` entry, a `FREQ_LIMITS` range, a `PROFILES` entry, and a test in
`tests/test_models.py` (the fake radio reads its item numbers from the same table).

## CI-V items used (IC-7300MK2 reference)

| Command | Use |
|---|---|
| 03 / 05 | read / set frequency |
| 04 / 06 | read / set mode + filter |
| 1A 06 | DATA mode on/off + filter |
| 1A 03 | IF filter width index |
| 15 02 / 15 11 / 15 12 | S-meter / Po / SWR (0000-0255 BCD) |
| 14 0A | RF power 0000-0255 |
| 1C 00 | PTT |
| 1C 01 | tuner: 00 off, 01 on, 02 tune |
| 1A 05 00 85 / 00 84 | DATA MOD / DATA OFF MOD: 00 MIC, 01 USB, 02 ACC, 03 MIC+USB, 04 MIC+ACC, 05 LAN |
| 1A 05 00 83 | LAN MOD level (warn when 0) |
| 1A 05 01 09 | Power OFF setting for remote: 00 shutdown only, 01 standby/shutdown |
| 18 00 / 18 01 | power off / on |

Scales: S-meter 0=S0, 120=S9, 241=S9+60. Po 143=50 %, 213=100 %. SWR 48=1.5,
80=2.0, 120=3.0.

## Session standby and power protocol

- Power state = CI-V liveness: `on` if the radio answers state reads,
  `standby` if CI-V is silent > 3 s while the UDP session is alive. Bare
  OK/NG don't count (the radio ACKs `18 00`), and CI-V is ignored for 2 s
  after a power-off.
- In standby the audio-timeout reconnect is suspended (`AudioStream.require_audio`);
  instead a link check fails the session if *nothing* arrives for 6 s.
- Power OFF: stop tune, unkey, restore DATA MOD/DATA OFF MOD, then `18 00`.
  Power ON: 25 x FE preamble + `18 01`, wait up to 25 s; on wake, re-read
  state and switch the MODs to LAN again (`_power_is_on`).
- **Verified on the IC-7300MK2** (log of 2026-10-24):
  - After `18 00` the network stays up; the RS-BA1 session survives and a
    new login while off is accepted and streams are granted.
  - In standby the radio **answers every CI-V command with NG (`FA`)**, it
    is not silent. Standby is still detected by "no state replies for 3 s";
    NG is logged at debug only.
  - `18 01` (with 25 x FE preamble) over the LAN is ACKed (`FB`) and the
    radio answers frequency reads about 4 s later.
  - `1A 05 01 09` reads `00` ("Shutdown only") on this radio, yet the
    network stays up. Don't predict LAN behaviour from that setting; it is
    only logged.
- The fake radio matches this (NG in standby, ACK on power-on).
  `network_off_when_off=True` models a radio whose network drops (IC-705
  Wi-Fi), which the 6 s link check turns into "stopped responding".

## Concurrency lessons (both found by tests, both fixed)

- **Stale poll replies**: a read in flight when the app changes a setting
  used to overwrite the new value, briefly letting PTT key in plain USB. Fix:
  per-group read counters and fences (`_fence`, `_reads_sent`, `_replies`):
  replies to reads sent before a change are ignored for that group. Same
  idea for tuner status at tune start (`_tune_fence`).
- **PTT request ordering**: requests and automatic unkey run on the session event loop. Preserve owner identity and cancellation of pending key-down so a stale release or stale confirmation cannot leave TX keyed.

- **Restore vs. a running MOD switch** (macOS CI, 2026-10-31): after Power ON
  the wake-up setup runs as a task and switches DATA MOD to LAN *after*
  power is reported on. A disconnect in that window restored nothing, then
  the task wrote LAN: radio left on LAN (record on disk caught it at the
  next connect). Fix: `_finish_mod_setup()` waits (max 8 s, then cancels) for
  the wake-up task and the re-read task before every restore (disconnect,
  Power OFF, Restore now). Tests: `test_disconnect_during_power_on_setup`,
  `test_power_off_during_power_on_setup` (fake `menu_delay`).
- **Waiting for an answer**: `_civ_ask` on a state read accepts only the
  reply to *its own* read (reply counters), not an older poll's reply still in
  flight. Without this, PTT right after switching to USB-D was refused
  (Windows CI, `test_key_right_after_switching_to_data`).
- **Unanswered reads** (owner's IC-7300MK2, 2026-10-31): reads the radio
  answers with NG or not at all left the reply counters behind for good, so
  every later answer looked like an older read's and voice PTT was refused
  ("the radio did not confirm its DATA mode") until the app restarted. Fix:
  pending read times per group (`_pending_reads`); a read unanswered after
  `READ_LOST_AFTER` (0.5 s) is written off. Tests:
  `test_unanswered_data_reads_dont_block_voice`, `test_unanswered_reads_then_data_mode`.

Any new feature that changes radio state or keys the transmitter needs a
race test (see `test_stale_reply_cannot_enable_ptt`, `test_quick_release_never_sticks`).

## Reliability (2026-10-25)

Owner saw, intermittently: no RX audio after connecting (speakers and cable),
and 3x data TX keyed without audio; a restart fixed both. Cause not yet
confirmed from a log. Hardening added (`tests/test_reliability.py`):
- ReorderBuffer follows a counter that went back (`LATE_RESYNC` late packets
  in a row) instead of dropping everything as duplicates; the audio timeout
  is re-armed on *delivered* audio, so a stalled buffer now reconnects.
- Sound-card output whose callback stopped while audio arrives: reopened
  (`_check_sink`, 2 s).
- TX input delivering no frames 1 s after key-down: reopened once, then an
  error status (`_tx_input_check`).
- DATA MOD / DATA OFF MOD unreadable: PTT refused (was allowed with the value
  unknown), re-read every 10 s; key-down waits for MOD setup to finish.
- Log always written to the settings dir, last 5 runs kept (`.1` = previous),
  `health:` line each minute. Ask for `icomremote.log.1` after a bad session.

## Environment and testing

- `python -m unittest discover -s tests -v`; also run with
  `-W error::ResourceWarning`. Repeat `tests.test_tx` a few times after
  touching PTT code.
- The fake radio follows the real IC-7300MK2 packet order. When the real
  radio shows new behaviour, change the fake to match and add a test.
- This Mac has a project-local `.venv` with Python 3.14 and the hash-locked dependencies. Use `QT_QPA_PLATFORM=offscreen ICOM_REMOTE_STATE_DIR=/tmp/icom-next-test-state .venv/bin/python tests/ci_run.py` for the CI-style test run. Individual modules can be run with `-m unittest tests.test_gui tests.test_secrets`.
- Python 3.15 on macOS: `loop.getaddrinfo` failed in the worker thread
  (`'lazy_import' object is not callable`). Workaround in place: resolve with
  `socket.getaddrinfo` in the session thread; `ThreadPoolExecutor` touched at
  import. Don't reintroduce executor use in the session loop.
- Debug a real radio with `python -m icomremote --debug --log-file icom.log`
  or `python -m icomremote.probe --host … --debug`.

## Packaging and CI

- Build locally with `.venv/bin/python build.py --no-install`. `build.py` -> PyInstaller (`icomremote.spec`) -> runs the built app with
  `--check` -> zip (macOS via `ditto`, keeps signature/symlinks) / zip
  (Windows) / tar.gz (Linux) in `dist/`.
- `--check` (`__main__.check`) reports Qt, PortAudio, PyAV, resampler,
  protocol modules and credential backend discovery. `--check-credentials` also writes, reads and deletes one unique temporary secret. For diagnostics use a temporary state directory. Exit 0 = required parts OK. In a windowed build stdout
  is None, so the result goes to the log file too.
- Built app always logs to `<settings dir>/icomremote.log` (overwritten each
  start). Ask the owner for it when the .app misbehaves.
- macOS Info.plist needs `NSLocalNetworkUsageDescription` (macOS 15 local
  network privacy) and `NSMicrophoneUsageDescription`; without them access is
  silently denied. The build uses PyInstaller ad hoc signing. Finder shows the complete `dist/Icom Remote.app` bundle as one item. Distribute that bundle or its ZIP.
- `.github/workflows/build.yml`: matrix macos-latest (arm64), windows-latest,
  ubuntu-22.04; Python 3.12; runs the **full** test suite (including GUI and
  M4A tests, offscreen) then `build.py --no-install`; artifacts kept 30 days;
  `v*` tags attach archives to a release. This is the only Windows/Linux
  testing so far, and Windows results can be inspected by reading runs
  with `gh api repos/jeffreycentex/icom-remote-client-next/actions/runs`.
- Version: `icomremote/__init__.py` `__version__` (0.9 = tag `v0.9`). Pushing a
  `v*` tag makes the workflow attach the three archives to a GitHub Release.
- If raw job logs are unavailable, `tests/ci_run.py` and `build.py` report results as annotations:
  `gh api repos/jeffreycentex/icom-remote-client-next/check-runs/<job id>/annotations`.

## Lessons from Windows / CI (2026-10-24)

- Windows + UDP: create sockets via `netcompat.udp_socket` (sets
  `SIO_UDP_CONNRESET` off) and run asyncio with `netcompat.run` (selector
  loop on every OS). The default Proactor loop lost CI-V answers and left
  transports half-closed.
- `time.monotonic()` has 15.6 ms resolution on Windows: use
  `time.perf_counter()` for short intervals (RTT).
- Menu reads (`_read_menu`) accept only the matching answer; stray NGs
  (standby probes) must not satisfy them.
- rigctl `m` right after `M` reports the filter's nominal width until the
  radio's 1A 03 answer arrives (never 0).
- Tests on CI: never assume `sleep()` is accurate; fake sources follow
  wall-clock time; always `wait_for` a second dependent condition; tear Qt
  down in `tearDownModule` (otherwise crash at interpreter exit on all OSes).

## Audio cables

Data needs two virtual cables (RX and TX) or received audio would be
transmitted; the GUI refuses the same device both ways. macOS: BlackHole 2ch
+ 16ch. Windows: VB-CABLE + VB-CABLE A+B or Hi-Fi Cable. Linux: app-created
null sinks IcomRX / IcomTX (use "Monitor of …"). Voice needs no cable.
