"""Receive filters must reject unwanted tones and preserve streaming state."""
import unittest
import numpy as np
from icomremote.receive_dsp import ReceiveDSP


class TestReceiveDSP(unittest.TestCase):
    def configured(self, custom=True, notch=False):
        dsp = ReceiveDSP()
        dsp.configure(custom=custom, center=1500, width=2000,
                      notch=notch, notch_center=1000, notch_width=50)
        return dsp

    def amplitude(self, samples, frequency):
        t = np.arange(len(samples)) / 48000
        return abs(np.dot(samples, np.exp(-2j * np.pi * frequency * t))) * 2 / len(samples)

    def test_bandpass_and_notch_frequency_response(self):
        t = np.arange(96000) / 48000
        samples = sum(3000 * np.sin(2 * np.pi * f * t) for f in (100, 1000, 1800, 8000))
        output = self.configured(notch=True).process(samples.astype(np.int16))[48000:]
        self.assertGreater(self.amplitude(output, 1800), 2700)
        for frequency in (100, 1000, 8000):
            self.assertLess(self.amplitude(output, frequency), 30)

    def test_streamed_blocks_match_single_block(self):
        samples = np.random.default_rng(1).integers(-5000, 5000, 48000, dtype=np.int16)
        expected = self.configured(notch=True).process(samples)
        streaming = self.configured(notch=True)
        actual = np.concatenate([streaming.process(block) for block in np.array_split(samples, 100)])
        np.testing.assert_array_equal(actual, expected)
        bypass = ReceiveDSP()
        np.testing.assert_array_equal(bypass.process(samples), samples)

    def test_invalid_edges_do_not_change_active_filter(self):
        dsp = self.configured()
        previous = dsp._sos.copy()
        with self.assertRaises(ValueError):
            dsp.configure(custom=True, center=500, width=2000,
                          notch=False, notch_center=1000, notch_width=50)
        np.testing.assert_array_equal(dsp._sos, previous)
