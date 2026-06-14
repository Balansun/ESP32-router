#pragma once

#include <Arduino.h>

#ifndef BALANSUN_REMOTE_DEBUG
#define BALANSUN_REMOTE_DEBUG 0
#endif

#if BALANSUN_REMOTE_DEBUG
#include <RemoteDebug.h>
#else

/** No-op stand-in when RemoteDebug is not linked (production wroom32 builds). */
class RmsDebugStub {
 public:
  void begin(const char *) {}
  void handle() {}
  template <typename T>
  void print(T) {}
  template <typename T>
  void println(T) {}
  void println() {}
};

#endif
