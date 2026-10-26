#pragma once
// lab_link's pure parts: the packet header, the peer table (loss from sequence gaps, RSSI, RTT)
// and the ping/pong bodies. Host-tested in shared/tests/test/test_link; the radio is lab_link.h.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace lab_link {

constexpr uint16_t kMagic = 0x4B4C;  // "LK" on the wire
constexpr uint8_t kVersion = 1;
constexpr size_t kPacketMax = 250;  // ESP-NOW v1 payload limit
constexpr uint8_t kMaxPeers = 8;
constexpr uint16_t kSeqJump = 1000;  // a larger gap is a reboot, not loss

// Kinds from 0xF0 up are lab_link's own; projects use 0..0xEF.
constexpr uint8_t kPing = 0xF0;
constexpr uint8_t kPong = 0xF1;

#pragma pack(push, 1)
struct Header {
  uint16_t magic;
  uint8_t version;
  uint8_t kind;
  uint16_t project;  // project_hash(LAB_PROJECT): another project's packets are dropped
  uint16_t seq;      // per sender, every packet
  uint32_t sent_us;  // sender's clock (esp_timer), low 32 bits
};

// Every board answers a broadcast ping with a broadcast pong naming the pinger.
struct Pong {
  uint8_t to[6];
  uint32_t ping_sent_us;  // the pinger's own clock, echoed
  uint32_t hold_us;       // how long the ping sat here before this pong left
};
#pragma pack(pop)

constexpr size_t kHeaderSize = sizeof(Header);
constexpr size_t kBodyMax = kPacketMax - kHeaderSize;

// FNV-1a folded to 16 bits.
constexpr uint16_t project_hash(const char* s) {
  uint32_t h = 2166136261u;
  for (; *s; ++s) h = (h ^ static_cast<uint8_t>(*s)) * 16777619u;
  return static_cast<uint16_t>(h ^ (h >> 16));
}

// Header then body into out; 0 if it does not fit.
inline size_t encode(uint8_t* out, size_t cap, const Header& h, const void* body, size_t len) {
  if (kHeaderSize + len > cap || kHeaderSize + len > kPacketMax) return 0;
  memcpy(out, &h, kHeaderSize);
  if (len) memcpy(out + kHeaderSize, body, len);
  return kHeaderSize + len;
}

// False unless `in` is a lab_link packet of `project` in this version.
inline bool decode(const uint8_t* in, size_t len, uint16_t project, Header* h, const uint8_t** body,
                   size_t* body_len) {
  if (len < kHeaderSize || len > kPacketMax) return false;
  memcpy(h, in, kHeaderSize);
  if (h->magic != kMagic || h->version != kVersion || h->project != project) return false;
  *body = in + kHeaderSize;
  *body_len = len - kHeaderSize;
  return true;
}

// The last two MAC bytes as 4 hex digits ("a574"): a board's name in events and capture files.
inline void mac_tag(const uint8_t mac[6], char out[5]) {
  snprintf(out, 5, "%02x%02x", mac[4], mac[5]);
}

struct Peer {
  uint8_t mac[6] = {};
  uint16_t last_seq = 0;
  uint32_t rx = 0;    // packets since the last report
  uint32_t lost = 0;  // sequence numbers skipped since the last report
  float rssi = 0;     // dBm, averaged
  int8_t rssi_last = 0;
  uint32_t last_ms = 0;  // when it was last heard, on this board's clock
  uint32_t rtt_us = 0;   // last round trip, 0 until one is measured
  bool used = false;

  float loss() const { return rx + lost ? static_cast<float>(lost) / (rx + lost) : 0; }
};

class PeerTable {
 public:
  static constexpr float kRssiAlpha = 0.1f;  // per packet: ~0.5 s at 20 Hz

  // Counts one packet from mac; returns its entry, or nullptr when the table is full.
  Peer* heard(const uint8_t mac[6], uint16_t seq, int8_t rssi, uint32_t now_ms) {
    Peer* p = find(mac);
    if (p == nullptr) {
      p = free_slot();
      if (p == nullptr) return nullptr;
      *p = Peer{};
      memcpy(p->mac, mac, 6);
      p->used = true;
      p->rssi = rssi;
    } else {
      const uint16_t gap = static_cast<uint16_t>(seq - p->last_seq);
      if (gap == 0) return p;  // a duplicate
      if (gap < kSeqJump) p->lost += gap - 1u;
      p->rssi += kRssiAlpha * (rssi - p->rssi);
    }
    p->last_seq = seq;
    p->rssi_last = rssi;
    p->last_ms = now_ms;
    ++p->rx;
    return p;
  }

  // Forgets peers not heard for timeout_ms.
  void age(uint32_t now_ms, uint32_t timeout_ms) {
    for (Peer& p : peers_) {
      if (p.used && now_ms - p.last_ms > timeout_ms) p.used = false;
    }
  }

  // Starts a new report window (rx and lost count from zero).
  void reset_counts() {
    for (Peer& p : peers_) p.rx = p.lost = 0;
  }

  Peer* find(const uint8_t mac[6]) {
    for (Peer& p : peers_) {
      if (p.used && memcmp(p.mac, mac, 6) == 0) return &p;
    }
    return nullptr;
  }

  uint8_t count() const {
    uint8_t n = 0;
    for (const Peer& p : peers_) n += p.used;
    return n;
  }

  const Peer* begin() const { return peers_; }
  const Peer* end() const { return peers_ + kMaxPeers; }

 private:
  Peer* free_slot() {
    for (Peer& p : peers_) {
      if (!p.used) return &p;
    }
    return nullptr;
  }

  Peer peers_[kMaxPeers];
};

constexpr uint32_t kRttMaxUs = 1000000;  // longer: a stale pong (a ping from before a reboot)

// Round trip of a pong answering this board's ping; 0 if the pong's numbers make no sense.
inline uint32_t rtt_us(const Pong& pong, uint32_t now_us) {
  const uint32_t total = now_us - pong.ping_sent_us;
  return total > pong.hold_us && total - pong.hold_us < kRttMaxUs ? total - pong.hold_us : 0;
}

}  // namespace lab_link
