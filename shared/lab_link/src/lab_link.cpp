#ifdef ARDUINO

#include "lab_link.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <math.h>

#include "lab_core.h"

namespace lab_link {

namespace {
constexpr uint8_t kBroadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
constexpr uint32_t kPingMs = 1000;
constexpr uint32_t kPeerTimeoutMs = 5000;
constexpr UBaseType_t kQueueDepth = 16;

QueueHandle_t queue = nullptr;
uint16_t project = 0;
uint16_t seq = 0;
uint8_t mac[6] = {};
PeerTable table;
uint32_t last_ping_ms = 0;
uint32_t tx_fail = 0;  // since the last emit()
bool up = false;
int8_t tx_quarter_dbm = 0;  // what begin() asked for, in esp_wifi's 0.25 dBm units
bool tx_checked = false;
constexpr uint32_t kTxCheckMs = 3000;

// CSI per peer: written by the WiFi task (on_csi), read by loop() (csi_motion) under csi_mux.
struct CsiPeer {
  uint8_t mac[6];
  bool used;
  bool has_prev;
  float prev[kCsiBins];
  float sum;
  uint32_t n;
};
CsiPeer csi_peers[kMaxPeers] = {};
portMUX_TYPE csi_mux = portMUX_INITIALIZER_UNLOCKED;
// Who has sent us a packet of this project: only their CSI counts. WiFi task only.
uint8_t known[kMaxPeers][6] = {};
uint8_t known_n = 0;

void remember(const uint8_t* src) {
  for (uint8_t i = 0; i < known_n; ++i) {
    if (memcmp(known[i], src, 6) == 0) return;
  }
  if (known_n < kMaxPeers) memcpy(known[known_n++], src, 6);
}

bool is_known(const uint8_t* src) {
  for (uint8_t i = 0; i < known_n; ++i) {
    if (memcmp(known[i], src, 6) == 0) return true;
  }
  return false;
}

// WiFi task: one packet's channel shape, compared with the previous from the same peer.
void on_csi(void*, wifi_csi_info_t* info) {
  if (info == nullptr || info->buf == nullptr || !is_known(info->mac)) return;
  int8_t buf[128];
  if (info->len < static_cast<int>(sizeof buf)) return;
  memcpy(buf, info->buf, sizeof buf);
  memset(buf, 0, 4);  // the first word is sometimes invalid (hardware); always drop it
  float shape[kCsiBins];
  if (!csi_shape(buf, sizeof buf, shape)) return;
  portENTER_CRITICAL(&csi_mux);
  CsiPeer* p = nullptr;
  for (CsiPeer& c : csi_peers) {
    if (c.used && memcmp(c.mac, info->mac, 6) == 0) p = &c;
  }
  for (CsiPeer& c : csi_peers) {
    if (p == nullptr && !c.used) {
      p = &c;
      *p = CsiPeer{};
      memcpy(p->mac, info->mac, 6);
      p->used = true;
    }
  }
  if (p != nullptr) {
    if (p->has_prev) {
      p->sum += csi_change(p->prev, shape);
      ++p->n;
    }
    memcpy(p->prev, shape, sizeof shape);
    p->has_prev = true;
  }
  portEXIT_CRITICAL(&csi_mux);
}

bool start_csi() {
  esp_now_rate_config_t rate = {};
  rate.phymode = WIFI_PHY_MODE_11G;  // OFDM, so every frame carries the LLTF that CSI reads
  rate.rate = WIFI_PHY_RATE_6M;
  wifi_csi_config_t cfg = {};
  cfg.lltf_en = true;
  cfg.ltf_merge_en = true;
  cfg.channel_filter_en = false;  // keep neighbouring subcarriers independent
  return esp_now_set_peer_rate_config(kBroadcast, &rate) == ESP_OK &&
         esp_wifi_set_csi_config(&cfg) == ESP_OK &&
         esp_wifi_set_csi_rx_cb(on_csi, nullptr) == ESP_OK && esp_wifi_set_csi(true) == ESP_OK;
}

uint32_t now_us() { return static_cast<uint32_t>(esp_timer_get_time()); }

// Runs in the WiFi task (core 0): decode, stamp and queue only; poll() does the rest in loop().
void on_recv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  static Rx rx;  // only the WiFi task calls this, so one buffer is enough
  const uint8_t* body = nullptr;
  size_t body_len = 0;
  if (queue == nullptr || len < 0) return;
  if (!decode(data, static_cast<size_t>(len), project, &rx.head, &body, &body_len)) return;
  remember(info->src_addr);
  memcpy(rx.mac, info->src_addr, 6);
  rx.rssi = info->rx_ctrl ? static_cast<int8_t>(info->rx_ctrl->rssi) : 0;
  rx.at_us = now_us();
  rx.len = static_cast<uint8_t>(body_len);
  memcpy(rx.body, body, body_len);
  xQueueSend(queue, &rx, 0);  // a full queue drops it; the sender's seq gap counts it as lost
}

bool send_raw(uint8_t kind, const void* body, size_t len) {
  if (!up) return false;
  const Header h{kMagic, kVersion, kind, project, seq++, now_us()};
  uint8_t buf[kPacketMax];
  const size_t n = encode(buf, sizeof buf, h, body, len);
  const bool ok = n > 0 && esp_now_send(kBroadcast, buf, n) == ESP_OK;
  if (!ok) ++tx_fail;
  return ok;
}

void answer(const Rx& rx) {
  Pong pong;
  memcpy(pong.to, rx.mac, 6);
  pong.ping_sent_us = rx.head.sent_us;
  pong.hold_us = now_us() - rx.at_us;
  send_raw(kPong, &pong, sizeof pong);
}
}  // namespace

bool begin(const Config& config) {
  project = project_hash(config.project);
  tx_quarter_dbm = static_cast<int8_t>(config.tx_dbm * 4);
  WiFi.setSleep(false);  // Arduino re-applies its sleep setting when the station starts
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  bool ok = esp_wifi_set_ps(WIFI_PS_NONE) == ESP_OK;
  ok = ok && esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE) == ESP_OK;
  ok = ok && esp_wifi_set_max_tx_power(tx_quarter_dbm) == ESP_OK;
  ok = ok && esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK;
  queue = xQueueCreate(kQueueDepth, sizeof(Rx));
  ok = ok && queue != nullptr && esp_now_init() == ESP_OK;
  ok = ok && esp_now_register_recv_cb(on_recv) == ESP_OK;
  if (ok) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, kBroadcast, 6);
    peer.channel = config.channel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    ok = esp_now_add_peer(&peer) == ESP_OK;
  }
  const bool csi = ok && config.csi && start_csi();
  up = ok;
  char tag[5], full[18];
  mac_tag(mac, tag);
  snprintf(full, sizeof full, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);
  lab_core::Event("radio")
      .str("mac", full)
      .str("tag", tag)
      .num("ch", config.channel)
      .num("tx_dbm", config.tx_dbm)
      .flag("csi", csi)
      .flag("ok", ok)
      .emit();
  if (!ok) lab_core::error("link_init");
  if (ok && config.csi && !csi) lab_core::error("link_csi");
  return ok;
}

bool send(uint8_t kind, const void* body, size_t len) {
  return kind < kPing && send_raw(kind, body, len);
}

bool poll(Rx* out) {
  while (queue != nullptr && xQueueReceive(queue, out, 0) == pdTRUE) {
    Peer* peer = table.heard(out->mac, out->head.seq, out->rssi, millis());
    if (out->head.kind == kPing) {
      answer(*out);
    } else if (out->head.kind == kPong) {
      Pong pong;
      if (peer != nullptr && out->len == sizeof pong) {
        memcpy(&pong, out->body, sizeof pong);
        if (memcmp(pong.to, mac, 6) == 0) peer->rtt_us = rtt_us(pong, out->at_us);
      }
    } else {
      return true;
    }
  }
  return false;
}

// The radio's TX power a few seconds in, put back if something reset it (reported either way).
void check_tx_power() {
  int8_t was = 0;
  esp_wifi_get_max_tx_power(&was);
  const bool drifted = abs(was - tx_quarter_dbm) > 4;  // chips round to their own steps
  if (drifted) esp_wifi_set_max_tx_power(tx_quarter_dbm);
  int8_t now = 0;
  esp_wifi_get_max_tx_power(&now);
  lab_core::Event("radio")
      .real("tx_dbm_was", was / 4.0, 2)
      .real("tx_dbm", now / 4.0, 2)
      .flag("reapplied", drifted)
      .emit();
}

void handle(uint32_t now_ms) {
  if (!up) return;
  if (!tx_checked && now_ms >= kTxCheckMs) {
    tx_checked = true;
    check_tx_power();
  }
  if (now_ms - last_ping_ms >= kPingMs) {
    last_ping_ms = now_ms;
    send_raw(kPing, nullptr, 0);
  }
  table.age(now_ms, kPeerTimeoutMs);
}

void emit() {
  for (const Peer& p : table) {
    if (!p.used) continue;
    char tag[5];
    mac_tag(p.mac, tag);
    lab_core::Event("link")
        .str("peer", tag)
        .num("rssi", lroundf(p.rssi))
        .num("rx", p.rx)
        .real("loss", p.loss(), 3)
        .num("rtt_us", p.rtt_us)
        .num("tx_fail", tx_fail)
        .emit();
  }
  table.reset_counts();
  tx_fail = 0;
}

bool csi_motion(const uint8_t peer_mac[6], float* motion) {
  bool got = false;
  portENTER_CRITICAL(&csi_mux);
  for (CsiPeer& c : csi_peers) {
    if (!c.used || memcmp(c.mac, peer_mac, 6) != 0) continue;
    got = c.n > 0;
    *motion = got ? c.sum / c.n : 0;
    c.sum = 0;
    c.n = 0;
  }
  portEXIT_CRITICAL(&csi_mux);
  return got;
}

const PeerTable& peers() { return table; }
const uint8_t* self_mac() { return mac; }

}  // namespace lab_link

#endif  // ARDUINO
