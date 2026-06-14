#pragma once

#include <cstdint>
#include <cstring>
#include <string>

class String {
public:
  String() = default;
  String(const char *s) : data_(s ? s : "") {}
  String(const std::string &s) : data_(s) {}

  unsigned int length() const { return static_cast<unsigned int>(data_.size()); }
  bool startsWith(const char *prefix) const {
    if (!prefix) return false;
    return data_.compare(0, strlen(prefix), prefix) == 0;
  }
  String substring(int from) const {
    if (from < 0) from = 0;
    if (static_cast<size_t>(from) >= data_.size()) return String();
    return String(data_.substr(static_cast<size_t>(from)));
  }
  String substring(unsigned int from, unsigned int len) const {
    if (from >= data_.size()) return String();
    return String(data_.substr(from, len));
  }
  int indexOf(char c) const {
    const size_t p = data_.find(static_cast<char>(c));
    return p == std::string::npos ? -1 : static_cast<int>(p);
  }
  int indexOf(const char *needle) const {
    if (!needle) return -1;
    const size_t p = data_.find(needle);
    return p == std::string::npos ? -1 : static_cast<int>(p);
  }
  int indexOf(const String &needle) const { return indexOf(needle.c_str()); }
  float toFloat() const { return static_cast<float>(atof(data_.c_str())); }
  long toInt() const { return atol(data_.c_str()); }
  const char *c_str() const { return data_.c_str(); }

  String operator+(const String &other) const { return String(data_ + other.data_); }
  String &operator+=(const String &other) {
    data_ += other.data_;
    return *this;
  }
  bool operator==(const char *s) const { return data_ == (s ? s : ""); }

private:
  std::string data_;
};

inline int min(int a, int b) { return a < b ? a : b; }
