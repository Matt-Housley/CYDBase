#pragma once
#include <string>
#include <functional>

// ─────────────────────────────────────────────────────────────
//  Common networking interface
//
//  CYD  : WiFiManager captive-portal, then HTTPClient
//  Desktop: libcurl over the OS connection
// ─────────────────────────────────────────────────────────────

// Optional callback invoked with human-readable status strings
// while network_init() is running (e.g. to update the display).
using NetworkStatusCallback = std::function<void(const std::string&)>;

struct HttpResponse {
    int         status_code = -1;   // HTTP status, or -1 on transport error
    std::string body;
    bool ok()   const { return status_code >= 200 && status_code < 300; }
    bool error() const { return status_code < 0; }
};

// Initialise networking.  Blocks until connected (or restarts on CYD).
void network_init(NetworkStatusCallback on_status = nullptr);

// True once a usable connection exists.
bool        network_connected();

// Local IP as a string ("127.0.0.1" on desktop).
std::string network_ip();

// WiFi signal strength in dBm (e.g. -45 = excellent, -80 = poor).
// Returns -45 on desktop (always treated as good signal).
int network_rssi();

// Blocking HTTP helpers.
HttpResponse http_get(const std::string& url);
HttpResponse http_post(const std::string& url,
                       const std::string& body,
                       const std::string& content_type = "application/json");
