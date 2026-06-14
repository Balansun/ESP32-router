/*
 * http_server.cpp — SPA at GET / and REST /api/v1/* (api_v1_routes.cpp).
 * See: /en/developer/ § Control interfaces
 */
#include "balansun_globals.h"
#include "balansun_forward.h"
#include "balansun_app.h"
void handleNotFound(void);

void handleRoot(void);

#include "api.h"
#include "web_ui.h"
#include "api_util.h"
#include "app_wifi_setup.h"

#include <balansun/platform/captive_portal.h>
#include <balansun/platform/http_listener.h>

static BalansunHttpListenerState g_http_listener;

static void balansun_http_send_wifi_portal_redirect(void) {
  server.sendHeader("Location", "/wifi");
  server.send(302, "text/plain", "");
}

static void handle_spa_deep_link(void) { WebUi_sendSpa(); }

static void handle_pwa_manifest(void) { WebUi_sendManifest(); }
static void handle_pwa_sw(void) { WebUi_sendServiceWorker(); }
static void handle_pwa_icon192(void) { WebUi_sendPwaIcon192(); }
static void handle_pwa_icon512(void) { WebUi_sendPwaIcon512(); }

void balansun_http_invalidate_binding(void) { balansun_platform_http_invalidate(server, g_http_listener); }

void balansun_http_ensure_listening(void) { balansun_platform_http_ensure_listening(server, g_http_listener); }

void Init_Server() {
  server.on("/", handleRoot);
  server.on("/manifest.webmanifest", HTTP_GET, handle_pwa_manifest);
  server.on("/sw.js", HTTP_GET, handle_pwa_sw);
  server.on("/pwa/icon-192.png", HTTP_GET, handle_pwa_icon192);
  server.on("/pwa/icon-512.png", HTTP_GET, handle_pwa_icon512);
  server.on("/wifi", HTTP_GET, handle_spa_deep_link);
  server.on("/login", HTTP_GET, handle_spa_deep_link);
  balansun_platform_register_captive_portal_routes(server);
  server.on("/redirect", HTTP_GET, balansun_http_send_wifi_portal_redirect);
  Init_ApiRoutes();
  server.onNotFound(handleNotFound);
  balansun_http_ensure_listening();
  Debug.println("HTTP server started");
}

void handleRoot() {
  const bool staUp =
      WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
  if (!staUp) {
    server.sendHeader("Location", "/wifi");
    server.send(302, "text/plain", "");
    return;
  }
  WebUi_sendSpa();
}

void handleNotFound() {
  if (api_try_handle_cors_preflight(server)) return;
  if (Api_handle_actions_config_subresource()) return;
  if (Api_handle_auth_tokens_subresource()) return;
  if (Api_handle_backup_subresource()) return;
  if (WebUi_trySpaFallback()) return;

  Debug.println(F("File not found"));
  String message;
  message.reserve(256);
  message = "File not found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
