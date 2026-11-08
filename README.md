# Icom Remote Next — local architecture revision

Icom Remote Next is a separate revision of the Icom multiplatform client.
It preserves the RS-BA1 protocol and adds a tabbed GUI, safer control and
and recording.

## Run this version

On this Mac, double-click **Launch Icom Remote Next.command**. Its dedicated
`.venv` is installed in this directory. To set up another machine:

```sh
python -m venv .venv
.venv/bin/python -m pip install --require-hashes -r requirements.lock
.venv/bin/python run_icomremote.py
```

On Windows, use `.venv\Scripts\python` for the interpreter. Settings and
restore records use the separate **IcomRemoteNext** identity. The radio wire
protocol and command-line entry points remain compatible. Close the original
client before connecting this version to the same radio or using the same
local CAT/API ports.

## Interface

- **Radio:** connection, tuning, signal meter, preset-frequency buttons,
  DATA PTT, voice arming, RF power, tuner and transmit meters. Import a file
  with **Import presets…**; each line is `name, frequency in kHz`, as in
  `40m FT8, 7074`. Up to ten buttons stay on this main page. The selected file
  is remembered and watched for changes.
- **Audio:** receive and transmit devices, gain, and DATA MOD / DATA OFF MOD
  routing and restoration settings.
- **Recording and control:** recording format/folder, CAT and HTTP controls.
  All tabs use the same light palette and fit their controls without scroll bars.
- Moving away from Radio releases the held Space PTT and disarms voice PTT.
  Voice is also disarmed on disconnect; a hidden tab cannot key the radio.

## Ownership and safety

The GUI and rigctld send mutations through `IcomService`, the same boundary
used by HTTP. A bounded command worker serializes ordinary mutations.
PTT requests go directly from that service to the radio's session event loop,
so a recording drain cannot delay key-up. The loop owns desired PTT, the
current owner, actual keying, and timeout. A second caller cannot take over an
active owner's key-down, and repeat presses never extend its original cutoff.
A mode/frequency/filter service request is validated and applied together on
that loop, and is refused during transmit or antenna tuning.

The servers track active sockets, cap simultaneous clients and apply read
timeouts. Turning a server off closes its clients as well as its listener;
CAT shutdown releases the clients' PTT. CAT is restricted to loopback; use an
authenticated tunnel for remote access. HTTP retains its Host, token and
JSON/CORS protections, validates body lengths and rejects non-finite numbers.
RS-BA1 itself still uses the radio's reversible credential obfuscation: use a
trusted network or encrypted tunnel for that connection.

Recording uses a bounded FIFO writer thread, separate from the radio loop.
Stopping drains accepted audio and finalizes its container. If storage stalls
and fills the queue, recording stops with an explicit error; queued audio is
drained and finalized when storage recovers. A close that remains blocked for
15 seconds reports pending finalization rather than claiming success. Long
reconnect gaps are queued as compact silence requests. WAV and AAC/MP4 files,
including rollover segments, are reserved with exclusive creation: existing
files and symlinks are never overwritten. Recording-name templates accept only
bounded simple named placeholders; format-width expansion is rejected.

The header always shows **CONNECTED**, **CONNECTING**, **DISCONNECTED**, or a
red failure state. Failure details remain visible across tabs and automatic
retries, including after a bad login stops the worker. A successful connection
clears the failure banner.

The app saves settings on exit. Select **Remember password** to save the radio
password, or clear the checkbox to remove it. The API token is saved with the
other settings. Both values are stored unencrypted in local app settings.
Re-enter values previously saved only in the operating system credential store.

## Development and tests

```sh
ICOM_REMOTE_STATE_DIR=/tmp/icom-next-test-state QT_QPA_PLATFORM=offscreen \
  .venv/bin/python -m unittest discover -s tests -v
```

The state-directory override keeps fake-radio restore records out of your
normal configuration. Tests use temporary files and fake radios, including
simulated slow storage, shutdown of live connections, repeated PTT, competing
PTT owners and tab placement/disarming. Live radio checks should be receive-only
unless transmitting has explicitly been authorized.

Dependencies have a cross-platform lock with exact versions and package hashes.
CI actions are pinned to immutable commit hashes. Update the lock with
`uv pip compile --universal --python-version 3.12 --generate-hashes requirements-build.in -o requirements.lock`.
The build workflow grants write permissions only to a separate release job,
and release publishing depends on successful tests and builds on all platforms.

---

The upstream README follows for hardware setup and protocol details. Its GUI
layout, plaintext-password and remote CAT-binding descriptions are superseded
by the local-version notes above.

# Icom Remote (multiplatform RS-BA1 client)

A cross-platform (Windows / macOS / Linux) desktop client for Icom's RS-BA1 LAN
remote protocol, built for the IC-7300MK2. It talks to the radio's built-in
network server directly; no RS-BA1 software or shack PC is needed. The design follows
[kiwisdr-remote](https://github.com/jeffreycentex/kiwisdr-remote).

```
                ┌──────────────────── Icom Remote ────────────────────┐
IC-7300MK2 ◄udp─┤ session thread (asyncio): control / serial / audio  │
  50001-50003   │   CI-V set/read ◄── GUI (PySide6) ◄── rigctld :4532 ◄┼── WSJT-X / fldigi / rigctl
                │   RX audio 48 kHz ──► SoundCardSink (drift-locked) ──┼─► BlackHole / VB-Cable / Pulse sink
                └─────────────────────────────────────────────────────┘
```

![Icom Remote running on macOS](docs/screenshot-macos.png)

*Icom Remote running on macOS (dark mode), before connecting.*

## Status

| Step | Content | State |
|---|---|---|
| 1 | Login, UDP streams, RX audio, CI-V read | done, verified on an IC-7300MK2 |
| 2 | GUI, frequency/mode/filter control, S-meter, rigctld | done, verified on an IC-7300MK2 |
| 3 | TX audio (data and voice), PTT with safety rules, DATA MOD / DATA OFF MOD switch and restore, recording | done, verified on an IC-7300MK2 |
| 4 | Output/SWR readout, RF power, TUNE | done, verified on an IC-7300MK2 |
| 5 | Power ON/OFF | done, verified on an IC-7300MK2 |
| 6 | Standalone builds (PyInstaller), CI on macOS / Windows / Linux | done |
| 0.9 | IC-9700 and IC-7610 profiles, per-model DATA MOD tables | released |
| API | Remote control API for schedulers (status, tune, record) | **new**, tested against the fake radio |

## Supported radios

| Radio | CI-V | Profile | DATA MOD switching | TUNE | Tested |
|---|---|---|---|---|---|
| IC-7300MK2 | B6h | yes | yes (`1A 05 00 85` / `00 84`) | yes | **on the radio** |
| IC-705 | A4h | yes | yes (`1A 05 01 19` / `01 18`, LAN = WLAN `03`) | yes (external tuner) | fake radio only |
| IC-9700 | A2h | yes | yes (`1A 05 01 16` / `01 15`) | no (the radio has no tuner command) | fake radio only |
| IC-7610 | 98h | yes | no: set DATA MOD / DATA OFF MOD to LAN on the radio | yes | fake radio only |

Menu item numbers come from each radio's Icom CI-V reference guide. They
differ between models (including the order of the MIC/USB/ACC values), which
is why the app only writes to radios whose table it has. The IC-7610's items
are not in the app yet: it never writes them. Other radios supported by the
RS-BA1 protocol (IC-785x, IC-7760 …) may connect with *Auto*, but nothing
beyond receive and basic CAT is expected.


## Install

Requires Python 3.10 or newer.

```
git clone https://github.com/jeffreycentex/icom-multiplatform-client.git
cd icom-multiplatform-client
python -m venv .venv
# Windows: .venv\Scripts\activate      macOS/Linux: source .venv/bin/activate
pip install -r requirements.txt
```

- Optional: `pip install samplerate` for libsamplerate resampling. Not needed:
  the radio sends 48 kHz and most sound devices run at 48 kHz, so the default
  linear interpolator only trims for clock drift and is inaudible. (On
  Windows it may need a compiler, which is why it isn't in the requirements.)
- Linux: PortAudio is required: `sudo apt install libportaudio2` (Debian/Ubuntu)
  or `sudo dnf install portaudio` (Fedora).

## Radio settings

In the radio's network/remote settings:

- **Network Control**: ON
- A **network user name and password** (this is what the app logs in with)
- **UDP ports** on their defaults: control 50001, serial 50002, audio 50003
- Note the radio's **IP address** (or give it a fixed one in your router)

On the IC-705 these are under `Menu > Set > WLAN Set > Remote Settings`. I
have not verified the menu path on the IC-7300MK2.

## Run

```
python run_icomremote.py                              # in the virtual environment
python run_icomremote.py --debug --log-file icom.log  # with a packet log
```

Enter the radio's address, user name and password, and click **Connect**.
**Radio** selects the CI-V address: *Auto* uses the address the radio reports
(recommended); the model profiles (IC-7300MK2 B6h, IC-705 A4h, IC-9700 A2h,
IC-7610 98h) force that address.

- **Frequency**: type a value in kHz and press Enter, or use the arrows /
  mouse wheel with the chosen step. Changes made on the radio show up within a
  moment.
- **Mode**: LSB, USB, AM, FM, plus **DATA** for the data variants (USB-D …).
- **Filter**: FIL1/FIL2/FIL3, with the selected filter's passband shown next
  to them.
- **Quick tune**: up to 10 buttons that set a stored frequency (mode and
  filter stay as they are). They come from `quicktune.txt` in the app's
  settings folder, one `name, kHz` per line; names up to 10 characters,
  lines starting with `#` ignored. Without the file there are no buttons.
  Saved changes show up in the running app.

  ```
  40mFT8, 7074.00
  Texas Net, 3573.5
  ```

  Settings folder: macOS `~/Library/Application Support/IcomRemote`, Windows
  `%APPDATA%\IcomRemote`, Linux `~/.config/icomremote`.
- **S-meter**: the bar in the Radio box.
- **Output / SWR** (next to the PTT timeout): measured while the radio
  transmits or tunes, four times a second; the last reading stays shown
  afterwards. Output is the percent of full power. The dot is green below
  SWR 1.5, yellow below 2.5, red at 2.5 or higher, and grey when not
  transmitting.
- **Receive audio output**: pick the virtual cable (or speakers) and tick
  *Play to*.

The password is not saved unless you tick *Remember password*; it is then
stored unencrypted in the app's settings on that computer.

## Recording

**Record received audio** saves what the radio receives (not your transmit
audio) to the chosen folder:

- **WAV**: 16-bit PCM at 48 kHz, as received from the radio.
- **M4A / MP4**: AAC, 64 kbit/s, 48 kHz. Needs PyAV (`pip install av`, included
  in `requirements.txt`); without it these formats are greyed out.

File names carry the time (UTC), frequency and mode, e.g.
`icom_20261024T180000Z_14074.000kHz_USB-D.wav`. Recording can be started before
connecting; it waits for audio. A classic WAV file is limited to 4 GB, about
12 hours at 48 kHz.

## Virtual audio cables

For data modes you need **two** virtual cables, one per direction: if receive
and transmit share one cable, received audio would be sent back to the radio
(the app refuses that combination). Voice needs no cable: *Audio from* is
simply your microphone.

- **Windows:** VB-CABLE gives one cable; add VB-CABLE A+B (two more) or
  Hi-Fi Cable from the same site. All are donationware.
- **macOS:** BlackHole 2ch and BlackHole 16ch install as two separate devices.
- **Linux:** no driver to install. Two PulseAudio / PipeWire *null sinks*
  (IcomRX, IcomTX) act as the cables; each sink's "Monitor of …" source is
  the other end. See [Linux audio setup](#linux-audio-setup-pulseaudio--pipewire)
  for the packages and for making the sinks permanent.

| Direction | Windows | macOS | Linux |
|---|---|---|---|
| Install | [VB-Audio Virtual Cable](https://vb-audio.com/Cable/) + a second cable (VB-Audio *Hi-Fi Cable* or *Cable A/B*) | [BlackHole](https://existential.audio/blackhole/): `brew install blackhole-2ch blackhole-16ch` | nothing: the app's buttons create PulseAudio/PipeWire sinks |
| **Receive**: Icom Remote *Play to* | `CABLE Input` | `BlackHole 2ch` | `PulseAudio/PipeWire sink: IcomRX` |
| Receive: WSJT-X / fldigi input | `CABLE Output` | `BlackHole 2ch` | `Monitor of IcomRX` |
| **Transmit**: WSJT-X / fldigi output | second cable's input | `BlackHole 16ch` | `IcomTX` |
| Transmit: Icom Remote *Audio from* | second cable's output | `BlackHole 16ch` | `PulseAudio/PipeWire source: IcomTX.monitor` |

macOS: the first time the app reads an input device, macOS asks for
microphone permission for the program running it (Terminal, or the app
bundle). If it was denied, the transmit level stays at zero: enable it in
System Settings → Privacy & Security → Microphone.

### Linux audio setup (PulseAudio / PipeWire)

Not yet tested on a real Linux desktop; the steps are standard
PulseAudio/PipeWire usage.

**1. Which sound server?**

```
pactl info | grep "Server Name"
```

`PulseAudio (on PipeWire …)` means PipeWire (Ubuntu 22.10+, Fedora 34+,
Debian 12 desktop …); plain `pulseaudio` means PulseAudio. If `pactl` is
missing, install the tools below.

**2. Packages.** PortAudio (used by the app), `pactl`, and the ALSA "pulse"
plugin, which lets the app pick a specific sink:

```
# Debian / Ubuntu / Raspberry Pi OS
sudo apt install libportaudio2 pulseaudio-utils libasound2-plugins
# Fedora
sudo dnf install portaudio pulseaudio-utils alsa-plugins-pulseaudio
# Arch
sudo pacman -S portaudio libpulse alsa-plugins
```

On PipeWire systems also make sure `pipewire-pulse` is installed (it usually
is); it provides the PulseAudio interface the app and `pactl` use.

**3. Create the two cables.** Either click **Create 'IcomRX' virtual sink**
(receive box) and **Create 'IcomTX' virtual sink** (transmit box) in the
app, or run:

```
pactl load-module module-null-sink sink_name=IcomRX sink_properties=device.description=IcomRX
pactl load-module module-null-sink sink_name=IcomTX sink_properties=device.description=IcomTX
pactl list short sinks | grep Icom          # both should be listed
```

These last until you log out or the sound server restarts.

**4. Make them permanent.**

PipeWire: create `~/.config/pipewire/pipewire-pulse.conf.d/icomremote.conf`:

```
pulse.cmd = [
  { cmd = "load-module" args = "module-null-sink sink_name=IcomRX sink_properties=device.description=IcomRX" flags = [ ] }
  { cmd = "load-module" args = "module-null-sink sink_name=IcomTX sink_properties=device.description=IcomTX" flags = [ ] }
]
```

then `systemctl --user restart pipewire-pulse` (or log out and in).

PulseAudio: add to `~/.config/pulse/default.pa` (create it if missing; the
`.include` line keeps the system defaults):

```
.include /etc/pulse/default.pa
load-module module-null-sink sink_name=IcomRX sink_properties=device.description=IcomRX
load-module module-null-sink sink_name=IcomTX sink_properties=device.description=IcomTX
```

then `pulseaudio -k` (it restarts by itself).

**5. Connect them.**

| Program | Setting | Device |
|---|---|---|
| Icom Remote | *Play to* (receive) | `PulseAudio/PipeWire sink: IcomRX` |
| Icom Remote | *Audio from* (transmit) | `PulseAudio/PipeWire source: IcomTX.monitor` |
| WSJT-X / fldigi | input | `Monitor of IcomRX` |
| WSJT-X / fldigi | output | `IcomTX` |

**WSJT-X and JS8Call** (Qt 5) don't list PulseAudio/PipeWire devices by name:
their lists come from ALSA, so you only see `pulse`, `default` and the
hardware cards, never "Monitor of IcomRX". Route them in `pavucontrol`
(`sudo apt install pavucontrol`; it also works on PipeWire):

1. In WSJT-X / JS8Call → Settings → Audio, set **Input** and **Output** to
   `pulse` (or `default` if `pulse` isn't listed).
2. With WSJT-X running, open pavucontrol. **Recording** tab: set WSJT-X's
   stream to **Monitor of IcomRX** (choose "Show: All streams" if it isn't
   listed). **Playback** tab: set WSJT-X's stream to **IcomTX**.
3. PulseAudio/PipeWire remember this per program, so it survives restarts.
   Repeat once for JS8Call.

Programs that list PulseAudio devices but hide monitor sources (some fldigi
setups) see the receive cable as a normal input after this remap; add the
same line to the persistence config above to keep it:

```
pactl load-module module-remap-source master=IcomRX.monitor source_name=IcomRX_in source_properties=device.description=IcomRX-Input
```

Avoid `pactl set-default-source IcomRX.monitor`: it works, but it also
redirects every other program's microphone to the radio.

## Transmit

One transmit input, **Audio from**, with its gain and level meter, used for
both data and voice: pick the transmit virtual cable (e.g. BlackHole 16ch) for
WSJT-X / fldigi, or your microphone for voice. At key-down the app asks the
radio for its DATA state and applies the data or voice rules below from the
radio's answer.

### Keying

| Control | Data modes | Voice modes |
|---|---|---|
| **PTT** button (toggle) | yes | refused |
| rigctl `T 1` (WSJT-X, fldigi …) | yes | refused (`RPRT -9`) |
| **Space bar**, held, while **Voice PTT armed** is on | yes | yes |

The space bar does nothing unless **Voice PTT armed** is switched on. Arming
starts off every time the app starts and is switched off on every disconnect.
While armed, Space is PTT anywhere in the app's window, even in a text field
(arming also takes focus out of any field); disarm to type spaces again.
Releasing Space unkeys, and so do disarming and the window losing focus.

Keying is refused unless:

- **data:** an *Audio from* input is selected and DATA MOD is LAN
- **voice:** it comes from the armed space bar, an *Audio from* input is
  selected, and DATA OFF MOD is LAN

Transmit stops (PTT released) when:

- the **PTT timeout** expires (default 180 s, whatever keyed it),
- the radio switches between voice and DATA while keyed, from the app or on the radio's front panel,
- you disconnect or quit, or the connection fails,
- the transmit input is changed or disappears,
- the rigctl client that keyed PTT disconnects (e.g. WSJT-X crashes mid-transmission).

Audio is only sent to the radio while PTT is on. The **TX/RX** indicator shows
what the radio reports, not just what was requested.

### Standby

Use the radio’s power switch. When the network stays up in standby, the client
shows **STANDBY** and disables tuning and transmit controls.

### RF power and TUNE

- **Power** sets the radio's RF POWER (0-100 %) when you release the slider;
  changes made on the radio show up within a couple of seconds.
- **TUNE** runs the internal antenna tuner's tune cycle, like the radio's
  TUNER button. It transmits a carrier, so it is refused while PTT is on, and
  PTT is refused while it runs. The cycle is stopped after 30 s or if you
  disconnect; the result ("tuner on" or "tuner off") appears in the status.

### DATA MOD and DATA OFF MOD

The radio takes transmit audio from **DATA MOD** in DATA modes and from
**DATA OFF MOD** in voice modes. Each has its own checkbox (both on by
default). For each one switched on, the app reads the radio's setting when it
connects, writes the original value to disk, then sets it to LAN. When you
disconnect or quit, it puts the original back and confirms it by reading it
back. While DATA OFF MOD is LAN, a microphone plugged into the radio is not
used.

If the app can't put it back (the app is killed, the computer loses power,
the network drops), the original value stays recorded in `restore.json` in
the app's settings folder:

- macOS: `~/Library/Application Support/IcomRemote/`
- Windows: `%APPDATA%\IcomRemote\`
- Linux: `~/.config/icomremote/`

The next connection to that radio restores it on disconnect, and the window
shows the pending restore while you're disconnected. **Restore now** puts
both back immediately (transmit is then refused until you reconnect). At
the radio without the app, change DATA MOD on the front panel (on the IC-7300
and IC-705: Menu → Set →
Connectors → MOD Input → DATA MOD).

On connect the app also warns if *LAN MOD Level* is 0 %, which would make
transmit silent.

Switching is implemented for the IC-7300MK2, IC-705 and IC-9700 (see
[Supported radios](#supported-radios)). For the IC-7610 and other models the
app reports that it doesn't know the setting; set DATA MOD and DATA OFF MOD
to LAN on the radio yourself.

#### Data from a computer on the radio's USB port, voice through Icom Remote

Because the two settings are separate, data can come from a program on a
computer connected to the radio by USB (at the station, reached by remote
desktop) while voice goes through Icom Remote:

| Setting | Value | Used in |
|---|---|---|
| DATA MOD | USB | DATA modes (USB-D …): audio from the USB-connected computer |
| DATA OFF MOD | LAN | voice modes (USB, LSB, AM, FM): audio from Icom Remote |

In Icom Remote, untick **DATA MOD → LAN** and leave **DATA OFF MOD → LAN**
ticked (both take effect at the next connect). The app then leaves DATA MOD
on USB, switches DATA OFF MOD to LAN while connected and puts it back when
you disconnect.

- **Voice**: as usual, the armed space bar in LSB/USB/AM/FM.
- **Data**: select a DATA mode first (the **DATA** button in Icom Remote, or
  on the radio). This matters for programs that key the radio through the
  USB virtual serial port (RTS or DTR, set by the radio's *USB SEND* setting)
  instead of CI-V: they don't change the mode themselves. Keyed in a voice
  mode, the radio would take its audio from DATA OFF MOD, i.e. from the LAN,
  and transmit silence (in AM/FM a bare carrier).
- Icom Remote's own data PTT (PTT button, rigctl `T 1`) is refused in this
  setup ("DATA MOD is USB, not LAN"), so it can't key the radio while the
  data audio would come from the USB port.
- Switching the radio to a DATA mode while voice is keyed from Icom Remote
  unkeys it, so a data program that changes the mode can't take over a voice
  transmission. Stop the data program's transmissions while you work voice.
- Received audio goes to both the USB port and the LAN.

## rigctld (CAT control)

On by default, listening on `127.0.0.1:4532` (the standard hamlib port). Use
`0.0.0.0` to allow clients from other machines on your LAN.

**WSJT-X / JTDX / JS8Call** (Settings → Radio): Rig `Hamlib NET rigctl`,
Network Server `127.0.0.1:4532`, PTT Method `CAT`, Mode `Data/Pkt` or `USB`,
Split `None`. Click *Test CAT*, then *Test PTT*. Settings → Audio: input =
the receive cable, output = the transmit cable (see above).

**fldigi** (Configure → Rig Control → Hamlib): rig `Hamlib NET rigctl`, device
`127.0.0.1:4532`. Audio (Configure → Soundcard → Devices → PortAudio):
**Capture** = the *receive* cable (what you hear from the radio),
**Playback** = the *transmit* cable (what goes to the radio):

| fldigi | Windows | macOS | Linux |
|---|---|---|---|
| Capture (receive) | `CABLE Output` | `BlackHole 2ch` | `Monitor of IcomRX` / `IcomRX-Input` |
| Playback (transmit) | second cable's input | `BlackHole 16ch` | `IcomTX` |

**Command line:**

```
rigctl -m 2 -r 127.0.0.1:4532 F 14074000   # set frequency
rigctl -m 2 -r 127.0.0.1:4532 M PKTUSB 0   # USB-D
rigctl -m 2 -r 127.0.0.1:4532 f m l STRENGTH
```

Modes:

| rigctl | Radio |
|---|---|
| `PKTUSB`, `PKTLSB`, `PKTAM`, `PKTFM` | USB-D, LSB-D, AM-D, FM-D |
| `USB`, `LSB` | USB-D, LSB-D by default; plain USB/LSB if *Plain USB/LSB … selects USB-D/LSB-D* is unticked |
| `AM`, `FM` | AM, FM |
| others (CW, RTTY …) | refused (`RPRT -11`) |

The passband a client sends is ignored: the radio keeps its selected filter,
and `m` reports that filter's actual width. While the radio is disconnected,
reads return the last known values and changes fail with `RPRT -6`.

`T 1` is refused with `RPRT -9` when the transmit rules above aren't met
(the reason appears in the status bar). PTT keyed by a client is released if
that client disconnects.

## Remote control API

A local HTTP/JSON API for schedulers and scripts such as
[FLDigi Net Scheduler](https://github.com/jeffreycentex/fldigi-net-scheduler):
status, frequency/mode and recording, compatible with KiwiSDR Remote's API
(without `/connect`; connect in the window). It is control only: it has no
PTT of its own and doesn't affect transmitting from the window or rigctld;
an API tune is refused while the radio is on the air. Off by default: tick **Enable** in the
*Remote control API* box (127.0.0.1, port 7373).

```
python run_icomremote.py ctl status
python run_icomremote.py ctl tune --khz 7074 --mode USB-D
python run_icomremote.py ctl record-start --name "{date}_net" --max-seconds 3600
```

Details: [docs/API.md](docs/API.md).

## Connection test (command line)

`python -m icomremote.probe` connects without the GUI and prints what it
finds; useful when something doesn't work:

```
python -m icomremote.probe --host 192.168.1.50 -u USER -p PASSWORD --debug
```

| Option | Purpose |
|---|---|
| `--play "BlackHole 2ch"` | play the received audio to an output device |
| `--list-devices` | list output device names |
| `--wav rx.wav` | record the received audio |
| `--civ B6` | force the CI-V address |
| `--debug` | hex-dump every control and serial packet to `icomremote-probe.log` (password blanked) |
| `--bind-same-ports` | use local UDP ports 50001-50003 like kappanhang does |
| `--seconds N` | how long to stay connected (default 20) |

The password can also come from the `ICOM_PASSWORD` environment variable.

## Tests

```
python -m unittest discover -s tests -v
```

Headless; no radio or audio hardware needed. A fake radio
(`tests/fake_rsba1.py`) implements the radio side of all three streams over
real UDP sockets and follows the packet order captured from a real
IC-7300MK2. Covered:

- packet layouts checked byte for byte against kappanhang, plus a real IC-705
  capabilities capture
- login, bad password, radio in use by another client, clean disconnect
- CI-V reads, transceive broadcasts, frequency/mode/DATA/filter control
- rigctld: frequency, mode mapping, passband reporting, PTT and its release on client disconnect, dump_state
- remote control API: status, tune (refused while keyed, including a race test), recording names,
  max_seconds, gap filling, browser/Host/token protections, the ctl client, the window following API changes
- transmit: framing/resampling, one input for data and voice, the PTT rules and every unkey trigger,
  audio sent only while keyed, rapid press/release never leaving the radio keyed
- the armed space bar (GUI test): disarmed, armed in a text field, disarm while keyed
- DATA MOD: switch, restore, crash recovery from the record on disk, manual restore
- audio continuity, with dropped packets recovered by retransmit; audio timeout
- recording: WAV (and M4A when PyAV is installed), including starting before connecting
- RF power read/set, Po/SWR polled only while transmitting, TUNE cycle (success, failure,
  30 s timeout, abort on disconnect, refusals both ways)
- power: off/on from the app, off/on at the radio, connecting to a radio in standby,
  power-on ignored, 'Shutdown only' (network goes down), unkey and DATA MOD restore before off
- the GUI, run offscreen against the fake radio (needs PySide6; skipped otherwise)

## Standalone app

### Download a build

> **Unsigned code.** The builds are not signed with an Apple Developer ID or a
> Windows code-signing certificate. macOS and Windows will warn that the
> developer can't be verified, and you have to approve the first launch
> yourself (steps below). Only do that for a build you downloaded from this
> repository's own Releases or Actions pages. If in doubt, run from source
> instead (see [Install](#install)); it's the same code.

Tagged versions are under **Releases**, with a download for each OS. Every
push to `main` is also tested and built by GitHub Actions: **Actions** tab →
latest *test-and-build* run → **Artifacts** (`IcomRemote-macOS`,
`IcomRemote-Windows`, `IcomRemote-Linux`; kept 30 days).

- **macOS**: unzip and move *Icom Remote.app* to Applications. The app is not
  signed by Apple, so macOS blocks the first launch. On macOS 15 and later:
  open it once, dismiss the warning, then **System Settings → Privacy &
  Security → Open Anyway** (right-click → Open no longer bypasses this). Or
  remove the download flag: `xattr -dr com.apple.quarantine "/Applications/Icom Remote.app"`.
  Allow **Local Network** and **Microphone** access when asked: without them
  it can't reach the radio or read transmit audio. Because each build has a
  new ad-hoc signature, macOS may ask for these permissions again after an
  update. The macOS build is for Apple Silicon.
- **Windows**: unzip anywhere and run `Icom Remote.exe`.
  SmartScreen may warn about an unknown publisher: *More info* → *Run
  anyway*. Allow it through the firewall (private networks) when asked.
- **Linux**: `tar xzf IcomRemote-*-linux-*.tar.gz` and run
  `./"Icom Remote"`. Needs PortAudio (`libportaudio2`).

### Build it yourself

PyInstaller can't cross-compile: build each OS on that OS. In the virtual
environment:

```
python build.py
```

This installs PyInstaller, builds, runs the built app's self-check and
writes the archive to `dist/`. `python build.py --console` (Windows) keeps a
console window for troubleshooting.

### Logs and self-check

The app (built or run from Python) always writes its log to `icomremote.log`
in its settings folder (macOS `~/Library/Application Support/IcomRemote/`,
Windows `%APPDATA%\IcomRemote\`, Linux `~/.config/icomremote/`). The five
previous runs are kept as `icomremote.log.1` (last run) to `.5`, so after a
session that misbehaved and a restart, send `icomremote.log.1`. A `health:`
line every minute records received audio, sound-card and transmit-input
state. Start with `--debug` for packet dumps.

`--check` reports what the installation can do and exits (0 = OK):

```
"/Applications/Icom Remote.app/Contents/MacOS/Icom Remote" --check
python run_icomremote.py --check
```

```
Icom Remote 0.6.0, Python 3.12.8, Darwin arm64 (built app)
  OK    Qt GUI       PySide6 6.8.1, platform 'cocoa'
  OK    Audio        PortAudio V19.7.0-devel, 5 output / 4 input devices
  OK    Recording    PyAV 14.0.1, AAC encoder present
  OK    Resampler    linear (samplerate not installed; fine at 48 kHz)
  OK    Protocol     RS-BA1 / CI-V / rigctld modules load
RESULT: OK
```

## Licensing

GPL-3.0-or-later; see [LICENSE](LICENSE). The RS-BA1 protocol code is a port of
[kappanhang](https://github.com/nonoo/kappanhang) (MIT). See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Layout

```
run_icomremote.py   launcher / PyInstaller entry (icomremote.spec)
build.py            build + self-check + archive for the current OS
.github/workflows/  tests and builds on macOS / Windows / Linux
icomremote/
  gui.py            PySide6 main window
  session.py        connection lifecycle, control API (frequency/mode/filter), audio sinks
  rigctld.py        hamlib NET rigctl server
  service.py        tune / record / status for the API (the window records through it too)
  api.py            remote control HTTP/JSON API (docs/API.md)
  ctl.py            API client and 'ctl' command line
  state.py          modes, rigctl mode mapping, radio profiles and limits
  civ.py            CI-V framing, BCD, meter scales, filter widths, state decoder
  sinks.py          sound card (clock-drift locked) and WAV outputs
  resample.py       streaming resampler
  timerres.py       1 ms timer resolution on Windows while connected
  txaudio.py        transmit audio capture and 20 ms framing
  restore.py        on-disk record of radio settings to put back
  quicktune.py      quick-tune buttons file (quicktune.txt in the settings folder)
  probe.py          command-line connection test
  rsba1/            RS-BA1 protocol: packets, passcode, reorder buffer, UDP streams
tests/              fake radio + unit, end-to-end and GUI tests
docs/API.md         remote control API reference
```
