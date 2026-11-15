"""Both receive devices must receive audio and fail independently."""
import unittest
from unittest.mock import Mock, patch
import numpy as np
from icomremote.session import RadioSession
from icomremote.sinks import audio_devices_conflict


class TestAudioOutputs(unittest.TestCase):
    def setUp(self):
        self.radio = RadioSession()
        self.factory = patch("icomremote.session.SoundCardSink", side_effect=self.sink)
        self.factory.start()

    @staticmethod
    def sink(key, gain):
        sink = Mock(device_key=key, gain=gain)
        return sink

    def tearDown(self):
        self.radio.shutdown()
        self.factory.stop()

    def test_fanout_gains_and_independent_failure(self):
        r = self.radio
        r.set_audio_device("BlackHole 2ch", .8)
        r.set_audio_device("Mac speakers", .4, output="monitor")
        first, second = r._sinks["card"], r._sinks["monitor"]
        samples = np.arange(480, dtype=np.int16)
        r._on_audio(samples.tobytes())
        np.testing.assert_array_equal(first.write.call_args.args[0], samples)
        np.testing.assert_array_equal(second.write.call_args.args[0], samples)
        self.assertEqual((first.gain, second.gain), (.8, .4))
        second.write.side_effect = RuntimeError("device disconnected")
        r._on_audio(samples.tobytes())
        self.assertNotIn("monitor", r._sinks)
        self.assertIs(r._sinks["card"], first)
        r._on_audio(samples.tobytes())
        self.assertEqual(first.write.call_count, 3)
        second.close.assert_called_once()

    def test_duplicate_and_transmit_routes_rejected_in_both_directions(self):
        r = self.radio
        r.set_audio_device("BlackHole 2ch")
        with self.assertRaises(ValueError):
            r.set_audio_device("BlackHole 2ch", output="monitor")
        r.set_audio_device("Mac speakers", output="monitor")
        with self.assertRaises(ValueError):
            r.set_tx_device("Mac speakers")
        r.set_tx_device("BlackHole 16ch")
        with self.assertRaises(ValueError):
            r.set_audio_device("BlackHole 16ch", output="monitor")
        self.assertEqual(r._sinks["monitor"].device_key, "Mac speakers")
        self.assertTrue(audio_devices_conflict("pulse-sink:IcomRX", "pulse-source:icomrx.monitor"))

    def test_watchdogs_are_independent(self):
        r = self.radio
        r.set_audio_device("first")
        r.set_audio_device("second", output="monitor")
        r._audio = Mock(rx_audio_packets=10)
        first, second = r._sinks["card"], r._sinks["monitor"]
        first.callbacks = second.callbacks = 1
        with patch("icomremote.session.time.monotonic", return_value=0):
            r._check_sink()
        r._audio.rx_audio_packets = 20
        first.callbacks = 2
        with patch("icomremote.session.time.monotonic", return_value=3):
            r._check_sink()
        self.assertIs(r._sinks["card"], first)
        self.assertIsNot(r._sinks["monitor"], second)
