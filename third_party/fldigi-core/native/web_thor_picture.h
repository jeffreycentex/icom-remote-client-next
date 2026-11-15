#pragma once
#include <cmath>
#include <vector>

class ThorPictureHistory {
    std::vector<double> samples;
    int writeIndex = 0;
    int sampleCount = 0;

public:
    void reset(int sampleRate) {
        samples.assign(2 * sampleRate, 0.0);
        writeIndex = 0;
        sampleCount = 0;
    }

    void push(double value) {
        if (samples.empty()) return;
        samples[writeIndex] = value;
        if (++writeIndex == static_cast<int>(samples.size())) writeIndex = 0;
        if (sampleCount < static_cast<int>(samples.size())) ++sampleCount;
    }

    int size() const {
        return sampleCount;
    }

    double sample(int index) const {
        const int capacity = static_cast<int>(samples.size());
        const int oldest = (writeIndex - sampleCount + capacity) % capacity;
        return samples[(oldest + index) % capacity];
    }

    int findMarker(int sampleRate, double frequency, int lookback = 0) const {
        const int window = sampleRate / 5;
        const double phaseStep = 2.0 * std::acos(-1.0) * frequency / sampleRate;
        const double stepCos = std::cos(phaseStep);
        const double stepSin = std::sin(phaseStep);
        double bestScore = 0.8;
        int bestOffset = -1;
        const int first = lookback > 0 && sampleCount > window + lookback ? sampleCount - window - lookback : 0;
        for (int offset = first; offset + window <= sampleCount; offset += 5) {
            double real = 0.0;
            double imaginary = 0.0;
            double energy = 0.0;
            double phaseCos = 1.0;
            double phaseSin = 0.0;
            for (int index = 0; index < window; index++) {
                const double value = sample(offset + index);
                real += value * phaseCos;
                imaginary -= value * phaseSin;
                energy += value * value;
                const double nextCos = phaseCos * stepCos - phaseSin * stepSin;
                phaseSin = phaseCos * stepSin + phaseSin * stepCos;
                phaseCos = nextCos;
            }
            const double score = 2.0 * (real * real + imaginary * imaginary) / (window * energy + 1e-12);
            if (score > bestScore) {
                bestScore = score;
                bestOffset = offset;
            }
        }
        return bestOffset;
    }
};
