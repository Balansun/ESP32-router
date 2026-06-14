#include <balansun/platform/captive_portal.h>

namespace {

WebServer *g_captive_server = nullptr;

void handle_captive_portal_apple(void) {
  if (!g_captive_server) return;
  g_captive_server->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
}

void handle_captive_portal_android204(void) {
  if (!g_captive_server) return;
  g_captive_server->send(204, "text/plain", "");
}

void handle_captive_portal_windows(void) {
  if (!g_captive_server) return;
  g_captive_server->send(200, "text/plain", "Microsoft Connect Test");
}

}  // namespace

void balansun_platform_register_captive_portal_routes(WebServer &server) {
  g_captive_server = &server;
  server.on("/generate_204", HTTP_GET, handle_captive_portal_android204);
  server.on("/gen_204", HTTP_GET, handle_captive_portal_android204);
  server.on("/hotspot-detect.html", HTTP_GET, handle_captive_portal_apple);
  server.on("/library/test/success.html", HTTP_GET, handle_captive_portal_apple);
  server.on("/connecttest.txt", HTTP_GET, handle_captive_portal_windows);
  server.on("/ncsi.txt", HTTP_GET, handle_captive_portal_windows);
}
