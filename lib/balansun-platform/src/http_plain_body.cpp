#include <balansun/platform/http_plain_body.h>

#include <cstdlib>

namespace {
char *g_plainBody = nullptr;
size_t g_plainBodyLen = 0;
}  // namespace

extern "C" void balansun_api_begin_request(void) { api_http_clear_plain_body(); }

extern "C" void balansun_api_store_plain_body(char *data, size_t len) {
  if (g_plainBody != nullptr) {
    free(g_plainBody);
    g_plainBody = nullptr;
    g_plainBodyLen = 0;
  }
  if (data == nullptr || len == 0) {
    free(data);
    return;
  }
  g_plainBody = data;
  g_plainBodyLen = len;
}

void api_http_clear_plain_body(void) {
  if (g_plainBody != nullptr) {
    free(g_plainBody);
    g_plainBody = nullptr;
  }
  g_plainBodyLen = 0;
}

size_t api_http_plain_body_length(void) { return g_plainBodyLen; }

const char *api_http_plain_body_data(void) { return g_plainBody; }

bool api_http_take_plain_body(String &out) {
  if (g_plainBody == nullptr || g_plainBodyLen == 0) return false;
  out = String(g_plainBody, g_plainBodyLen);
  api_http_clear_plain_body();
  return true;
}

