#pragma once
// Serial-contract encoder: builds {"t":<ms>,"ev":"<name>",...} into a caller-owned buffer.
// Pure C++ (no Arduino), so it is unit-tested on the host. Never writes past `cap`; on
// overflow the line is cut, ok() turns false, and the buffer stays NUL-terminated.

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace lab {

class JsonLine {
 public:
  JsonLine(char* buf, size_t cap, uint32_t t_ms, const char* ev) : buf_(buf), cap_(cap) {
    if (cap_ == 0) {
      ok_ = false;
      return;
    }
    buf_[0] = '\0';
    raw("{\"t\":");
    print("%lu", static_cast<unsigned long>(t_ms));
    str("ev", ev);
  }

  JsonLine& str(const char* key, const char* value) {
    this->key(key);
    put('"');
    for (const char* p = value ? value : ""; *p; ++p) escape(*p);
    put('"');
    return *this;
  }

  JsonLine& num(const char* key, long long value) {
    this->key(key);
    print("%lld", value);
    return *this;
  }

  // Non-finite values become null so the line stays valid JSON.
  JsonLine& real(const char* key, double value, int decimals = 2) {
    this->key(key);
    if (isfinite(value)) {
      print("%.*f", decimals, value);
    } else {
      raw("null");
    }
    return *this;
  }

  JsonLine& flag(const char* key, bool value) {
    this->key(key);
    raw(value ? "true" : "false");
    return *this;
  }

  // Closes the object and returns the line (without newline).
  const char* finish() {
    if (!closed_) {
      put('}');
      closed_ = true;
    }
    return buf_;
  }

  bool ok() const { return ok_; }
  size_t length() const { return len_; }

 private:
  void key(const char* k) {
    raw(",\"");
    raw(k);
    raw("\":");
  }

  void escape(char c) {
    switch (c) {
      case '"':
        raw("\\\"");
        break;
      case '\\':
        raw("\\\\");
        break;
      case '\n':
        raw("\\n");
        break;
      case '\r':
        raw("\\r");
        break;
      case '\t':
        raw("\\t");
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          print("\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
        } else {
          put(c);
        }
    }
  }

  void put(char c) {
    if (!ok_) return;
    if (len_ + 1 >= cap_) {
      ok_ = false;
      return;
    }
    buf_[len_++] = c;
    buf_[len_] = '\0';
  }

  void raw(const char* s) {
    while (*s) put(*s++);
  }

  template <typename... Args>
  void print(const char* fmt, Args... args) {
    char tmp[32];
    const int n = snprintf(tmp, sizeof tmp, fmt, args...);
    if (n < 0 || static_cast<size_t>(n) >= sizeof tmp) {
      ok_ = false;
      return;
    }
    raw(tmp);
  }

  char* buf_;
  size_t cap_;
  size_t len_ = 0;
  bool ok_ = true;
  bool closed_ = false;
};

}  // namespace lab
