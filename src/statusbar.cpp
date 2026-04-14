#include "statusbar.h"
#include "network.h"
#include <ctime>      // time_t, struct tm – available on both platforms

// Material Symbols WiFi icons – 16 × 16 px RGBA PNGs embedded as byte arrays.
// Export from fonts.google.com/icons (20 dp, weight 400, #FFFFFF), then:
//   xxd -i icon.png > src/icon_name.h
#include "wifi_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20.h"        // 3 bars (full)
#include "wifi_2_bar_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20.h"  // 2 bars
#include "wifi_1_bar_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20.h"  // 1 bar (dot)

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
//  Renders one of the three Material Symbols PNG assets (16 × 16 px, RGBA)
//  using LovyanGFX's built-in PNG decoder.  drawPng() alpha-blends each pixel
//  over whatever is already on screen, so the transparent background in the
//  exported icons composites cleanly over the black status bar.
//
//  bars == 0: area cleared, no icon drawn (clearly disconnected)
//  bars == 1: wifi_1_bar  (dot only)
//  bars == 2: wifi_2_bar  (dot + inner arc)
//  bars >= 3: wifi        (dot + both arcs)

static void draw_wifi_icon(int bars)
{
    constexpr int IW = 16;
    constexpr int IH = 16;
    const int     IX = WIDTH - IW - 2;           // 2 px right margin
    const int     IY = (STATUS_BAR_H - IH) / 2; // vertically centred in bar

    // Clear icon area
    tft.fillRect(IX - 1, 0, IW + 3, STATUS_BAR_H, 0x000000);

    const uint8_t* png = nullptr;
    uint32_t       len = 0;

    if (bars >= 3) {
        png = wifi_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20_png;
        len = wifi_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20_png_len;
    } else if (bars == 2) {
        png = wifi_2_bar_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20_png;
        len = wifi_2_bar_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20_png_len;
    } else if (bars == 1) {
        png = wifi_1_bar_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20_png;
        len = wifi_1_bar_16dp_FFFFFF_FILL0_wght400_GRAD0_opsz20_png_len;
    } else {
        return;   // bars == 0: leave area blank to indicate no connection
    }

    tft.drawPng(png, len, IX, IY);
}

// ── Time display ─────────────────────────────────────────────────────────────

static void draw_time(const struct tm& ti, bool valid)
{
    // FreeMono9pt7b glyphs are ~14 px tall – fits the 20 px bar with a small
    // margin.  setFont() is reset to nullptr at the end so callers that draw
    // text afterwards (e.g. show_status in app.cpp) keep the default font.
    tft.setFont(&lgfx::fonts::FreeMonoBold9pt7b);
    tft.setTextDatum(lgfx::middle_left);
    tft.setTextSize(1);

    // Clear time region – FreeMono9pt7b is ~11 px/char, 5 chars + 4 px margin
    tft.fillRect(0, 0, 72, STATUS_BAR_H, 0x000000);

    const int Y = STATUS_BAR_H / 2;
    int x = 4;

    if (!valid) {
        tft.setTextColor(0x3A3A3A);
        tft.drawString("--:--", x, Y);
        tft.setFont(nullptr);
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

    tft.setFont(nullptr);  // restore default font for subsequent draw calls
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
