# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded receive-only PCM bridge to the bundled GPL modem engine."""

import json
import math
from pathlib import Path
import queue
import subprocess
import struct
import sys
import threading
import time

import numpy as np

from .resample import StreamResampler
from .audiofile import audio_file_blocks, local_audio_path
from .rsid import RsidDetector

# Nominal occupied audio widths. All four decoders consume mono 8000 Hz PCM.
DATA_MODES = {"Auto (RSID)": 0, "MFSK16": 250, "MFSK32": 500, "MFSK64": 1000, "Contestia 4-250": 250}
ENGINE_MODES = {"Contestia 4-250": "CONTESTIA4_250"}
DECODER_RATE = 8000


def decoder_path():
    filename = "icom-data-decoder.exe" if sys.platform == "win32" else "icom-data-decoder"
    if getattr(sys, "frozen", False):
        return Path(sys._MEIPASS) / "decoder" / filename
    return Path(__file__).resolve().parent.parent / "build" / "decoder" / filename


def validate_data_mode(mode, sweetspot):
    if mode not in DATA_MODES:
        raise ValueError("unsupported data mode")
    center = float(sweetspot)
    half_width = DATA_MODES[mode] / 2
    if not math.isfinite(center) or not half_width < center < 3000 - half_width:
        raise ValueError("sweetspot for %s must be above %g and below %g Hz"
                         % (mode, half_width, 3000 - half_width))
    return center


class DataDecoderSink:
    """The receiver only enqueues PCM. Decoder I/O and resampling run separately."""

    def __init__(self, mode, sweetspot, on_event, session, *,
                 squelch=True, threshold=20, source=None):
        self.mode = mode
        self.sweetspot = validate_data_mode(mode, sweetspot)
        self._decode_mode = None if mode == "Auto (RSID)" else mode
        self._center = self.sweetspot
        self.source = local_audio_path(source) if source is not None else None
        self.set_squelch(squelch, threshold)
        self._executable = decoder_path()
        if not self._executable.is_file():
            raise RuntimeError("Digital decoder is missing. Run python build_decoder.py "
                               "or install a build that includes the decoder.")
        self._emit = on_event
        self.session = session
        self._queue = queue.Queue(maxsize=64)
        self._stop = threading.Event()
        self._process_lock = threading.Lock()
        self._process = None
        self._error = None
        self._thread = threading.Thread(target=self._run, name="data-decoder", daemon=True)
        self._state("waiting", "Waiting for receiver audio")
        self._thread.start()

    def _state(self, state, message):
        self._emit("data_state", {"session": self.session, "state": state,
                                  "active": state in ("waiting", "receiving"),
                                  "mode": self._decode_mode or self.mode,
                                  "sweetspot_hz": self._center,
                                  "message": message})

    def set_squelch(self, enabled, threshold):
        if type(threshold) is not int or not 0 <= threshold <= 100:
            raise ValueError("squelch threshold must be between 0 and 100")
        self._squelch = (bool(enabled), threshold)

    def _put(self, item):
        if self._error:
            raise RuntimeError(self._error)
        if not self._stop.is_set():
            try:
                self._queue.put_nowait(item)
            except queue.Full:
                self._error = "Digital decoder cannot keep up with receiver audio"
                self._state("failed", self._error)
                raise RuntimeError(self._error)

    def set_input_rate(self, rate):
        rate = float(rate)
        if not math.isfinite(rate) or rate <= 0:
            raise ValueError("invalid decoder sample rate")
        # Recreate the modem even at the same rate to clear FEC after reconnects.
        self._put(("rate", rate))

    def write(self, samples):
        self._put(("audio", np.asarray(samples, dtype=np.int16).copy()))

    def _read_text(self, process, ready):
        pending = []
        pending_raw = bytearray()
        pending_size = 0
        last_text = last_quality = 0

        def flush_text():
            if pending_raw:
                self._emit("data_raw", {"session": self.session, "data": bytes(pending_raw)})
                pending_raw.clear()
            if pending:
                self._emit("data_text", {"session": self.session, "text": "".join(pending)})
                pending.clear()

        try:
            if process.stdout.readline(64) != b"READY\n":
                raise RuntimeError("Digital decoder failed to initialize")
            ready.set()
            while not self._stop.is_set():
                data = process.stdout.readline(65537)
                if not data:
                    break
                if len(data) > 65536:
                    raise RuntimeError("Digital decoder output exceeds its limit")
                event = json.loads(data)
                now = time.monotonic()
                if event["kind"] == "text":
                    pending.append(event["text"])
                    pending_size += len(event["text"])
                elif event["kind"] == "raw":
                    pending_raw.extend(bytes.fromhex(event["hex"]))
                if (pending or pending_raw) and (
                        self.source is None or now - last_text >= 0.05 or pending_size + len(pending_raw) >= 8192):
                    flush_text()
                    pending_size = 0
                    last_text = now
                if event["kind"] == "quality" and now - last_quality >= 0.1:
                    self._emit("data_quality", {"session": self.session, "value": event["value"]})
                    last_quality = now
        except (OSError, ValueError, RuntimeError) as error:
            if not self._stop.is_set():
                self._error = str(error)
        finally:
            flush_text()
            ready.set()

    def _launch(self):
        flags = subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
        process = subprocess.Popen(
            [str(self._executable), ENGINE_MODES.get(self._decode_mode, self._decode_mode), str(self._center)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            bufsize=0, creationflags=flags)
        with self._process_lock:
            self._process = process
        ready = threading.Event()
        reader = threading.Thread(target=self._read_text, args=(process, ready),
                                  name="data-text", daemon=True)
        reader.start()
        if self._stop.is_set():
            process.terminate()
        if not ready.wait(5) or self._error or process.poll() is not None:
            self._finish_process(process, reader)
            raise RuntimeError(self._error or "Digital decoder did not start")
        return process, reader

    def _finish_process(self, process, reader):
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=2)
        reader.join(timeout=2)
        process.stdin.close()
        process.stdout.close()
        with self._process_lock:
            if self._process is process:
                self._process = None

    def _run(self):
        process = reader = resampler = None
        detector = None
        applied_squelch = None
        try:
            for kind, value in self._inputs():
                if self._stop.is_set():
                    break
                if self._error:
                    raise RuntimeError(self._error)
                if kind == "idle":
                    if process is not None and process.poll() is not None:
                        raise RuntimeError("Digital decoder stopped unexpectedly")
                    continue
                if kind == "rate":
                    if process is not None:
                        self._finish_process(process, reader)
                        process = reader = None
                    resampler = StreamResampler(value, DECODER_RATE)
                    if self.mode == "Auto (RSID)":
                        self._decode_mode = None
                        detector = RsidDetector(value)
                        self._state("waiting", "Listening for RSID for a supported mode")
                    else:
                        process, reader = self._launch()
                        applied_squelch = None
                        self._state("receiving", "Decoding %s at %g Hz" % (self._decode_mode, self._center))
                elif kind == "audio":
                    normalized = value.astype(np.float32) / 32768
                    if detector is not None:
                        for mode, center in detector.process(normalized):
                            try:
                                validate_data_mode(mode, center)
                            except ValueError:
                                continue
                            if process is not None:
                                self._finish_process(process, reader)
                                process = reader = None
                            self._decode_mode, self._center = mode, center
                            process, reader = self._launch()
                            applied_squelch = None
                            self._state("receiving", "RSID: %s at %.1f Hz" % (mode, center))
                    samples = resampler.process(normalized)
                    if process is not None:
                        if self._squelch != applied_squelch:
                            enabled, threshold = self._squelch
                            self._write_pipe(process, struct.pack("<III", 0, enabled, threshold))
                            applied_squelch = self._squelch
                        self._send_audio(process, samples)
                elif kind == "eof":
                    if process is not None:
                        self._send_audio(process, np.zeros(DECODER_RATE * 4, dtype=np.float32))
                        process.stdin.close()
                        if process.wait(timeout=10):
                            raise RuntimeError("Digital decoder failed while finishing the file")
                        reader.join(timeout=5)
                    message = ("File decoding complete: %s at %.1f Hz" % (self._decode_mode, self._center)
                               if self._decode_mode else "File complete: no supported RSID detected")
                    self._state("completed", message)
        except Exception as error:
            if not self._stop.is_set():
                self._error = str(error)
                self._state("failed", self._error)
        finally:
            if process is not None:
                self._finish_process(process, reader)
            self._stop.set()

    def _inputs(self):
        if self.source is not None:
            rate = None
            for new_rate, samples in audio_file_blocks(self.source):
                if self._stop.is_set():
                    return
                if new_rate != rate:
                    rate = new_rate
                    yield "rate", rate
                yield "audio", samples
            if rate is None:
                raise ValueError("The audio file contains no samples")
            yield "eof", None
            return
        while not self._stop.is_set():
            try:
                yield self._queue.get(timeout=0.1)
            except queue.Empty:
                yield "idle", None

    def _write_pipe(self, process, data):
        data = memoryview(data)
        while data and not self._stop.is_set():
            written = process.stdin.write(data)
            if not written:
                raise RuntimeError("Digital decoder input closed")
            data = data[written:]

    def _send_audio(self, process, samples):
        for start in range(0, len(samples), 512):
            block = np.ascontiguousarray(samples[start:start + 512], dtype="<f4")
            self._write_pipe(process, struct.pack("<I", len(block)) + block.tobytes())

    @property
    def active(self):
        return not self._stop.is_set() and self._error is None

    def close(self):
        self._stop.set()
        with self._process_lock:
            process = self._process
        if process is not None and process.poll() is None:
            try:
                process.terminate()
            except OSError:
                pass
        self._thread.join(timeout=8)
        if self._thread.is_alive():
            raise RuntimeError("Digital decoder worker did not stop")
        if not self._error:
            self._state("stopped", "Decoding stopped")
