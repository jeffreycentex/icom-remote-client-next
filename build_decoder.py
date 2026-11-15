# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the bundled receive engine using a C/C++17 compiler (MinGW on Windows)."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
VENDOR = ROOT / "third_party" / "fldigi-core"
BUILD = ROOT / "build" / "decoder"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"))
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--test-library", action="store_true",
                        help="also build the native library used to generate test signals")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    sources = [VENDOR / name for name in json.loads((VENDOR / "SOURCES.json").read_text())]
    sources = [ROOT / "native/decoder_core.cpp" if path == VENDOR / "native/core_api.cpp"
               else path for path in sources]
    includes = ["-I" + str(ROOT / "native"), "-I" + str(VENDOR / "generated" / "include"),
                "-I" + str(VENDOR / "native")]
    windows = sys.platform == "win32"
    if windows:
        sources.append(VENDOR / "vendor/fldigi/src/compat/regex.c")
        includes.append("-I" + str(VENDOR / "vendor/fldigi/src/compat"))

    def compile_source(item):
        index, source = item
        obj = BUILD / ("source_%03d.o" % index)
        compiler = args.cc if source.suffix == ".c" else args.cxx
        flags = ["-std=c99"] if source.suffix == ".c" else ["-std=c++17"]
        if windows:
            # The upstream DSP headers use the POSIX math constants.
            flags.append("-D_USE_MATH_DEFINES")
            if source.suffix != ".c":
                flags.extend(["-include", str(ROOT / "native/compat.h")])
        if not windows:
            flags.append("-fPIC")
        subprocess.run([compiler, *flags, "-O2", "-pthread", *includes,
                        "-c", str(source), "-o", str(obj)], check=True)
        return str(obj)

    with ThreadPoolExecutor(max_workers=min(4, os.cpu_count() or 1)) as pool:
        objects = list(pool.map(compile_source, enumerate(sources)))
    executable = BUILD / ("icom-data-decoder.exe" if windows else "icom-data-decoder")
    link_flags = ["-static", "-static-libgcc", "-static-libstdc++"] if windows else []
    subprocess.run([args.cxx, "-std=c++17", "-O2", "-pthread", *objects,
                    str(ROOT / "native/digital_decoder.cpp"), *link_flags,
                    "-o", str(executable)], check=True)
    print(executable)
    if args.test_library:
        suffix = ".dll" if windows else ".dylib" if sys.platform == "darwin" else ".so"
        shared = BUILD / ("test-modems" + suffix)
        flags = ["-dynamiclib"] if sys.platform == "darwin" else ["-shared"]
        if windows:
            flags.append("-Wl,--export-all-symbols")
        subprocess.run([args.cxx, *flags, "-pthread", *objects, *link_flags,
                        "-o", str(shared)], check=True)
        print(shared)


if __name__ == "__main__":
    main()
