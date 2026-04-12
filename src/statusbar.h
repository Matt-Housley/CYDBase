#pragma once
#include "lgfx_config.h"

constexpr int STATUS_BAR_H = 20;   // pixels; content must start below this

// Draw the initial black bar background.  Call once after tft.init().
void statusbar_init();

// Refresh time and WiFi icon.  Call every frame from app_loop().
// Redraws only the sections that have changed to avoid flicker.
void statusbar_update();
