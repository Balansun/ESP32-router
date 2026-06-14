// Vite SPA served at GET / (STA and deep links); PWA assets at fixed paths.
#include "web_ui.h"
#include <WebServer.h>

extern WebServer server;

#include "pageHtmlApp.h"
#include "pageHtmlPwaAssets.h"

static void send_no_cache() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
}

void WebUi_sendSpa() {
  server.sendHeader("Content-Encoding", "gzip");
  // SPA is embedded in firmware — avoid stale UI after OTA/serial flash (browser cache).
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send_P(200, "text/html", reinterpret_cast<const char *>(MainAppGz), MainAppGzLen);
}

void WebUi_sendManifest() {
  send_no_cache();
  server.send_P(200, "application/manifest+json", PwaManifestJson, PwaManifestJsonLen);
}

void WebUi_sendServiceWorker() {
  send_no_cache();
  server.send_P(200, "application/javascript", PwaSwJs, PwaSwJsLen);
}

void WebUi_sendPwaIcon192() {
  server.sendHeader("Cache-Control", "public, max-age=86400");
  server.send_P(200, "image/png", reinterpret_cast<const char *>(PwaIcon192), PwaIcon192Len);
}

void WebUi_sendPwaIcon512() {
  server.sendHeader("Cache-Control", "public, max-age=86400");
  server.send_P(200, "image/png", reinterpret_cast<const char *>(PwaIcon512), PwaIcon512Len);
}

bool WebUi_trySpaFallback(void) {
  if (server.method() != HTTP_GET) return false;
  String uri = server.uri();
  if (uri.startsWith("/api/")) return false;
  if (uri == F("/manifest.webmanifest") || uri == F("/sw.js") || uri == F("/pwa/icon-192.png") ||
      uri == F("/pwa/icon-512.png")) {
    return false;
  }
  WebUi_sendSpa();
  return true;
}
