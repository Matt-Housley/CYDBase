#ifdef LGFX_SDL
// ─────────────────────────────────────────────────────────────
//  Desktop networking – libcurl (ships with macOS, no Homebrew needed)
// ─────────────────────────────────────────────────────────────
#include "network.h"
#include <curl/curl.h>

void network_init(NetworkStatusCallback on_status)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (on_status) on_status("Network: Ready");
}

bool network_connected() { return true; }
int  network_rssi()      { return -45; }   // desktop: simulate strong signal

std::string network_ip() { return "127.0.0.1"; }

// ── internal helpers ─────────────────────────────────────────

static size_t write_cb(char* ptr, size_t size, size_t nmemb, std::string* out)
{
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

static HttpResponse run(CURL* curl, curl_slist* headers = nullptr)
{
    std::string body;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    HttpResponse r;
    if (curl_easy_perform(curl) == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        r.status_code = static_cast<int>(code);
        r.body        = std::move(body);
    }
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return r;
}

// ── public API ───────────────────────────────────────────────

HttpResponse http_get(const std::string& url)
{
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    return run(curl);
}

HttpResponse http_post(const std::string& url,
                       const std::string& body,
                       const std::string& content_type)
{
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

    curl_slist* headers = curl_slist_append(
        nullptr, ("Content-Type: " + content_type).c_str());
    return run(curl, headers);
}

#endif // LGFX_SDL
