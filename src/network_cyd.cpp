#ifndef LGFX_SDL
// ─────────────────────────────────────────────────────────────
//  CYD networking – WiFiManager + HTTPClient
// ─────────────────────────────────────────────────────────────
#include "network.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

void network_init(NetworkStatusCallback on_status)
{
    if (on_status) on_status("Connecting to WiFi...");

    // Force STA mode and let the WiFi stack settle before handing off to
    // WiFiManager.  Without this the ESP32 can be left in a mixed or
    // AP-remnant state from the previous run, causing the saved-credential
    // connect attempt to fail non-deterministically.
    WiFi.mode(WIFI_STA);
    delay(100);

    WiFiManager wm;

    // Shown on the display while the captive portal is active.
    wm.setAPCallback([on_status](WiFiManager* mgr) {
        if (on_status)
            on_status("Connect to WiFi:\n" + std::string(mgr->getConfigPortalSSID().c_str()));
    });

    // Give the saved-network connection attempt a full 60 s before falling
    // back to the portal.  WiFiManager's default is short enough that a
    // slightly slow router response will cause it to silently open the portal
    // instead of retrying – which looks like "didn't connect" to the user.
    wm.setConnectTimeout(60);

    // Never time out the captive portal – block until the user configures WiFi.
    // A timed-out portal + ESP.restart() creates an infinite reboot loop when
    // NVS credentials are missing (e.g. after a clean flash).
    wm.setConfigPortalTimeout(0);

    // After the captive portal the radio is still in WIFI_AP_STA mode with an
    // active softAP.  Calling WiFi.begin() without first tearing that down
    // hangs the ESP32 WiFi driver indefinitely ("Connecting to NEW AP" freeze).
    // setCleanConnect(true) makes WiFiManager call WiFi.disconnect() and wait
    // for the radio to fully idle before every WiFi.begin() attempt.
    wm.setCleanConnect(true);

    wm.autoConnect("CYDBase-Setup");   // blocks until connected

    // Re-enable auto-reconnect so a transient signal drop doesn't require a
    // reboot to recover.
    WiFi.setAutoReconnect(true);

    if (on_status) on_status("WiFi: " + network_ip());

    // ── NTP time sync ─────────────────────────────────────────
#ifndef NTP_UTC_OFFSET
#define NTP_UTC_OFFSET 0   // override in platformio.ini: -DNTP_UTC_OFFSET=1 for UTC+1
#endif
    configTime(NTP_UTC_OFFSET * 3600L, 0, "pool.ntp.org", "time.nist.gov");
    if (on_status) on_status("Syncing time...");
    struct tm ti{};
    unsigned long t0 = millis();
    while (!getLocalTime(&ti, 200) && millis() - t0 < 10000)
        ;  // wait up to 10 s for first sync

    delay(400);
}

int network_rssi()
{
    return (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -100;
}

bool network_connected()
{
    return WiFi.status() == WL_CONNECTED;
}

std::string network_ip()
{
    return std::string(WiFi.localIP().toString().c_str());
}

HttpResponse http_get(const std::string& url)
{
    HTTPClient http;
    http.begin(url.c_str());
    HttpResponse r;
    r.status_code = http.GET();
    if (r.status_code > 0)
        r.body = http.getString().c_str();
    http.end();
    return r;
}

HttpResponse http_post(const std::string& url,
                       const std::string& body,
                       const std::string& content_type)
{
    HTTPClient http;
    http.begin(url.c_str());
    http.addHeader("Content-Type", content_type.c_str());
    HttpResponse r;
    r.status_code = http.POST(body.c_str());
    if (r.status_code > 0)
        r.body = http.getString().c_str();
    http.end();
    return r;
}

#endif // !LGFX_SDL
