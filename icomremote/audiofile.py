# SPDX-License-Identifier: GPL-3.0-or-later
"""Stream a local recording as bounded mono int16 PCM blocks."""

from pathlib import Path
import wave

import numpy as np


def local_audio_path(path):
    source = Path(path).expanduser().resolve()
    if not source.is_file():
        raise ValueError("Choose an existing local audio file")
    return source


def audio_file_blocks(path):
    source = local_audio_path(path)
    try:
        import av
    except (ImportError, OSError):
        av = None
    if av is not None:
        with av.open(str(source)) as container:
            if not container.streams.audio:
                raise ValueError("The file contains no audio stream")
            resampler = av.AudioResampler(format="s16", layout="mono")
            for frame in container.decode(container.streams.audio[0]):
                for output in resampler.resample(frame):
                    samples = output.to_ndarray().reshape(-1)
                    for start in range(0, len(samples), 4096):
                        yield output.sample_rate, samples[start:start + 4096].copy()
            for output in resampler.resample(None):
                yield output.sample_rate, output.to_ndarray().reshape(-1).copy()
        return
    try:
        with wave.open(str(source), "rb") as recording:
            channels = recording.getnchannels()
            width = recording.getsampwidth()
            if not 1 <= channels <= 64 or width not in (1, 2, 3, 4):
                raise ValueError("Unsupported WAV channel count or sample format")
            while data := recording.readframes(4096):
                if width == 1:
                    pcm = (np.frombuffer(data, dtype=np.uint8).astype(np.int32) - 128) * 256
                elif width == 3:
                    raw = np.frombuffer(data, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
                    pcm = raw[:, 0] | raw[:, 1] << 8 | raw[:, 2] << 16
                    pcm = ((pcm ^ 0x800000) - 0x800000) >> 8
                else:
                    pcm = np.frombuffer(data, dtype="<i%d" % width)
                    if width == 4:
                        pcm = pcm >> 16
                mono = np.rint(pcm.reshape(-1, channels).mean(axis=1)).astype(np.int16)
                yield recording.getframerate(), mono
    except wave.Error as error:
        raise ValueError("Install PyAV to decode this audio format, or choose a PCM WAV file") from error
