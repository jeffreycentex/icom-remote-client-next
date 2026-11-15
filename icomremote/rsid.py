# SPDX-License-Identifier: GPL-3.0-or-later
"""Streaming RSID receiver for the four supported text modes.

Reed-Solomon symbols and FFT bucket matching adapted from fldigi rsid.cxx,
Copyright 2008–2012 Dave Freese W1HKJ, Stelios Bounanos M0GLD, and
John Douyere VK2ETA. Original code and notices are retained in third_party.
"""

from collections import deque

import numpy as np

from .resample import StreamResampler

RSID_RATE = 11025
SYMBOL_SAMPLES = 1024
# Code 6 announces the extended table, where MFSK64 is assigned code 620.
PRIMARY_CODES = {57: "MFSK16", 147: "MFSK32", 55: "Contestia 4-250", 6: "escape"}


def rsid_symbols(code):
    """Generate the 15 GF(16) symbols for an assigned RSID code."""
    symbols = [code >> 8, (code >> 4) & 15, code & 15] + [0] * 12

    def multiply(a, b):
        product = 0
        for _ in range(4):
            if b & 1:
                product ^= a
            b >>= 1
            a <<= 1
            if a & 16:
                a ^= 25  # x^4 + x^3 + 1, matching fldigi's Squares table
        return product

    for factor in (2, 4, 8, 9, 11, 15, 7, 14, 5, 10, 13, 3):
        for index in range(14, 0, -1):
            symbols[index] = symbols[index - 1] ^ multiply(symbols[index], factor)
        symbols[0] = multiply(symbols[0], factor)
    return np.array(symbols, dtype=np.uint8)


class RsidDetector:
    def __init__(self, input_rate):
        self._resampler = StreamResampler(input_rate, RSID_RATE)
        self._samples = np.empty(0, dtype=np.float32)
        self._window = np.hamming(2048)
        self._history = deque(maxlen=30)
        self._powers = deque(maxlen=30)
        self._primary = [(rsid_symbols(code), mode) for code, mode in PRIMARY_CODES.items()]
        self._extended = [(rsid_symbols(620), "MFSK64")]
        self._extended_until = 0
        self._clock = 0
        self._inhibit_until = 0
        self._bins = np.arange(10, int(3000 * 2048 / RSID_RATE) - 30)

    def process(self, samples):
        self._samples = np.concatenate((self._samples, self._resampler.process(samples)))
        detections = []
        while len(self._samples) >= 2048:
            power = abs(np.fft.rfft(self._samples[:2048] * self._window)) ** 2
            self._samples = self._samples[512:]
            self._clock += 512
            buckets = np.stack([power[self._bins + 2 * tone] for tone in range(16)])
            self._history.append(buckets.argmax(axis=0).astype(np.uint8))
            self._powers.append(power)
            if len(self._history) < 30 or self._clock < self._inhibit_until:
                continue
            history = np.asarray(self._history)[1::2]
            spectra = np.asarray(self._powers)[1::2]
            candidates = self._extended if self._clock < self._extended_until else self._primary
            strongest = None
            strongest_power = 0.0
            for symbols, mode in candidates:
                errors = (history != symbols[:, None]).sum(axis=0)
                eligible = np.flatnonzero(errors <= 1)
                if not len(eligible):
                    continue
                bins = self._bins[eligible][None, :] + 2 * symbols[:, None].astype(int)
                scores = spectra[np.arange(15)[:, None], bins].sum(axis=0)
                best = int(scores.argmax())
                if scores[best] > strongest_power:
                    strongest_power = float(scores[best])
                    strongest = (symbols, mode, int(eligible[best]))
            # Linear resampling can leave weak spectral images with the same
            # codeword. Select the strongest match across the entire passband.
            if strongest is None or strongest_power < max(1e-12, spectra.sum() * 0.02):
                continue
            symbols, mode, best = strongest
            estimates = []
            for spectrum, symbol in zip(spectra, symbols):
                expected = self._bins[best] + 2 * int(symbol)
                peak = expected - 1 + int(spectrum[expected - 1:expected + 2].argmax())
                left, middle, right = np.log(spectrum[peak - 1:peak + 2] + 1e-30)
                curvature = left - 2 * middle + right
                offset = 0.5 * (left - right) / curvature if curvature < 0 else 0
                estimates.append((peak + np.clip(offset, -0.5, 0.5) - 2 * int(symbol) + 14)
                                 * RSID_RATE / 2048)
            center = float(np.median(estimates))
            self._history.clear()
            self._powers.clear()
            if mode == "escape":
                self._extended_until = self._clock + 27 * SYMBOL_SAMPLES
            else:
                detections.append((mode, center))
                self._extended_until = 0
                self._inhibit_until = self._clock + 5 * SYMBOL_SAMPLES
        return detections
