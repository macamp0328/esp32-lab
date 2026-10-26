#pragma once
// Who is who, and what each board tells the others twenty times a second. The board with the
// matrix is m; the two S3s are a and b in MAC order. No board is in charge of the law: the M5
// only carries Miles' button presses (mode, scatter) and the claps it hears. Pure; host-tested.

#include <stdint.h>
#include <string.h>

#include "field.h"
#include "law.h"

namespace trio {

enum Slot : uint8_t { kM = 0, kA = 1, kB = 2 };
constexpr int kSlots = 3;
constexpr char kSlotName[kSlots] = {'m', 'a', 'b'};
constexpr uint8_t kRoleM = 'm';   // the board with the matrix, buttons, mic and TFT
constexpr uint8_t kRoleS = 's';   // an S3 Mini
constexpr uint8_t kBeatKind = 1;  // lab_link packet kind

enum class Mode : uint8_t { kUnison = 0, kWaltz = 1 };

#pragma pack(push, 1)
// How disturbed this board finds its link to one other (named by its MAC's last two bytes).
struct Heard {
  uint8_t tag[2];
  uint8_t stir;  // 0..255
};

struct Beat {
  uint8_t role;
  uint8_t mode;       // Mode; the M5's is the one everybody follows
  uint16_t cmd;       // the M5's command count: a new value is a mode change or a scatter
  uint8_t scatter;    // 1 when command `cmd` was a scatter
  uint8_t claps;      // the M5's clap count: a new value means fire now
  uint16_t phase;     // 0..65535 for 0..1
  uint16_t rate_mhz;  // phase speed, 1000 = once a second
  Heard heard[2];     // unused entries have stir 0 and tag 0
};
#pragma pack(pop)

inline uint16_t to_u16(float x01) {
  const float w = wrap01(x01);
  return w >= 1 ? 0 : static_cast<uint16_t>(w * 65536.0f);
}
inline float from_u16(uint16_t v) { return v / 65536.0f; }

// Microseconds from then to now on a wrapping 32-bit clock; 0 when then is later (a packet the
// radio stamped after the loop read the clock).
inline uint32_t elapsed_us(uint32_t now_us, uint32_t then_us) {
  const int32_t d = static_cast<int32_t>(now_us - then_us);
  return d > 0 ? static_cast<uint32_t>(d) : 0;
}

struct Member {
  bool present = false;
  uint8_t mac[6] = {};
  Beat beat = {};
  uint32_t at_us = 0;  // when its last beat arrived (this board's clock)
  Link link;           // this board hearing it

  // Its phase now, carried forward from the last beat at the speed it reported.
  float phase_at(uint32_t now_us) const {
    const float age_s = elapsed_us(now_us, at_us) * 1e-6f;
    return wrap01(from_u16(beat.phase) + beat.rate_mhz * 1e-3f * age_s);
  }
};

// This board and the others it has heard from.
class Trio {
 public:
  static constexpr int kOthers = 2;

  void set_self(const uint8_t mac[6], uint8_t role, bool swap_ab) {
    memcpy(self_mac_, mac, 6);
    self_role_ = role;
    swap_ = swap_ab;
  }

  // Records a beat from mac heard at rssi; nullptr when two others are already known (a fourth
  // board).
  Member* heard(const uint8_t mac[6], const Beat& b, uint32_t at_us, int8_t rssi) {
    Member* m = find(mac);
    for (int i = 0; m == nullptr && i < kOthers; ++i) {
      if (!others_[i].present) m = &others_[i];
    }
    if (m == nullptr) return nullptr;
    const float dt_s = m->present ? elapsed_us(at_us, m->at_us) * 1e-6f : 0;
    if (!m->present) *m = Member{};  // a new member starts its link from scratch
    m->link.push(rssi, dt_s);
    m->present = true;
    memcpy(m->mac, mac, 6);
    m->beat = b;
    m->at_us = at_us;
    return m;
  }

  void age(uint32_t now_us, uint32_t timeout_us) {
    for (Member& m : others_) {
      if (m.present && elapsed_us(now_us, m.at_us) > timeout_us) m.present = false;
    }
  }

  Member* find(const uint8_t mac[6]) {
    for (Member& m : others_) {
      if (m.present && memcmp(m.mac, mac, 6) == 0) return &m;
    }
    return nullptr;
  }

  const Member& other(int i) const { return others_[i]; }
  const uint8_t* self_mac() const { return self_mac_; }
  int count() const { return others_[0].present + others_[1].present; }

  // The M5, if it is someone else and present.
  const Member* m5() const {
    for (const Member& m : others_) {
      if (m.present && m.beat.role == kRoleM) return &m;
    }
    return nullptr;
  }

  // Corner of this board, or of other i: m for the M5, a/b for the S3s in MAC order. -1 if
  // the corner is taken twice (two M5s) or the S3 has no corner (three S3s).
  int slot_self() const { return slot(self_mac_, self_role_); }
  int slot_other(int i) const {
    return others_[i].present ? slot(others_[i].mac, others_[i].beat.role) : -1;
  }

 private:
  int slot(const uint8_t mac[6], uint8_t role) const {
    if (role == kRoleM) return kM;
    int lower = 0;  // S3s with a smaller MAC than this one
    if (self_role_ == kRoleS && memcmp(self_mac_, mac, 6) < 0) ++lower;
    for (const Member& m : others_) {
      if (m.present && m.beat.role == kRoleS && memcmp(m.mac, mac, 6) < 0) ++lower;
    }
    if (lower > 1) return -1;
    return (lower == 0) != swap_ ? kA : kB;
  }

  uint8_t self_mac_[6] = {};
  uint8_t self_role_ = kRoleS;
  bool swap_ = false;
  Member others_[kOthers];
};

// The last two MAC bytes, as beats name the links they hear.
inline bool tag_is(const uint8_t tag[2], const uint8_t mac[6]) {
  return tag[0] == mac[4] && tag[1] == mac[5];
}

// What this board puts in its beat about the links it hears.
inline void fill_heard(const Trio& t, Beat* b, float floor_db, float span_db) {
  memset(b->heard, 0, sizeof b->heard);
  int n = 0;
  for (int i = 0; i < Trio::kOthers; ++i) {
    const Member& m = t.other(i);
    if (!m.present) continue;
    b->heard[n].tag[0] = m.mac[4];
    b->heard[n].tag[1] = m.mac[5];
    b->heard[n].stir = static_cast<uint8_t>(m.link.stir(floor_db, span_db) * 255 + 0.5f);
    ++n;
  }
}

// Each edge's disturbance 0..1 as this board sees it: its own two links, and for the edge it
// is not on, what the two at its ends report. An edge takes the larger of its two directions.
inline void edges(const Trio& t, float floor_db, float span_db, float out[3]) {
  out[0] = out[1] = out[2] = 0;
  const int self = t.slot_self();
  for (int i = 0; i < Trio::kOthers; ++i) {
    const Member& m = t.other(i);
    const int s = t.slot_other(i);
    if (!m.present || s < 0 || self < 0) continue;
    float* e = &out[edge_of(self, s)];
    *e = fmaxf(*e, m.link.stir(floor_db, span_db));
    for (const Heard& h : m.beat.heard) {
      int k = -1;
      if (tag_is(h.tag, t.self_mac())) k = self;
      for (int j = 0; j < Trio::kOthers; ++j) {
        if (j != i && t.other(j).present && tag_is(h.tag, t.other(j).mac)) k = t.slot_other(j);
      }
      if (k < 0 || k == s) continue;
      float* f = &out[edge_of(s, k)];
      *f = fmaxf(*f, h.stir / 255.0f);
    }
  }
}

// An S3's view of the M5's commands: acts once per new command or clap, never on the first
// beat it hears (an S3 that boots late must not scatter the others).
struct Follow {
  bool seen = false;
  uint16_t cmd = 0;
  uint8_t claps = 0;

  struct Act {
    bool scatter = false;
    bool fire = false;
  };

  Act update(const Beat& m5) {
    Act act;
    if (seen) {
      act.scatter = m5.cmd != cmd && m5.scatter;
      act.fire = m5.claps != claps;
    }
    seen = true;
    cmd = m5.cmd;
    claps = m5.claps;
    return act;
  }
};

}  // namespace trio
