#pragma once
struct WebStatus {
    double VIEWER_psksquelch {};
    int WKFSK_baud {};
    int WKFSK_mode {};
    int WKFSK_stopbits {};
    bool WK_online {};
    bool afconoff {};
    int carrier {};
    bool fsq_rx_abort {};
    bool ifkp_rx_abort {};
    bool nanoFSK_online {};
    bool psk8DCDShortFlag {};
    bool rtty_filter_changed {};
    bool shaped_rtty {};
    bool show_channels {};
    double sldrSquelchValue {};
    bool sqlonoff {};
    bool thordebug {};
    double txlevel {};
};
extern WebStatus progStatus;
