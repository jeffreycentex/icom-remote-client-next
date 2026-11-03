"""Regression checks for control ownership, shutdown, and recording isolation."""
import asyncio
import http.client
import os
from pathlib import Path
import socket
import sys
import tempfile
import threading
import time
import unittest

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from icomremote import civ
from icomremote.api import ApiServer
from icomremote.commands import CommandQueue
from icomremote.rigctld import RigctldServer
from icomremote.service import ApiError, IcomService
from icomremote.session import ConnectionSpec, PttRefused, RadioSession
from icomremote.sinks import BufferedFileSink, WavSink
from fake_rsba1 import FakeRadio
from test_session import wait_for
from test_tx import FakeSource


class TestNetworkLifecycle(unittest.TestCase):
    def test_existing_api_connection_is_closed(self):
        api = ApiServer(object(), port=0)
        api.start()
        conn = http.client.HTTPConnection('127.0.0.1', api.port, timeout=2)
        try:
            conn.request('GET', '/api/v1')
            response = conn.getresponse()
            self.assertEqual(response.status, 200)
            response.read()
            api.stop()
            with self.assertRaises((OSError, http.client.HTTPException)):
                conn.request('GET', '/api/v1')
                conn.getresponse()
        finally:
            conn.close()
            api.stop()

    def test_existing_rig_connection_is_closed_and_ptt_released(self):
        class Controller:
            rig = civ.RigState(freq_hz=14074000)
            connected = True
            power = 'on'
            def __init__(self): self.owner = None
            def set_ptt(self, on, owner=None): self.owner = owner if on else None
            def release_ptt(self, owner):
                if self.owner is owner: self.owner = None
        controller = Controller()
        server = RigctldServer(controller, port=0)
        server.start()
        conn = socket.create_connection(('127.0.0.1', server.port), timeout=2)
        try:
            conn.sendall(b'T 1\n')
            self.assertEqual(conn.recv(100), b'RPRT 0\n')
            self.assertIsNotNone(controller.owner)
            server.stop()
            self.assertIsNone(controller.owner)
            try:
                conn.sendall(b'T 1\n')
                self.assertEqual(conn.recv(100), b'')
            except OSError:
                pass
            self.assertIsNone(controller.owner)
        finally:
            conn.close()
            server.stop()

    def test_http_post_cannot_smuggle_cat_commands(self):
        class Controller:
            rig = civ.RigState(freq_hz=14074000)
            def __init__(self): self.keyed = False
            def set_ptt(self, on, owner=None): self.keyed = on
            def release_ptt(self, owner): pass
        controller = Controller()
        server = RigctldServer(controller, port=0)
        server.start()
        try:
            with socket.create_connection(('127.0.0.1', server.port), timeout=2) as conn:
                conn.sendall(b'POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\nContent-Length: 4\r\n\r\nT 1\n')
                try:
                    self.assertEqual(conn.recv(100), b'')
                except ConnectionResetError:
                    pass
            self.assertFalse(controller.keyed)
        finally:
            server.stop()

    def test_rigctl_remote_binding_is_refused(self):
        with self.assertRaises(ValueError):
            RigctldServer(object(), host='0.0.0.0')

    def test_negative_content_length_is_rejected(self):
        api = ApiServer(object(), port=0)
        api.start()
        try:
            with socket.create_connection(('127.0.0.1', api.port), timeout=2) as conn:
                conn.sendall(b'POST /api/v1/tune HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: -1\r\n\r\n')
                self.assertIn(b'400', conn.recv(100).split(b'\r\n')[0])
        finally:
            api.stop()


class TestRecordingIsolation(unittest.TestCase):
    def test_rollover_never_overwrites(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            existing = path / 'rx_1.wav'
            existing.write_bytes(b'precious recording')
            sink = WavSink(str(path / 'rx.wav'))
            sink.set_input_rate(48000)
            sink.write(np.zeros(48, dtype=np.int16))
            sink.set_input_rate(44100)
            sink.close()
            self.assertEqual(existing.read_bytes(), b'precious recording')
            self.assertNotEqual(sink.current_path, str(existing))

    def test_existing_symlink_is_not_followed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            target = path / 'target'
            (path / 'rx.wav').symlink_to(target)
            sink = WavSink(str(path / 'rx.wav'))
            sink.set_input_rate(48000)
            sink.close()
            self.assertFalse(target.exists())

    def test_slow_writer_does_not_block_producer_and_close_drains(self):
        entered = threading.Event()
        release = threading.Event()
        class SlowSink:
            path = 'memory.wav'
            current_path = None
            def __init__(self): self.frames = 0; self.closed = False
            def set_input_rate(self, rate): pass
            def write(self, samples):
                entered.set()
                if not release.wait(5): raise RuntimeError('test writer stalled')
                self.frames += len(samples)
            def close(self): self.closed = True
        raw = SlowSink()
        sink = BufferedFileSink(raw)
        try:
            sink.set_input_rate(48000)
            sink.write(np.zeros(480, dtype=np.int16))
            self.assertTrue(entered.wait(2))
            start = time.monotonic()
            sink.write(np.zeros(480, dtype=np.int16))
            self.assertLess(time.monotonic() - start, .2)
            sink.MAX_BYTES = 2000
            with self.assertRaises(BufferError):
                sink.write(np.zeros(480, dtype=np.int16))
        finally:
            release.set()
            with self.assertRaises(RuntimeError):
                sink.close()
        self.assertEqual(raw.frames, 960)
        self.assertTrue(raw.closed)

    def test_writer_error_is_reported_and_container_closed(self):
        failure = threading.Event()
        class BrokenSink:
            path = "memory.wav"
            current_path = None
            closed = False
            def set_input_rate(self, rate): pass
            def write(self, samples): raise OSError("disk full")
            def close(self): self.closed = True
        raw = BrokenSink()
        sink = BufferedFileSink(raw, on_error=lambda exc: failure.set())
        sink.set_input_rate(48000)
        sink.write(np.zeros(480, dtype=np.int16))
        self.assertTrue(failure.wait(2))
        with self.assertRaisesRegex(RuntimeError, "disk full"):
            sink.close()
        self.assertTrue(raw.closed)

    def test_template_width_is_rejected_before_expansion(self):
        service = IcomService(start_ticker=False)
        try:
            with self.assertRaises(ApiError):
                service.recording_path_for(name='{date:0>1000000000}')
        finally:
            service.shutdown()


class TestControlOwnership(unittest.TestCase):
    def test_command_queue_has_one_owner(self):
        commands = CommandQueue()
        owners = []
        threads = [threading.Thread(target=lambda: commands.call(lambda: owners.append(threading.get_ident()))) for _ in range(8)]
        try:
            for thread in threads: thread.start()
            for thread in threads: thread.join(2)
            self.assertEqual(len(owners), 8)
            self.assertEqual(len(set(owners)), 1)
        finally:
            commands.close()

    def test_ptt_deadline_and_owner_cannot_be_replaced(self):
        with tempfile.TemporaryDirectory() as directory:
            radio = FakeRadio(mode=1, data=True).start()
            service = IcomService(state_dir=directory)
            session = service.radio
            owner, intruder = object(), object()
            session._tx_source_factory = FakeSource
            session.ptt_timeout = .5
            p = radio.ports
            try:
                service.connect(ConnectionSpec('127.0.0.1', 'beer', 'beerbeer', p['control'], p['serial'], p['audio']))
                service.set_tx_device('fake')
                self.assertTrue(wait_for(lambda: session.connected and session.data_mod == 5 and session._tx_source is not None))
                service.set_ptt(True, owner=owner)
                self.assertTrue(wait_for(lambda: radio.ptt))
                with self.assertRaises(PttRefused):
                    service.set_ptt(True, owner=intruder)
                with self.assertRaises(PttRefused):
                    service.set_frequency(7074000)
                with self.assertRaises(ApiError):
                    service.tune(7074000)
                for _ in range(3):
                    service.set_ptt(True, owner=owner)
                    time.sleep(.08)
                self.assertTrue(wait_for(lambda: not radio.ptt, timeout=.5))
                self.assertFalse(session.ptt)
            finally:
                service.shutdown()
                radio.stop()


if __name__ == '__main__':
    unittest.main()
