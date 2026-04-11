#include "app.h"

// ─────────────────────────────────────────────────────────────
//  Demo – colour gradient background, centred label, touch dots.
//  Replace with your own application logic.
// ─────────────────────────────────────────────────────────────

static int32_t last_touch_x = -1;
static int32_t last_touch_y = -1;

void app_setup()
{
    tft.setRotation(DISPLAY_ROTATION);
    tft.setBrightness(200);

    // Gradient background
    for (int y = 0; y < HEIGHT; y++) {
        uint8_t r = (uint8_t)(y * 255 / HEIGHT);
        uint8_t b = 255 - r;
        tft.drawFastHLine(0, y, WIDTH, tft.color888(r, 60, b));
    }

    // Centre label
    tft.setTextDatum(lgfx::middle_center);
    tft.setTextSize(2);
    tft.setTextColor(0xFFFFFF);                   // white
    tft.drawString("CYDBase ready", WIDTH / 2, HEIGHT / 2);
    tft.setTextSize(1);
    tft.drawString("Touch the screen", WIDTH / 2, HEIGHT / 2 + 24);
}

void app_loop()
{
    lgfx::touch_point_t tp;
    if (tft.getTouch(&tp)) {
        if (tp.x != last_touch_x || tp.y != last_touch_y) {
            last_touch_x = tp.x;
            last_touch_y = tp.y;
            tft.fillCircle(tp.x, tp.y, 4, 0xFFE0);   // yellow dot
        }
    }
}
