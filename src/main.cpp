#include "lgfx_config.h"
#include "app.h"

// ─────────────────────────────────────────────────────────────
//  Shared entry points (Arduino-style, used by both targets)
// ─────────────────────────────────────────────────────────────

#ifdef LGFX_SDL
// SDL: width x height, 2x pixel scaling for Retina displays
LGFX tft(WIDTH, HEIGHT, 2);
#else
LGFX tft;
#endif

void setup(void)
{
#ifndef LGFX_SDL
    Serial.begin(115200);
#endif
    tft.init();
    app_setup();
}

void loop(void)
{
    app_loop();
}

// ─────────────────────────────────────────────────────────────
//  Desktop (SDL2): provide main() and event loop
// ─────────────────────────────────────────────────────────────
#ifdef LGFX_SDL
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

static int user_func(bool* running)
{
    setup();
    do { loop(); } while (*running);
    return 0;
}

int main(int, char**)
{
    return lgfx::Panel_sdl::main(user_func);
}
#endif
// On Arduino/ESP32 the framework provides main() and calls setup()/loop().
