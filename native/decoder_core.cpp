// SPDX-License-Identifier: GPL-3.0-or-later
// Preserve received octets for FLAMP while retaining fldigi-web's UI text conversion.
#define put_rx_char upstream_put_rx_char
#include "../third_party/fldigi-core/native/core_api.cpp"
#undef put_rx_char

static std::string raw_received;
static std::string returned_raw;

void put_rx_char(unsigned int c, int style) {
    raw_received.push_back(static_cast<char>(c));
    upstream_put_rx_char(c, style);
}

extern "C" const unsigned char* kiwi_take_raw() {
    returned_raw.swap(raw_received);
    raw_received.clear();
    return reinterpret_cast<const unsigned char*>(returned_raw.data());
}

extern "C" int kiwi_raw_size() {
    return static_cast<int>(returned_raw.size());
}
