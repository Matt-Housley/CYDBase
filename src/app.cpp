#include "app.h"
#include "network.h"
#include "statusbar.h"

// ─────────────────────────────────────────────────────────────
//  Demo – gradient background, centred label, touch dots.
//  Replace / extend with your own application logic.
// ─────────────────────────────────────────────────────────────

static int32_t last_touch_x = -1;
static int32_t last_touch_y = -1;

// Clears a strip in the content area and prints a one-line status message.
static void show_status(const std::string& msg)
{
    const int content_mid = STATUS_BAR_H + (HEIGHT - STATUS_BAR_H) / 2;
    tft.fillRect(0, content_mid - 12, WIDTH, 24, 0x000000);
    tft.setTextDatum(lgfx::middle_center);
    tft.setTextSize(1);
    tft.setTextColor(0xFFFFFF);
    tft.drawString(msg.c_str(), WIDTH / 2, content_mid);
}

void app_setup()
{
    tft.setRotation(DISPLAY_ROTATION);
    tft.setBrightness(200);
    tft.fillScreen(0x000000);

    // Draw the status bar shell (black background) before network blocks.
    statusbar_init();

    // Network – status messages appear in the content area while connecting.
    network_init(show_status);

    // ── Main UI ──────────────────────────────────────────────

    // Gradient fills the area below the status bar.
    for (int y = STATUS_BAR_H; y < HEIGHT; y++) {
        uint8_t r = (uint8_t)((y - STATUS_BAR_H) * 255 / (HEIGHT - STATUS_BAR_H));
        uint8_t b = 255 - r;
        tft.drawFastHLine(0, y, WIDTH, tft.color888(r, 60, b));
    }

    const int content_mid = STATUS_BAR_H + (HEIGHT - STATUS_BAR_H) / 2;

    tft.setTextDatum(lgfx::middle_center);
    tft.setTextColor(0xFFFFFF);

    tft.setTextSize(2);
    tft.drawString("CYDBase ready", WIDTH / 2, content_mid - 20);

    tft.setTextSize(1);
    tft.drawString(network_ip().c_str(), WIDTH / 2, content_mid);
    tft.drawString("Touch the screen",  WIDTH / 2, content_mid + 16);

    // Refresh status bar on top of the gradient.
    statusbar_init();
}

void app_loop()
{
    statusbar_update();

    lgfx::touch_point_t tp;
    if (tft.getTouch(&tp)) {
        if (tp.x != last_touch_x || tp.y != last_touch_y) {
            last_touch_x = tp.x;
            last_touch_y = tp.y;
            // Only draw touch dots below the status bar.
            if (tp.y > STATUS_BAR_H)
                tft.fillCircle(tp.x, tp.y, 4, 0xFFE0);
        }
    }
}
