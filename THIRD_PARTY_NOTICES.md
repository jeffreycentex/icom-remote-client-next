# Third-party notices

## kappanhang

`icomremote/rsba1/` is a Python port of the RS-BA1 protocol implementation in
[kappanhang](https://github.com/nonoo/kappanhang) (packet layouts, login
sequence, sequence/retransmit handling, passcode table). The tests use a radio
capture published in kappanhang's source.

```
MIT License

Copyright (c) 2020 Norbert Varga HA2NON, Akos Marton ES1AKOS

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

/Icom is a registered trademark of Icom Incorporated (Japan)/
```

The passcode algorithm is credited by kappanhang to W6EL.

## kiwisdr-remote

`icomremote/sinks.py` and `icomremote/resample.py` come from
[kiwisdr-remote](https://github.com/jeffreycentex/kiwisdr-remote)
(GPL-3.0-or-later, same author as this project).

## Digital reception and FLAMP

The receive-only native modem, Python decoder bridge, audio-file reader, RSID
detector, FLAMP assembler, and related tests were adapted from
https://github.com/jeffreycentex/kiwisdr-remote-client at digitalmodes / v0.1,
commit a28a9ad49066d64892ca7d6200066f6c39b2d997. Original file hashes and
integration changes are recorded in native/PORT_PROVENANCE.json.

The bundled modem sources are pinned to fldigi-web commit
739174ecb182874ba595bafcd7ed5e8b4842a39e, based on fldigi 4.2.13.
The complete copied source, authors, provenance manifests and GPL/LGPL
license texts are under third_party/fldigi-core. Its PROVENANCE.json
records upstream identities and hashes. These pinned files are preserved
byte-for-byte. FLAMP receive framing and encoding are adapted from FLAMP
by Dave Freese W1HKJ and Robert Stiles KK5VD, GPL-3.0-or-later.

The packaging spec bundles the applicable license texts and provenance.
Distribute the corresponding third_party/fldigi-core sources, native/
entry points and build_decoder.py with the application source for each
release. The wider upstream modem interfaces remain in the buildable
source, while the application's helper exposes only the four receive modes.
