# PyInstaller spec. Use build.py, or directly:  pyinstaller --noconfirm icomremote.spec
# Produces a macOS app bundle or a single Windows/Linux executable.
# PyInstaller does not cross-compile: build on each target OS.
#
# ICOMREMOTE_CONSOLE=1 builds with a console window (Windows troubleshooting).
import os
import re
import sys

with open(os.path.join("icomremote", "__init__.py"), encoding="utf-8") as f:
    VERSION = re.search(r'__version__ = "([^"]+)"', f.read()).group(1)

hidden = []
for optional in ("samplerate", "av"):          # bundled when installed in the build environment
    try:
        __import__(optional)
        hidden.append(optional)
    except ImportError:
        print("NOTE: optional package %r not installed; the build will not include it" % optional)

console = os.environ.get("ICOMREMOTE_CONSOLE") == "1"
decoder_name = "icom-data-decoder.exe" if sys.platform == "win32" else "icom-data-decoder"
decoder = os.path.join(SPECPATH, "build", "decoder", decoder_name)
if not os.path.isfile(decoder):
    raise SystemExit("Run 'python build_decoder.py' before packaging")
licenses = os.path.join(SPECPATH, "third_party", "fldigi-core")
license_data = [
    (os.path.join(licenses, name), "licenses/fldigi-core")
    for name in ("COPYING", "COPYING.LESSER", "THIRD_PARTY_NOTICES.md", "README.md",
                 "PROVENANCE.json", "SOURCES.json")]
license_data.extend([
    (os.path.join(licenses, "vendor", "fldigi", "AUTHORS"), "licenses/fldigi-core"),
    (os.path.join(licenses, "vendor", "fldigi", "SOURCE.json"), "licenses/fldigi-core"),
    (os.path.join(licenses, "web", "licenses", "*"), "licenses/tiniconv"),
    (os.path.join(SPECPATH, "native", "PORT_PROVENANCE.json"), "licenses/fldigi-core")])


a = Analysis(["run_icomremote.py"], pathex=["."], hiddenimports=hidden,
             excludes=["tkinter", "PySide6.QtWebEngineCore", "PySide6.QtWebEngineWidgets",
                       "PySide6.Qt3DCore", "PySide6.QtQuick", "PySide6.QtQml"],
             binaries=[(decoder, "decoder")],
             datas=[("icomremote/assets/radio.svg", "icomremote/assets")] + license_data,
             noarchive=False)
pyz = PYZ(a.pure)
if sys.platform == "darwin":
    exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name="Icom Remote",
              console=console, upx=False)
    coll = COLLECT(exe, a.binaries, a.datas, name="Icom Remote", upx=False)
    app = BUNDLE(
        coll, name="Icom Remote.app", bundle_identifier="net.icomremote.app", version=VERSION,
        icon="icomremote/assets/radio.icns",
        info_plist={
            "CFBundleShortVersionString": VERSION,
            "CFBundleVersion": VERSION,
            "NSHighResolutionCapable": True,
            "LSMinimumSystemVersion": "11.0",
            # macOS asks for these the first time; without the keys it silently denies.
            "NSMicrophoneUsageDescription":
                "Transmit audio is read from the sound input you select (microphone or virtual cable).",
            "NSLocalNetworkUsageDescription":
                "Icom Remote connects to your radio's network server on the local network.",
        })
else:
    exe = EXE(pyz, a.scripts, a.binaries, a.datas, [], name="Icom Remote",
              icon="icomremote/assets/radio.ico" if sys.platform == "win32" else None,
              console=console or sys.platform != "win32", upx=False)
