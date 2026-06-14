#pragma once

#include "balansun_psram.h"
#include "balansun_string_limits.h"

#include <Arduino.h>
#include <cctype>
#include <cstring>

/*
 * balansun_fixed_str.h — Bounded string buffers for essential runtime state.
 * No heap growth on assignment; large buffers may use PSRAM via BalansunPsramStr.
 */

template <size_t N>
struct BalansunStr {
  char data[N];

  BalansunStr() { data[0] = '\0'; }

  BalansunStr(const char *s) { assign(s); }

  const char *c_str() const { return data; }
  operator const char *() const { return data; }

  size_t length() const { return strnlen(data, N); }
  bool empty() const { return data[0] == '\0'; }
  size_t capacity() const { return N; }

  char operator[](size_t i) const { return (i < N) ? data[i] : '\0'; }

  void clear() { data[0] = '\0'; }

  void assign(const char *s) {
    if (!s) {
      clear();
      return;
    }
    strncpy(data, s, N - 1);
    data[N - 1] = '\0';
  }

  void assign(const String &s) { assign(s.c_str()); }

  BalansunStr &operator=(const char *s) {
    assign(s);
    return *this;
  }

  BalansunStr &operator=(const String &s) {
    assign(s);
    return *this;
  }

  bool equals(const char *other) const {
    if (!other) return empty();
    return strcmp(data, other) == 0;
  }

  bool operator==(const char *other) const { return equals(other); }
  bool operator!=(const char *other) const { return !equals(other); }

  bool operator==(const String &other) const { return equals(other.c_str()); }
  bool operator!=(const String &other) const { return !equals(other.c_str()); }

  void trim() {
    size_t start = 0;
    while (start < N - 1 && isspace(static_cast<unsigned char>(data[start]))) start++;
    size_t end = length();
    while (end > start && isspace(static_cast<unsigned char>(data[end - 1]))) end--;
    if (start > 0 || end < length()) {
      memmove(data, data + start, end - start);
      data[end - start] = '\0';
    }
  }

  int indexOf(char ch, int fromIndex = 0) const {
    if (fromIndex < 0) fromIndex = 0;
    for (size_t i = static_cast<size_t>(fromIndex); i < length(); i++) {
      if (data[i] == ch) return static_cast<int>(i);
    }
    return -1;
  }

  int indexOf(const char *needle, int fromIndex = 0) const {
    if (!needle || !needle[0]) return 0;
    if (fromIndex < 0) fromIndex = 0;
    const char *found = strstr(data + fromIndex, needle);
    return found ? static_cast<int>(found - data) : -1;
  }

  int indexOf(const String &needle, int fromIndex = 0) const { return indexOf(needle.c_str(), fromIndex); }

  long toInt() const { return strtol(data, nullptr, 10); }

  BalansunStr &operator+=(const char *s) {
    if (!s || !s[0]) return *this;
    const size_t used = length();
    if (used >= N - 1) return *this;
    strncat(data, s, N - 1 - used);
    data[N - 1] = '\0';
    return *this;
  }

  BalansunStr &operator+=(const String &s) { return operator+=(s.c_str()); }

  String toString() const { return String(data); }
};

template <size_t N>
inline String operator+(const char *lhs, const BalansunStr<N> &rhs) {
  return String(lhs) + rhs.c_str();
}

template <size_t N>
inline String operator+(const BalansunStr<N> &lhs, const char *rhs) {
  return String(lhs.c_str()) + rhs;
}

template <size_t N>
inline String operator+(const BalansunStr<N> &lhs, const String &rhs) {
  return String(lhs.c_str()) + rhs;
}

template <size_t N>
struct BalansunPsramStr {
  char *data = nullptr;
  bool allocated = false;

  BalansunPsramStr() = default;

  ~BalansunPsramStr() {
    if (data) {
      balansun_cache_free(data);
      data = nullptr;
      allocated = false;
    }
  }

  BalansunPsramStr(const BalansunPsramStr &) = delete;
  BalansunPsramStr &operator=(const BalansunPsramStr &) = delete;

  void ensure_allocated() {
    if (allocated) return;
    data = static_cast<char *>(balansun_cache_malloc(N));
    if (data) {
      data[0] = '\0';
    }
    allocated = true;
  }

  const char *c_str() const { return data ? data : ""; }
  operator const char *() const { return c_str(); }

  size_t length() const { return data ? strnlen(data, N) : 0; }
  bool empty() const { return !data || data[0] == '\0'; }
  size_t capacity() const { return N; }

  void clear() {
    ensure_allocated();
    if (data) data[0] = '\0';
  }

  void assign(const char *s) {
    ensure_allocated();
    if (!data) return;
    if (!s) {
      data[0] = '\0';
      return;
    }
    strncpy(data, s, N - 1);
    data[N - 1] = '\0';
  }

  void assign(const String &s) { assign(s.c_str()); }

  BalansunPsramStr &operator=(const char *s) {
    assign(s);
    return *this;
  }

  BalansunPsramStr &operator=(const String &s) {
    assign(s);
    return *this;
  }

  bool equals(const char *other) const {
    if (!other) return empty();
    return strcmp(c_str(), other) == 0;
  }

  bool operator==(const char *other) const { return equals(other); }
  bool operator!=(const char *other) const { return !equals(other); }

  bool operator==(const String &other) const { return equals(other.c_str()); }
  bool operator!=(const String &other) const { return !equals(other.c_str()); }

  void trim() {
    ensure_allocated();
    if (!data) return;
    size_t start = 0;
    while (start < N - 1 && isspace(static_cast<unsigned char>(data[start]))) start++;
    size_t end = length();
    while (end > start && isspace(static_cast<unsigned char>(data[end - 1]))) end--;
    if (start > 0 || end < length()) {
      memmove(data, data + start, end - start);
      data[end - start] = '\0';
    }
  }

  int indexOf(char ch, int fromIndex = 0) const {
    if (fromIndex < 0) fromIndex = 0;
    for (size_t i = static_cast<size_t>(fromIndex); i < length(); i++) {
      if (data[i] == ch) return static_cast<int>(i);
    }
    return -1;
  }

  int indexOf(const char *needle, int fromIndex = 0) const {
    if (!needle || !needle[0]) return 0;
    if (fromIndex < 0) fromIndex = 0;
    const char *found = strstr(c_str() + fromIndex, needle);
    return found ? static_cast<int>(found - c_str()) : -1;
  }

  int indexOf(const String &needle, int fromIndex = 0) const { return indexOf(needle.c_str(), fromIndex); }

  long toInt() const { return strtol(c_str(), nullptr, 10); }

  BalansunPsramStr &operator+=(const char *s) {
    if (!s || !s[0]) return *this;
    ensure_allocated();
    if (!data) return *this;
    const size_t used = length();
    if (used >= N - 1) return *this;
    strncat(data, s, N - 1 - used);
    data[N - 1] = '\0';
    return *this;
  }

  BalansunPsramStr &operator+=(const String &s) { return operator+=(s.c_str()); }

  String toString() const { return String(c_str()); }
};

/** Pre-allocate large PSRAM globals (call once early in boot). */
void balansun_fixed_str_boot_init();
