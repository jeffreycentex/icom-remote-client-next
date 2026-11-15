// SPDX-License-Identifier: GPL-3.0-or-later
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
extern "C" {
int web_create(int mode);
void web_set_frequency(double frequency);
void web_set_option(int key, double value);
void web_process(const float* data, int length);
void web_flush();
const char* web_take_text();
const char* web_take_secondary();
const uint32_t* web_take_image_updates();
const unsigned char* web_take_raster();
double web_frequency();
double web_metric();
const unsigned char* kiwi_take_raw();
int kiwi_raw_size();
}
int main(int argc, char** argv) {
    if (argc != 3) {
        std::fputs("Usage: kiwi-data-decoder MODE CENTER_HZ\n", stderr);
        return 2;
    }
    int mode = 0;
    if (!std::strcmp(argv[1], "MFSK16")) mode = 39;
    else if (!std::strcmp(argv[1], "MFSK32")) mode = 40;
    else if (!std::strcmp(argv[1], "MFSK64")) mode = 45;
    else if (!std::strcmp(argv[1], "CONTESTIA4_250")) mode = 4;
    else { std::fputs("Unsupported mode\n", stderr); return 2; }
    errno = 0;
    char* end = nullptr;
    const double center = std::strtod(argv[2], &end);
    if (errno || !end || *end || !std::isfinite(center) || center <= 0 || center >= 4000) {
        std::fputs("Center must be between 0 and 4000 Hz\n", stderr); return 2;
    }
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    if (web_create(mode) != 8000) { std::fputs("Unexpected modem sample rate\n", stderr); return 3; }
    web_set_option(0, 0); // Disable AFC; the user-selected sweetspot controls tuning.
    web_set_option(1, 0); // Decode continuously; application provides on/off control.
    web_set_frequency(center);
    if (std::fabs(web_frequency() - center) > 0.01) {
        std::fputs("Sweetspot would clip the selected mode's audio passband\n", stderr); return 2;
    }
    // A bounded startup handshake lets the application report configuration errors.
    std::fputs("READY\n", stdout);
    if (std::fflush(stdout)) return 4;
    float samples[4096];
    uint32_t count = 0;
    size_t quality_samples = 0;
    auto emit_text = [](const char* text) {
        if (!*text) return;
        std::fputs("{\"kind\":\"text\",\"text\":\"", stdout);
        for (const unsigned char* c = reinterpret_cast<const unsigned char*>(text); *c; ++c) {
            if (*c == '"' || *c == '\\') {
                std::fputc('\\', stdout);
                std::fputc(*c, stdout);
            } else if (*c < 32) {
                std::fprintf(stdout, "\\u%04x", *c);
            } else {
                std::fputc(*c, stdout);
            }
        }
        std::fputs("\"}\n", stdout);
    };
    auto emit_raw = []() {
        const unsigned char* bytes = kiwi_take_raw();
        const int length = kiwi_raw_size();
        if (!length) return;
        std::fputs("{\"kind\":\"raw\",\"hex\":\"", stdout);
        for (int index = 0; index < length; ++index)
            std::fprintf(stdout, "%02x", bytes[index]);
        std::fputs("\"}\n", stdout);
    };
    while (std::fread(&count, sizeof(count), 1, stdin) == 1) {
        if (count == 0) {
            uint32_t options[2];
            if (std::fread(options, sizeof(uint32_t), 2, stdin) != 2 ||
                options[0] > 1 || options[1] > 100) return 2;
            web_set_option(1, options[0]);
            web_set_option(2, options[1]);
            continue;
        }
        if (count > 4096 || std::fread(samples, sizeof(float), count, stdin) != count) return 2;
        web_process(samples, static_cast<int>(count));
        emit_raw();
        emit_text(web_take_text());
        quality_samples += count;
        if (quality_samples >= 2000) {
            quality_samples %= 2000;
            double metric = web_metric();
            if (!std::isfinite(metric)) metric = 0;
            std::fprintf(stdout, "{\"kind\":\"quality\",\"value\":%.2f}\n",
                         std::max(0.0, std::min(100.0, metric)));
        }
        // Drain upstream channels that the text-only Data tab does not display.
        web_take_secondary();
        web_take_image_updates();
        web_take_raster();
        if (std::fflush(stdout)) return 4;
    }
    if (std::ferror(stdin)) return 4;
    web_flush();
    emit_raw();
    emit_text(web_take_text());
    return std::fflush(stdout) ? 4 : 0;
}
