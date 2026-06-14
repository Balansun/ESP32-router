"""Pre-build: WebServer POST body timeouts, cumulative read, Balansun plain-body store, client tuning."""
from pathlib import Path

Import("env")

MARKER = "BALANSUN_WEBSERVER_HTTP_PATCH"
PLAIN_MARKER = "BALANSUN_PLAIN_STORE"
EARLY_REJECT_MARKER = "BALANSUN_EARLY_BODY_REJECT"


def patch_webserver_h(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    if "HTTP_MAX_POST_WAIT 30000" in text:
        return
    repl = {
        "#define HTTP_MAX_DATA_WAIT 5000": "#define HTTP_MAX_DATA_WAIT 30000",
        "#define HTTP_MAX_POST_WAIT 5000": "#define HTTP_MAX_POST_WAIT 30000",
        "#define HTTP_MAX_SEND_WAIT 5000": "#define HTTP_MAX_SEND_WAIT 30000",
    }
    for old, new in repl.items():
        if old not in text:
            raise SystemExit(f"patch_webserver_http: missing {old!r} in {path}")
        text = text.replace(old, new, 1)
    path.write_text(text, encoding="utf-8")
    print(f"Patched WebServer timeouts ({MARKER}): {path}")


def _balansunize_patch_text(text: str) -> str:
    return text.replace("helio_api_", "balansun_api_").replace("HELIO_", "BALANSUN_")


def patch_read_bytes(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    if "READ_BYTES_DEADLINE" in text:
        path.write_text(_balansunize_patch_text(text), encoding="utf-8")
        return
    old = """static char* readBytesWithTimeout(WiFiClient& client, size_t maxLength, size_t& dataLength, int timeout_ms)
{
  char *buf = nullptr;
  dataLength = 0;
  while (dataLength < maxLength) {
    int tries = timeout_ms;
    size_t newLength;
    while (!(newLength = client.available()) && tries--) delay(1);
    if (!newLength) {
      break;
    }
    if (!buf) {
      buf = (char *) malloc(newLength + 1);
      if (!buf) {
        return nullptr;
      }
    }
    else {
      char* newBuf = (char *) realloc(buf, dataLength + newLength + 1);
      if (!newBuf) {
        free(buf);
        return nullptr;
      }
      buf = newBuf;
    }
    client.readBytes(buf + dataLength, newLength);
    dataLength += newLength;
    buf[dataLength] = '\\0';
  }
  return buf;
}"""
    new = """static char* readBytesWithTimeout(WiFiClient& client, size_t maxLength, size_t& dataLength, int timeout_ms)
{
  char *buf = nullptr;
  dataLength = 0;
  const unsigned long deadline = millis() + (unsigned long)timeout_ms; /* BALANSUN_READ_BYTES_DEADLINE */
  while (dataLength < maxLength) {
    if ((long)(deadline - millis()) <= 0) {
      break;
    }
    size_t newLength = client.available();
    if (!newLength) {
      delay(1);
      continue;
    }
    if (!buf) {
      buf = (char *) malloc(newLength + 1);
      if (!buf) {
        return nullptr;
      }
    }
    else {
      char* newBuf = (char *) realloc(buf, dataLength + newLength + 1);
      if (!newBuf) {
        free(buf);
        return nullptr;
      }
      buf = newBuf;
    }
    client.readBytes(buf + dataLength, newLength);
    dataLength += newLength;
    buf[dataLength] = '\\0';
  }
  return buf;
}"""
    if old not in text:
        raise SystemExit(f"patch_webserver_http: readBytesWithTimeout block not found in {path}")
    text = text.replace(old, new, 1)
    path.write_text(text, encoding="utf-8")
    print(f"Patched readBytesWithTimeout ({MARKER}): {path}")


def patch_plain_store(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    text = _balansunize_patch_text(text)

    if 'extern "C" void balansun_api_begin_request(void);' not in text:
        needle = 'extern "C" void balansun_api_store_plain_body(char *, size_t);'
        if needle not in text:
            text = text.replace(
                '#include "detail/mimetable.h"\n',
                '#include "detail/mimetable.h"\n\n'
                'extern "C" void balansun_api_begin_request(void);\n'
                'extern "C" void balansun_api_store_plain_body(char *, size_t);\n',
                1,
            )
        else:
            text = text.replace(
                needle,
                'extern "C" void balansun_api_begin_request(void);\n' + needle,
                1,
            )

    if "balansun_api_begin_request();" not in text:
        text = text.replace(
            "bool WebServer::_parseRequest(WiFiClient& client) {\n  // Read the first line of HTTP request",
            "bool WebServer::_parseRequest(WiFiClient& client) {\n  balansun_api_begin_request();\n  // Read the first line of HTTP request",
            1,
        )

    duplicate_partial = """        if(!isEncoded){
          /* BALANSUN_PLAIN_STORE */
          balansun_api_store_plain_body(plainBuf, plainLength);
          RequestArgument& arg = _currentArgs[_currentArgCount++];
          arg.key = F("plain");
          arg.value = String(plainBuf, plainLength);
          plainBuf = nullptr;
        }

        if (plainBuf) {
          free(plainBuf);
        }

        log_v("Plain length: %u", (unsigned)plainLength);"""

    memory_efficient_plain = """        if(!isEncoded){
          /* BALANSUN_PLAIN_STORE */
          balansun_api_store_plain_body(plainBuf, plainLength);
          RequestArgument& arg = _currentArgs[_currentArgCount++];
          arg.key = F("plain");
          arg.value = String();
          plainBuf = nullptr;
        }

        if (plainBuf) {
          free(plainBuf);
        }

        log_v("Plain length: %u", (unsigned)plainLength);"""

    if duplicate_partial in text:
        text = text.replace(duplicate_partial, memory_efficient_plain, 1)
        path.write_text(text, encoding="utf-8")
        print(f"Deduped plain-body arg copy ({PLAIN_MARKER}): {path}")
        return

    broken_partial = """        if(!isEncoded){
          //plain post json or other data
          RequestArgument& arg = _currentArgs[_currentArgCount++];
          arg.key = F("plain");
          balansun_api_store_plain_body(plainBuf, plainLength);
          arg.value = String();
        }

        log_v("Plain length: %u", (unsigned)plainLength);"""

    if broken_partial in text:
        text = text.replace(broken_partial, memory_efficient_plain, 1)
        path.write_text(text, encoding="utf-8")
        print(f"Fixed broken plain-body patch ({PLAIN_MARKER}): {path}")
        return

    if PLAIN_MARKER in text:
        path.write_text(text, encoding="utf-8")
        return

    old_plain = """        if(!isEncoded){
          //plain post json or other data
          RequestArgument& arg = _currentArgs[_currentArgCount++];
          arg.key = F("plain");
          arg.value = String(plainBuf);
        }

        log_v("Plain: %s", plainBuf);
        free(plainBuf);"""

    new_plain = """        if(!isEncoded){
          /* BALANSUN_PLAIN_STORE */
          balansun_api_store_plain_body(plainBuf, plainLength);
          RequestArgument& arg = _currentArgs[_currentArgCount++];
          arg.key = F("plain");
          arg.value = String();
          plainBuf = nullptr;
        }

        if (plainBuf) {
          free(plainBuf);
        }

        log_v("Plain length: %u", (unsigned)plainLength);"""

    if old_plain not in text:
        raise SystemExit(f"patch_webserver_http: plain body block not found in {path}")
    text = text.replace(old_plain, new_plain, 1)
    path.write_text(text, encoding="utf-8")
    print(f"Patched plain body store ({PLAIN_MARKER}): {path}")


def patch_early_body_reject(path: Path) -> None:
    text = _balansunize_patch_text(path.read_text(encoding="utf-8", errors="replace"))
    if EARLY_REJECT_MARKER in text:
        path.write_text(text, encoding="utf-8")
        return

    needle = 'extern "C" void balansun_api_store_plain_body(char *, size_t);'
    if "balansun_api_reject_oversized_before_read" not in text:
        if needle not in text:
            raise SystemExit(f"patch_webserver_http: plain store extern not found in {path}")
        text = text.replace(
            needle,
            'extern "C" bool balansun_api_reject_oversized_before_read(int httpMethod, const char *uri, '
            "size_t contentLength, WiFiClient &client);\n" + needle,
            1,
        )

    old = """    }

    if (!isForm){
      size_t plainLength;
      char* plainBuf = readBytesWithTimeout(client, _clientContentLength, plainLength, HTTP_MAX_POST_WAIT);"""
    new = """    }

    if (_clientContentLength > 0 &&
        balansun_api_reject_oversized_before_read((int)method, _currentUri.c_str(), (size_t)_clientContentLength, client)) { /* BALANSUN_EARLY_BODY_REJECT */
      return false;
    }

    if (!isForm){
      size_t plainLength;
      char* plainBuf = readBytesWithTimeout(client, _clientContentLength, plainLength, HTTP_MAX_POST_WAIT);"""
    if old not in text:
        raise SystemExit(f"patch_webserver_http: pre-read body block not found in {path}")
    text = text.replace(old, new, 1)
    path.write_text(text, encoding="utf-8")
    print(f"Patched early body reject ({EARLY_REJECT_MARKER}): {path}")


def patch_handle_client(path: Path) -> None:
    text = _balansunize_patch_text(path.read_text(encoding="utf-8", errors="replace"))

    flush_close_long = """          _handleRequest();
          /* BALANSUN_FLUSH_CLOSE: drain TX then release slot (no HC_WAIT_CLOSE) */
          if (_currentClient.connected()) {
            const unsigned long flushUntil = millis() + 10000UL;
            while (_currentClient.connected() && (long)(flushUntil - millis()) > 0) {
              _currentClient.flush();
              yield();
              delay(1);
            }
            _currentClient.stop();
          }
        }"""

    flush_close = """          _handleRequest();
          /* BALANSUN_FLUSH_CLOSE: release single client slot (large JSON drained in api_send_json) */
          if (_currentClient.connected()) {
            _currentClient.flush();
            _currentClient.stop();
          }
        }"""

    wait_close = """          _handleRequest();

          /* BALANSUN_WAIT_CLOSE: keep socket until client closes so large bodies flush */
          if (_currentClient.connected()) {
            _currentStatus = HC_WAIT_CLOSE;
            _statusChange = millis();
            keepCurrentClient = true;
          }
        }"""

    stock_close = """          _handleRequest();

// Fix for issue with Chrome based browsers: https://github.com/espressif/arduino-esp32/issues/3652
//           if (_currentClient.connected()) {
//             _currentStatus = HC_WAIT_CLOSE;
//             _statusChange = millis();
//             keepCurrentClient = true;
//           }
        }"""

    if flush_close_long in text:
        text = text.replace(flush_close_long, flush_close, 1)
        print(f"Downgraded long flush loop to immediate flush+stop ({MARKER}): {path}")
    elif "BALANSUN_WAIT_CLOSE" in text:
        text = text.replace(wait_close, flush_close, 1)
        print(f"Replaced HC_WAIT_CLOSE with flush+stop ({MARKER}): {path}")
    elif "BALANSUN_FLUSH_CLOSE" not in text:
        if stock_close in text:
            text = text.replace(stock_close, flush_close, 1)
            print(f"Patched handleClient flush+stop ({MARKER}): {path}")
        else:
            raise SystemExit(f"patch_webserver_http: handleClient send block not found in {path}")

    if "BALANSUN_CLIENT_TUNE" in text:
        path.write_text(text, encoding="utf-8")
        return
    old = """    log_v("New client: client.localIP()=%s", _currentClient.localIP().toString().c_str());

    _currentStatus = HC_WAIT_READ;"""
    new = """    log_v("New client: client.localIP()=%s", _currentClient.localIP().toString().c_str());
    _currentClient.setNoDelay(true); /* BALANSUN_CLIENT_TUNE */
    _currentClient.setTimeout(30);

    _currentStatus = HC_WAIT_READ;"""
    if old not in text:
        raise SystemExit(f"patch_webserver_http: new client block not found in {path}")
    text = text.replace(old, new, 1)
    path.write_text(text, encoding="utf-8")
    print(f"Patched handleClient client tune ({MARKER}): {path}")


def patch_client_write(path: Path) -> None:
    text = _balansunize_patch_text(path.read_text(encoding="utf-8", errors="replace"))
    new = """  virtual size_t _currentClientWrite(const char* b, size_t l) { /* BALANSUN_CLIENT_WRITE_ALL */
    size_t sent = 0;
    const unsigned long deadline = millis() + (unsigned long)HTTP_MAX_SEND_WAIT;
    while (sent < l && _currentClient.connected()) {
      const size_t n = _currentClient.write(b + sent, l - sent);
      if (n == 0) {
        _currentClient.flush();
        yield();
        delay(1);
        if ((long)(deadline - millis()) <= 0) {
          break;
        }
        continue;
      }
      sent += n;
    }
    return sent;
  }"""
    old_break = """  virtual size_t _currentClientWrite(const char* b, size_t l) { /* BALANSUN_CLIENT_WRITE_ALL */
    size_t sent = 0;
    while (sent < l) {
      const size_t n = _currentClient.write(b + sent, l - sent);
      if (n == 0) {
        break;
      }
      sent += n;
    }
    return sent;
  }"""
    if "while (sent < l && _currentClient.connected())" in text:
        return
    if "const unsigned long deadline = millis() + (unsigned long)HTTP_MAX_SEND_WAIT" in text:
        text = text.replace(
            "while (sent < l) {",
            "while (sent < l && _currentClient.connected()) {",
            1,
        )
        path.write_text(text, encoding="utf-8")
        print(f"Upgraded _currentClientWrite connected guard ({MARKER}): {path}")
        return
    if old_break in text:
        text = text.replace(old_break, new, 1)
        path.write_text(text, encoding="utf-8")
        print(f"Upgraded _currentClientWrite retry ({MARKER}): {path}")
        return
    old = (
        "  virtual size_t _currentClientWrite(const char* b, size_t l) "
        "{ return _currentClient.write( b, l ); }"
    )
    if old not in text:
        raise SystemExit(f"patch_webserver_http: _currentClientWrite not found in {path}")
    text = text.replace(old, new, 1)
    path.write_text(text, encoding="utf-8")
    print(f"Patched _currentClientWrite ({MARKER}): {path}")


def main() -> None:
    fw = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
    ws = fw / "libraries" / "WebServer" / "src"
    patch_webserver_h(ws / "WebServer.h")
    patch_read_bytes(ws / "Parsing.cpp")
    patch_plain_store(ws / "Parsing.cpp")
    patch_early_body_reject(ws / "Parsing.cpp")
    patch_handle_client(ws / "WebServer.cpp")
    patch_client_write(ws / "WebServer.h")


main()
