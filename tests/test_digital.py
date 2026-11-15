# SPDX-License-Identifier: GPL-3.0-or-later
"""Native known-text checks and the asynchronous service/audio bridge."""

import ctypes
import json
from pathlib import Path
import subprocess
import sys
import struct
import tempfile
import threading
import time
import unittest
import wave
from unittest.mock import patch

import numpy as np

from icomremote.digital import DataDecoderSink, decoder_path, validate_data_mode
from icomremote.session import RadioSession
from icomremote.resample import StreamResampler
from icomremote.service import IcomService
from icomremote.rsid import RsidDetector, rsid_symbols
from icomremote.audiofile import audio_file_blocks

ROOT = Path(__file__).resolve().parent.parent
SUFFIX = ".dll" if sys.platform == "win32" else ".dylib" if sys.platform == "darwin" else ".so"
TEST_LIBRARY = ROOT / "build" / "decoder" / ("test-modems" + SUFFIX)


def framed_audio(samples, enabled=False, threshold=20):
    packets = [struct.pack("<III", 0, enabled, threshold)]
    for start in range(0, len(samples), 512):
        block = np.asarray(samples[start:start + 512], dtype="<f4")
        packets.append(struct.pack("<I", len(block)) + block.tobytes())
    return b"".join(packets)


def rsid_burst(code, center):
    audio = []
    phase = 0.0
    for symbol in rsid_symbols(code):
        frequency = center + (int(symbol) - 7) * 11025 / 1024
        angles = phase + np.arange(1024) * 2 * np.pi * frequency / 11025
        audio.append(0.5 * np.sin(angles).astype(np.float32))
        phase = (angles[-1] + 2 * np.pi * frequency / 11025) % (2 * np.pi)
    return np.concatenate(audio)


class TestRsid(unittest.TestCase):
    def test_fallback_resampler_does_not_lock_to_a_weak_image(self):
        with patch("icomremote.resample.HAVE_LIBSAMPLERATE", False):
            samples = np.concatenate((np.zeros(3300, dtype=np.float32), rsid_burst(147, 1750),
                                      np.zeros(5500, dtype=np.float32)))
            downsampled = StreamResampler(11025, 8000).process(samples)
            detections = RsidDetector(8000).process(downsampled)
        self.assertEqual([item[0] for item in detections], ["MFSK32"])
        self.assertAlmostEqual(detections[0][1], 1750, delta=1)

    def test_assigned_codeword_and_streaming_detection(self):
        np.testing.assert_array_equal(rsid_symbols(57), [0, 1, 0, 14, 9, 15, 8, 1, 15, 7, 8, 14, 6, 6, 9])
        for code, mode in [(57, "MFSK16"), (147, "MFSK32"), (55, "Contestia 4-250"), (620, "MFSK64")]:
            with self.subTest(mode=mode):
                parts = [np.zeros(3300, dtype=np.float32)]
                if code == 620:
                    parts += [rsid_burst(6, 900), np.zeros(10240, dtype=np.float32)]
                parts += [rsid_burst(code, 900), np.zeros(5500, dtype=np.float32)]
                detector = RsidDetector(11025)
                detections = []
                samples = np.concatenate(parts)
                for offset in range(0, len(samples), 137):
                    detections += detector.process(samples[offset:offset + 137])
                self.assertEqual([item[0] for item in detections], [mode])
                self.assertAlmostEqual(detections[0][1], 900, delta=1)

    def test_noise_and_unannounced_extended_mode_are_ignored(self):
        detector = RsidDetector(11025)
        noise = np.random.default_rng(42).normal(0, 0.1, 11025 * 8).astype(np.float32)
        self.assertEqual(detector.process(noise), [])
        self.assertEqual(detector.process(rsid_burst(620, 1500)), [])


class TestDigitalControl(unittest.TestCase):
    def test_pcm_file_conversion_without_pyav(self):
        with tempfile.TemporaryDirectory() as directory, patch.dict(sys.modules, {"av": None}):
            for width, frame in [(1, bytes([160, 160])), (2, struct.pack("<hh", 8192, 8192)),
                                 (3, bytes([0, 0, 32, 0, 0, 32])),
                                 (4, struct.pack("<ii", 536870912, 536870912))]:
                path = Path(directory) / ("sample%d.wav" % width)
                with wave.open(str(path), "wb") as output:
                    output.setnchannels(2)
                    output.setsampwidth(width)
                    output.setframerate(11999)
                    output.writeframes(frame * 32)
                blocks = list(audio_file_blocks(path))
                self.assertEqual(blocks[0][0], 11999)
                np.testing.assert_array_equal(blocks[0][1], np.full(32, 8192, dtype=np.int16))

    def test_validate_before_start(self):
        service = IcomService(start_ticker=False)
        try:
            for mode, center in [("THOR16", 1500), ("MFSK32", float("nan")),
                                 ("MFSK64", 300), ("MFSK16", float("inf"))]:
                with self.assertRaises(ValueError):
                    service.start_data(mode, center)
                self.assertIsNone(service._data_decoder)
            self.assertEqual(validate_data_mode("MFSK32", 895), 895)
        finally:
            service.shutdown()

    def test_flamp_blocks_survive_service_decoder_restarts(self):
        from unittest.mock import Mock
        from tests.test_flamp import transfer_frames

        saved = threading.Event()
        def on_event(kind, value):
            if kind == "flamp_transfers" and any(item["path"] for item in value):
                saved.set()
        headers, blocks = transfer_frames(b"Retained across Data restarts" * 5)
        with tempfile.TemporaryDirectory() as directory:
            service = IcomService(on_event=on_event, state_dir=directory, start_ticker=False)
            try:
                service.configure_flamp(directory, True)
                with patch("icomremote.digital.DataDecoderSink", side_effect=lambda *a, **k: Mock()):
                    service.start_data("MFSK32")
                    first = service._data_session
                    service._on_data_event("data_raw", {"session": first, "data": b"".join(headers + blocks[1:])})
                    service.stop_data()
                    service.start_data("MFSK32")
                    service._on_data_event("data_raw", {"session": first, "data": blocks[0]})
                    self.assertFalse(saved.wait(0.2))
                    service._on_data_event("data_raw", {"session": service._data_session, "data": blocks[0]})
                    self.assertTrue(saved.wait(5))
            finally:
                service.shutdown()
            self.assertEqual((Path(directory) / "message.txt").read_bytes(),
                             b"Retained across Data restarts" * 5)

    def test_missing_engine_is_actionable(self):
        with patch("icomremote.digital.decoder_path", return_value=Path("/missing/decoder")):
            with self.assertRaisesRegex(RuntimeError, "build_decoder.py"):
                DataDecoderSink("MFSK32", 1500, lambda *event: None, 1)

    def test_raw_audio_reaches_decoder(self):
        from unittest.mock import Mock
        radio = RadioSession()
        sink = Mock()
        radio.set_data_decoder(sink)
        radio._connected = True
        from unittest.mock import patch
        raw = np.ones(16, dtype=np.int16)
        with patch.object(radio.receive_dsp, "process", return_value=np.zeros(16, dtype=np.int16)):
            radio._on_audio(raw.tobytes())
        np.testing.assert_array_equal(sink.write.call_args.args[0], raw)
        radio.shutdown()


@unittest.skipUnless(TEST_LIBRARY.is_file() and decoder_path().is_file(),
                     "run python build_decoder.py --test-library for native checks")
class TestNativeDigital(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.library = ctypes.CDLL(str(TEST_LIBRARY))
        cls.library.web_create.argtypes = [ctypes.c_int]
        cls.library.web_set_frequency.argtypes = [ctypes.c_double]
        cls.library.web_set_option.argtypes = [ctypes.c_int, ctypes.c_double]
        cls.library.web_tx_begin.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_double]
        cls.library.web_tx_step.argtypes = [ctypes.c_int]
        cls.library.web_tx_buffer.restype = ctypes.POINTER(ctypes.c_float)

    def signal(self, mode, center, message=None):
        library = self.library
        self.assertEqual(library.web_create(mode), 8000)
        library.web_set_frequency(center)
        library.web_set_option(0, 0)
        library.web_set_option(1, 0)
        message = b"CQ KIWI TEST 123\n" * 3 if message is None else message
        self.assertEqual(library.web_tx_begin(message, 0, 0), 8000)
        chunks = []
        for _ in range(10000):
            if library.web_tx_done():
                break
            count = library.web_tx_step(8192)
            self.assertGreaterEqual(count, 0)
            if count:
                chunks.append(np.ctypeslib.as_array(library.web_tx_buffer(), shape=(count,)).copy())
        else:
            self.fail("Test signal generator did not finish")
        return np.concatenate(chunks + [np.zeros(32000, dtype=np.float32)]).astype("<f4")

    def test_receiver_waits_for_connection_and_stops_cleanly(self):
        from tests.fake_rsba1 import FakeRadio
        from icomremote.session import ConnectionSpec
        events = []
        with tempfile.TemporaryDirectory() as directory:
            radio = FakeRadio().start()
            service = IcomService(on_event=lambda *event: events.append(event),
                                  state_dir=directory, start_ticker=False)
            service.configure_transmit(manage_data_mod=False, manage_data_off_mod=False)
            try:
                service.start_data("MFSK32")
                sink = service._data_decoder
                self.assertIsNone(sink._process)
                self.assertTrue(any(k == "data_state" and p["state"] == "waiting" for k, p in events))
                ports = radio.ports
                service.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer",
                                                ports["control"], ports["serial"], ports["audio"]))
                end = time.monotonic() + 5
                while not any(k == "data_state" and p["state"] == "receiving" for k, p in events) and time.monotonic() < end:
                    time.sleep(0.01)
                self.assertIsNone(sink._error)
                self.assertIsNotNone(sink._process)
                service.stop_data()
                self.assertFalse(sink._thread.is_alive())
                self.assertIsNone(service.radio._data_decoder)
            finally:
                service.shutdown()
                radio.stop()

    def test_four_mode_decodes(self):
        for name, mode in [("MFSK16", 39), ("MFSK32", 40), ("MFSK64", 45),
                           ("CONTESTIA4_250", 4)]:
            with self.subTest(mode=name):
                samples = self.signal(mode, 1750)
                result = subprocess.run([str(decoder_path()), name, "1750"],
                                        input=framed_audio(samples), capture_output=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue(result.stdout.startswith(b"READY\n"))
                events = [json.loads(line) for line in result.stdout.splitlines()[1:]]
                self.assertIn("CQ KIWI TEST 123", "".join(event.get("text", "") for event in events))

    def test_modem_squelch_suppresses_text(self):
        samples = self.signal(40, 1500)
        closed = subprocess.run([str(decoder_path()), "MFSK32", "1500"],
                                input=framed_audio(samples, True, 100), capture_output=True,
                                check=True, timeout=30)
        events = [json.loads(line) for line in closed.stdout.splitlines()[1:]]
        self.assertTrue(any(event["kind"] == "quality" for event in events))
        self.assertNotIn("CQ KIWI", "".join(event.get("text", "") for event in events))

    def test_binary_flamp_survives_modem_and_saves(self):
        from icomremote.flamp import ESCAPES
        from tests.test_flamp import transfer_frames
        original = bytes(range(256))
        reverse = {value: key for key, value in ESCAPES.items()}
        escaped = b"".join(b":" + bytes([reverse[byte]]) if byte in reverse else bytes([byte])
                           for byte in original)
        envelope = b"[b256:start]256\n" + escaped + b"\n[b256:end]"
        headers, blocks = transfer_frames(envelope, name="binary.dat", block_size=64)
        message = b"\n".join(headers + blocks) + b"\n"
        audio = self.signal(40, 1500, message=message * 2)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.wav"
            with wave.open(str(path), "wb") as output:
                output.setnchannels(1)
                output.setsampwidth(2)
                output.setframerate(8000)
                output.writeframes(np.rint(audio * 20000).astype("<i2").tobytes())
            service = IcomService(start_ticker=False)
            try:
                service.configure_flamp(str(Path(directory) / "received"), True)
                service.start_data("MFSK32", source=path)
                service._data_decoder._thread.join(timeout=20)
                self.assertFalse(service._data_decoder._thread.is_alive())
            finally:
                service.shutdown()
            saved = Path(directory) / "received" / "binary.dat"
            self.assertEqual(saved.read_bytes(), original)
            self.assertFalse((saved.parent / "binary_1.dat").exists())

    def test_file_source_auto_mode_and_completion(self):
        for code, native_mode in [(57, 39), (147, 40), (55, 4), (620, 45)]:
            with self.subTest(code=code), tempfile.TemporaryDirectory() as directory:
                prefix = [np.zeros(3300, dtype=np.float32)]
                if code == 620:
                    prefix += [rsid_burst(6, 1750), np.zeros(10240, dtype=np.float32)]
                prefix += [rsid_burst(code, 1750), np.zeros(5500, dtype=np.float32)]
                rsid = StreamResampler(11025, 8000).process(np.concatenate(prefix))
                audio = np.concatenate((rsid, self.signal(native_mode, 1750)))
                path = Path(directory) / "input.wav"
                with wave.open(str(path), "wb") as output:
                    output.setnchannels(1)
                    output.setsampwidth(2)
                    output.setframerate(8000)
                    output.writeframes(np.rint(audio * 20000).astype("<i2").tobytes())
                events = []
                service = IcomService(on_event=lambda *event: events.append(event), start_ticker=False)
                try:
                    service.start_data("Auto (RSID)", source=path)
                    sink = service._data_decoder
                    sink._thread.join(timeout=20)
                    self.assertFalse(sink._thread.is_alive())
                    self.assertIsNone(sink._error)
                    text = "".join(payload["text"] for kind, payload in events if kind == "data_text")
                    self.assertIn("CQ KIWI TEST 123", text)
                    self.assertTrue(any(kind == "data_state" and payload["state"] == "completed"
                                        for kind, payload in events), events)
                finally:
                    service.shutdown()

    def test_audio_file_decoding_can_be_cancelled(self):
        def endless_file(path):
            while True:
                yield 8000, np.zeros(4096, dtype=np.int16)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.wav"
            path.touch()
            with patch("icomremote.digital.audio_file_blocks", endless_file):
                sink = DataDecoderSink("MFSK32", 1500, lambda *event: None, 1, source=path)
                end = time.monotonic() + 5
                while sink._process is None and time.monotonic() < end:
                    time.sleep(0.01)
                process = sink._process
                self.assertIsNotNone(process)
                sink.close()
                self.assertFalse(sink._thread.is_alive())
                self.assertIsNotNone(process.poll())

    def test_streaming_resampling_and_close(self):
        samples = self.signal(40, 1500)
        upsample = StreamResampler(8000, 11999)
        pcm = np.clip(np.rint(upsample.process(samples) * 32768), -32768, 32767).astype(np.int16)
        received = []
        decoded = threading.Event()

        def on_event(kind, payload):
            if kind == "data_text":
                received.append(payload["text"])
                if "CQ KIWI TEST 123" in "".join(received):
                    decoded.set()

        sink = DataDecoderSink("MFSK32", 1500, on_event, 1)
        try:
            sink.set_input_rate(11999)
            for start in range(0, len(pcm), 32768):
                sink.write(pcm[start:start + 32768])
            self.assertTrue(decoded.wait(20), "".join(received))
            process = sink._process
            sink.set_squelch(True, 100)
            sink.write(np.zeros(4096, dtype=np.int16))
            self.assertIs(sink._process, process)
        finally:
            sink.close()
        self.assertFalse(sink.active)
        self.assertFalse(sink._thread.is_alive())

    def test_process_failure(self):
        events = []
        sink = DataDecoderSink("MFSK32", 1500, lambda *event: events.append(event), 1)
        try:
            sink.set_input_rate(12000)
            end = time.monotonic() + 5
            while sink._process is None and time.monotonic() < end:
                time.sleep(0.01)
            self.assertIsNotNone(sink._process)
            sink._process.kill()
            end = time.monotonic() + 5
            while sink.active and time.monotonic() < end:
                time.sleep(0.01)
            self.assertFalse(sink.active)
            self.assertTrue(any(kind == "data_state" and payload["state"] == "failed"
                                for kind, payload in events), events)
        finally:
            sink.close()

    def test_backpressure_is_reported(self):
        events = []
        with patch.object(DataDecoderSink, "_run", lambda self: self._stop.wait()):
            sink = DataDecoderSink("MFSK32", 1500, lambda *event: events.append(event), 1)
            try:
                for _ in range(64):
                    sink.write(np.zeros(512, dtype=np.int16))
                with self.assertRaisesRegex(RuntimeError, "cannot keep up"):
                    sink.write(np.zeros(512, dtype=np.int16))
                self.assertFalse(sink.active)
                self.assertEqual(events[-1][1]["state"], "failed")
            finally:
                sink.close()
