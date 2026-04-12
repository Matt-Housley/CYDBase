#include "statusbar.h"
#include "network.h"
#include <ctime>      // time_t, struct tm – available on both platforms

// ── Platform-specific time retrieval ────────────────────────────────────────
#ifdef LGFX_SDL
static bool get_local_time(struct tm* t)
{
    time_t now = time(nullptr);
    struct tm* tmp = localtime(&now);
    if (!tmp) return false;
    *t = *tmp;
    return true;
}
#else
#include <time.h>     // ESP32 Arduino: getLocalTime
static bool get_local_time(struct tm* t)
{
    return getLocalTime(t, 5);   // 5 ms max wait
}
#endif

// ── Helpers ──────────────────────────────────────────────────────────────────

static int rssi_to_bars(int rssi)
{
    if (rssi >= -55) return 3;
    if (rssi >= -67) return 2;
    if (rssi >= -80) return 1;
    return 0;
}

// ── WiFi icon ────────────────────────────────────────────────────────────────
//
//  Three 1-bit bitmaps (13 × 12 px) that match the Material Symbols wifi,
//  wifi_2_bar and wifi_1_bar icons (viewBox 0 -960 960 960), scaled down to
//  fit the 20 px status bar.
//
//  Layout (rows within the 12 px icon, Y increases downward):
//    rows  0-3  outer arc  – shown at bars ≥ 3
//    rows  4-8  inner arc  – shown at bars ≥ 2
//    rows  9-11 dot        – shown at bars ≥ 1  (dimmed at bars == 0)
//
//  Bitmap encoding: MSB = leftmost pixel, stride = 2 bytes/row,
//  the rightmost 3 bits of the second byte in each row are unused padding.

static void draw_wifi_icon(int bars)
{
    // Outer arc: .XXXXXXXXXXX. / XX.........XX / X...........X / (gap)
    static const uint8_t outer_arc[] = {
        0x7F,0xF0,   // .XXXXXXXXXXX.   cols 1-11
        0xC0,0x18,   // XX.........XX   cols 0-1, 11-12
        0x80,0x08,   // X...........X   cols 0, 12
        0x00,0x00,   // (gap)
    };
    // Inner arc: ...XXXXXXX... / ..X.......X.. / .XX.......XX. / (gap×2)
    static const uint8_t inner_arc[] = {
        0x1F,0xC0,   // ...XXXXXXX...   cols 3-9
        0x20,0x20,   // ..X.......X..   cols 2, 10
        0x60,0x30,   // .XX.......XX.   cols 1-2, 10-11
        0x00,0x00,   // (gap)
        0x00,0x00,   // (gap)
    };
    // Dot: 3 × 3 px centred on the icon (cols 5-7)
    static const uint8_t dot_bmp[] = {
        0x07,0x00,   // .....XXX.....
        0x07,0x00,   // .....XXX.....
        0x07,0x00,   // .....XXX.....
    };

    constexpr int IW = 13;                       // icon pixel width
    constexpr int IH = 12;                       // icon pixel height (4+5+3)
    const int     IX = (WIDTH - 13) - IW / 2;   // left edge  ≈ WIDTH - 20
    const int     IY = (STATUS_BAR_H - IH) / 2; // top edge   = 4 px

    // Clear icon area (add 1 px margin each side)
    tft.fillRect(IX - 1, 0, IW + 2, STATUS_BAR_H, 0x000000);

    const uint32_t ON  = 0xFFFFFF;   // white – active bars
    const uint32_t OFF = 0x3A3A3A;   // dark grey – disconnected

    if (bars == 0) {
        // Full icon dimmed to signal no connection
        tft.drawBitmap(IX, IY,     outer_arc, IW, 4, OFF);
        tft.drawBitmap(IX, IY + 4, inner_arc, IW, 5, OFF);
        tft.drawBitmap(IX, IY + 9, dot_bmp,   IW, 3, OFF);
        return;
    }

    if (bars >= 3)
        tft.drawBitmap(IX, IY,     outer_arc, IW, 4, ON);
    if (bars >= 2)
        tft.drawBitmap(IX, IY + 4, inner_arc, IW, 5, ON);
    tft.drawBitmap(IX, IY + 9, dot_bmp, IW, 3, ON);
}

// ── Time display ─────────────────────────────────────────────────────────────

static void draw_time(const struct tm& ti, bool valid)
{
    tft.setTextDatum(lgfx::middle_left);
    tft.setTextSize(1);

    // Clear time region (enough for "HH:MM" at size 1)
    tft.fillRect(0, 0, 48, STATUS_BAR_H, 0x000000);

    const int Y = STATUS_BAR_H / 2;
    int x = 4;

    if (!valid) {
        tft.setTextColor(0x3A3A3A);
        tft.drawString("--:--", x, Y);
        return;
    }

    char buf[3];

    // Hours
    tft.setTextColor(0xFFFFFF);
    snprintf(buf, sizeof(buf), "%02d", ti.tm_hour);
    tft.drawString(buf, x, Y);
    x += tft.textWidth("00");

    // Colon – white on even seconds, black (hidden) on odd seconds
    bool colon_on = (ti.tm_sec % 2 == 0);
    tft.setTextColor(colon_on ? (uint32_t)0xFFFFFF : (uint32_t)0x000000);
    tft.drawString(":", x, Y);
    x += tft.textWidth(":");

    // Minutes
    tft.setTextColor(0xFFFFFF);
    snprintf(buf, sizeof(buf), "%02d", ti.tm_min);
    tft.drawString(buf, x, Y);
}

// ── Public API ────────────────────────────────────────────────────────────────

void statusbar_init()
{
    tft.fillRect(0, 0, WIDTH, STATUS_BAR_H, 0x000000);

    // Initial draw
    struct tm ti{};
    bool valid = get_local_time(&ti);
    draw_time(ti, valid);
    draw_wifi_icon(rssi_to_bars(network_rssi()));
}

void statusbar_update()
{
    // ── Time: redraw whenever the second changes ──────────────
    static int last_sec = -1;

    struct tm ti{};
    bool valid = get_local_time(&ti);

    int cur_sec = valid ? ti.tm_sec : -2;
    if (cur_sec != last_sec) {
        last_sec = cur_sec;
        draw_time(ti, valid);
    }

    // ── WiFi: poll every 5 s and redraw only when bars change ─
    static int    last_bars        = -1;
    static time_t last_wifi_check  =  0;

    time_t now = time(nullptr);
    if (now - last_wifi_check >= 5) {
        last_wifi_check = now;
        int bars = rssi_to_bars(network_rssi());
        if (bars != last_bars) {
            last_bars = bars;
            draw_wifi_icon(bars);
        }
    }
}
