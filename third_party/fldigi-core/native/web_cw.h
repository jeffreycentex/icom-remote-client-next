#pragma once
#include "cw.h"
#include <vector>

/** Complete the final Morse element without requiring a padded recording. */
class WebCw final : public cw {
    bool pendingFlush = false;
    bool needsWarmup = true;

public:
    int rx_process(const double* samples, int length) override {
        if (length > 0) {
            if (needsWarmup) {
                needsWarmup = false;
                // Settle the initial level estimate before timing the first mark.
                // A recording can start at full level without a quiet lead-in.
                std::vector<double> silence(samplerate, 0);
                cw::rx_process(silence.data(), silence.size());
            }
            pendingFlush = true;
        }
        return cw::rx_process(samples, length);
    }

    void rx_init() override {
        pendingFlush = false;
        needsWarmup = true;
        cw::rx_init();
    }

    void rx_flush() override {
        if (!pendingFlush) return;
        pendingFlush = false;
        // At 5 WPM, a word gap and the longest receive filter fit in two seconds.
        std::vector<double> silence(samplerate * 2, 0);
        cw::rx_process(silence.data(), silence.size());
    }
};
