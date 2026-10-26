#pragma once
// Boards talking to each other over ESP-NOW: no router, every packet a broadcast on one fixed
// channel. Packets carry a header (lab_link_proto.h), so another project's traffic is dropped and
// each peer's loss, RSSI and round trip are known.
// Usage: setup(): lab_link::begin({LAB_PROJECT, LINK_CHANNEL, LINK_TX_DBM});
//        loop():  lab_link::handle(now); while (lab_link::poll(&rx)) { ... }
//                 lab_link::send(kind, &body, sizeof body);
//        every 5 s: lab_link::emit();  // one `link` event per peer
// Events: `radio` (mac, tag, channel) at begin, `link` per peer, `err` link_init / link_send.

#include "lab_link_proto.h"

#ifdef ARDUINO

namespace lab_link {

struct Config {
  const char* project;  // LAB_PROJECT
  uint8_t channel;      // 1..13; every board of the project on the same one
  int8_t tx_dbm;        // 2..20
  bool csi = false;     // channel state per peer (csi_motion); broadcasts move to OFDM 6 Mb/s
};

struct Rx {
  uint8_t mac[6];
  Header head;
  int8_t rssi;
  uint32_t at_us;  // when the radio handed it over, on this board's esp_timer
  uint8_t len;
  uint8_t body[kBodyMax];
};

// Starts the radio (WiFi station, no access point, power save off) and ESP-NOW. False, with an
// `err` event, if the radio would not start.
bool begin(const Config& config);

// Broadcasts body as a packet of `kind` (0..0xEF). False if it was not queued.
bool send(uint8_t kind, const void* body, size_t len);

// The next packet of this project, oldest first. lab_link's own (ping, pong) are answered here
// and not returned. Every packet counts toward its sender's loss and RSSI.
bool poll(Rx* out);

// Pings once a second and forgets peers silent for 5 s. Call every loop.
void handle(uint32_t now_ms);

// One `link` event per peer (peer tag, rssi, rx, loss, rtt_us), then a new counting window.
void emit();

const PeerTable& peers();
const uint8_t* self_mac();

// Mean change of the CSI shape (lab_link_proto.h) over the packets from mac since the last call;
// false if none came (or csi is off).
bool csi_motion(const uint8_t mac[6], float* motion);

}  // namespace lab_link

#endif  // ARDUINO
