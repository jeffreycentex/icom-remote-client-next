"""Stateful receive audio filters shared by playback and recording."""

import math
import numpy as np
from scipy import signal


class ReceiveDSP:
    def __init__(self, sample_rate=48000):
        self.sample_rate = sample_rate
        self.custom = False
        self.notch = False
        self.center = 1500.0
        self.width = 2400.0
        self.notch_center = 1000.0
        self.notch_width = 50.0
        self._sos = None
        self._zi = None

    def configure(self, *, custom, center, width, notch, notch_center, notch_width):
        values = (center, width, notch_center, notch_width)
        if not all(math.isfinite(v) for v in values):
            raise ValueError("Filter settings must be finite")
        nyquist = self.sample_rate / 2
        low, high = center - width / 2, center + width / 2
        if width <= 0 or not 0 < low < high < nyquist:
            raise ValueError("Bandpass edges must be above 0 Hz and below 24000 Hz")
        if not 0 < notch_width < 2 * min(notch_center, nyquist - notch_center):
            raise ValueError("Notch center and width must fit within 0–24000 Hz")
        sections = []
        if custom:
            sections.append(signal.butter(4, (low, high), btype="bandpass",
                                          fs=self.sample_rate, output="sos"))
        if notch:
            b, a = signal.iirnotch(notch_center, notch_center / notch_width,
                                   fs=self.sample_rate)
            sections.append(signal.tf2sos(b, a))
        self.custom, self.notch = bool(custom), bool(notch)
        self.center, self.width = center, width
        self.notch_center, self.notch_width = notch_center, notch_width
        self._sos = np.vstack(sections) if sections else None
        self.reset()

    def disable_custom(self):
        self.configure(custom=False, center=self.center, width=self.width,
                       notch=self.notch, notch_center=self.notch_center,
                       notch_width=self.notch_width)

    def reset(self):
        self._zi = np.zeros((len(self._sos), 2)) if self._sos is not None else None

    def process(self, samples):
        if self._sos is None or not len(samples):
            return samples
        output, self._zi = signal.sosfilt(self._sos, samples.astype(float), zi=self._zi)
        return np.clip(np.rint(output), -32768, 32767).astype(np.int16)
