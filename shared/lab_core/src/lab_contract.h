#pragma once
// Builders for the serial contract's fixed events (AGENTS.md), pure so the exact lines are
// host-tested. lab_core::begin() and lab_core::error() send what these return.

#include <stddef.h>
#include <stdint.h>

#include "lab_json.h"

namespace lab {

struct BootInfo {
  const char* project;
  const char* fw;
  const char* board;
  const char* fastled;
  const char* core;
};

inline const char* error_line(char* buf, size_t cap, uint32_t t_ms, const char* code);

// {"t":..,"ev":"boot","proj":..,"fw":..,"board":..,"fastled":..,"core":..}; if it does not fit,
// an `err` "boot_overflow" instead, so a capture fails loudly rather than parsing a cut line.
inline const char* boot_line(char* buf, size_t cap, uint32_t t_ms, const BootInfo& b) {
  JsonLine line(buf, cap, t_ms, "boot");
  line.str("proj", b.project).str("fw", b.fw).str("board", b.board);
  line.str("fastled", b.fastled).str("core", b.core);
  line.finish();
  return line.ok() ? buf : error_line(buf, cap, t_ms, "boot_overflow");
}

// {"t":..,"ev":"err","code":..}; a code too long for `buf` becomes "log_overflow", so the
// line is always valid JSON.
inline const char* error_line(char* buf, size_t cap, uint32_t t_ms, const char* code) {
  JsonLine line(buf, cap, t_ms, "err");
  line.str("code", code);
  line.finish();
  if (line.ok()) return buf;
  JsonLine fallback(buf, cap, t_ms, "err");
  fallback.str("code", "log_overflow");
  return fallback.finish();
}

}  // namespace lab
