// lab_link's pure parts: framing, project filtering, loss from sequence gaps, RSSI, round trip.
#include <unity.h>

#include "lab_link_proto.h"

using lab_link::Header;
using lab_link::PeerTable;

void setUp() {}
void tearDown() {}

static const uint8_t kA[6] = {0xd4, 0x05, 0x92, 0x44, 0xa5, 0x74};
static const uint8_t kB[6] = {0xd4, 0x05, 0x92, 0x48, 0x72, 0x0c};

static Header header(uint16_t project, uint8_t kind, uint16_t seq) {
  return Header{lab_link::kMagic, lab_link::kVersion, kind, project, seq, 123456u};
}

static void test_packets_round_trip_and_other_projects_drop() {
  const uint16_t p5 = lab_link::project_hash("p005-trio");
  TEST_ASSERT_NOT_EQUAL(p5, lab_link::project_hash("p006-other"));
  uint8_t buf[lab_link::kPacketMax];
  const uint8_t body[3] = {7, 8, 9};
  const size_t n = lab_link::encode(buf, sizeof buf, header(p5, 2, 41), body, sizeof body);
  TEST_ASSERT_EQUAL(lab_link::kHeaderSize + 3, n);

  Header h;
  const uint8_t* got = nullptr;
  size_t len = 0;
  TEST_ASSERT_TRUE(lab_link::decode(buf, n, p5, &h, &got, &len));
  TEST_ASSERT_EQUAL(2, h.kind);
  TEST_ASSERT_EQUAL(41, h.seq);
  TEST_ASSERT_EQUAL(123456u, h.sent_us);
  TEST_ASSERT_EQUAL(3, len);
  TEST_ASSERT_EQUAL(9, got[2]);

  TEST_ASSERT_FALSE(lab_link::decode(buf, n, lab_link::project_hash("p006-other"), &h, &got, &len));
  TEST_ASSERT_FALSE(lab_link::decode(buf, lab_link::kHeaderSize - 1, p5, &h, &got, &len));
  buf[0] ^= 0xff;  // not a lab_link packet at all
  TEST_ASSERT_FALSE(lab_link::decode(buf, n, p5, &h, &got, &len));
}

static void test_a_body_too_big_does_not_encode() {
  uint8_t buf[lab_link::kPacketMax];
  static uint8_t body[lab_link::kBodyMax + 1];
  TEST_ASSERT_EQUAL(0, lab_link::encode(buf, sizeof buf, header(1, 0, 0), body, sizeof body));
  TEST_ASSERT_EQUAL(lab_link::kPacketMax,
                    lab_link::encode(buf, sizeof buf, header(1, 0, 0), body, sizeof body - 1));
}

static void test_loss_counts_sequence_gaps_across_wrap() {
  PeerTable t;
  t.heard(kA, 65534, -40, 0);
  t.heard(kA, 65535, -40, 50);
  t.heard(kA, 1, -40, 100);  // 0 went missing across the wrap
  t.heard(kA, 1, -40, 110);  // a duplicate is neither a packet nor a loss
  t.heard(kA, 5, -40, 150);  // 2, 3, 4 lost
  const lab_link::Peer* a = t.find(kA);
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_EQUAL(4, a->rx);
  TEST_ASSERT_EQUAL(4, a->lost);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f, a->loss());
  t.reset_counts();
  TEST_ASSERT_EQUAL(0, t.find(kA)->rx);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, t.find(kA)->loss());
}

static void test_a_reboot_is_not_loss() {
  PeerTable t;
  t.heard(kA, 30000, -40, 0);
  t.heard(kA, 3, -40, 50);  // seq restarted after a reboot
  TEST_ASSERT_EQUAL(0, t.find(kA)->lost);
  t.heard(kA, 4, -40, 100);
  TEST_ASSERT_EQUAL(0, t.find(kA)->lost);
}

static void test_rssi_averages_and_peers_age_out() {
  PeerTable t;
  t.heard(kA, 0, -40, 0);
  for (uint16_t s = 1; s <= 60; ++s) t.heard(kA, s, -60, s * 50);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, -60.0f, t.find(kA)->rssi);
  TEST_ASSERT_EQUAL(-60, t.find(kA)->rssi_last);
  t.heard(kB, 0, -30, 2900);
  TEST_ASSERT_EQUAL(2, t.count());
  t.age(8000, 5000);  // A last heard at 3000: exactly 5 s is not yet silent
  TEST_ASSERT_EQUAL(1, t.count());
  t.age(8100, 5000);
  TEST_ASSERT_EQUAL(0, t.count());
  t.heard(kB, 9, -30, 8100);
  t.age(9000, 5000);
  TEST_ASSERT_EQUAL(1, t.count());
}

static void test_a_full_table_refuses_new_peers() {
  PeerTable t;
  uint8_t mac[6] = {1, 2, 3, 4, 5, 0};
  for (uint8_t i = 0; i < lab_link::kMaxPeers; ++i) {
    mac[5] = i;
    TEST_ASSERT_NOT_NULL(t.heard(mac, 0, -50, 0));
  }
  mac[5] = 200;
  TEST_ASSERT_NULL(t.heard(mac, 0, -50, 0));
  TEST_ASSERT_EQUAL(lab_link::kMaxPeers, t.count());
}

static void test_round_trip_subtracts_the_peers_hold() {
  lab_link::Pong pong = {};
  pong.ping_sent_us = 4294967000u;  // the clock wraps between ping and pong
  pong.hold_us = 700;
  TEST_ASSERT_EQUAL(2000 - 700 + 296, lab_link::rtt_us(pong, 2000));
  pong.hold_us = 5000;  // longer than the whole trip: junk
  TEST_ASSERT_EQUAL(0, lab_link::rtt_us(pong, 2000));
  pong.ping_sent_us = 90000000;  // a ping this board sent before it rebooted (clock now ~10 ms)
  pong.hold_us = 700;
  TEST_ASSERT_EQUAL(0, lab_link::rtt_us(pong, 10000));
}

static void test_mac_tag_is_the_last_two_bytes() {
  char tag[5];
  lab_link::mac_tag(kA, tag);
  TEST_ASSERT_EQUAL_STRING("a574", tag);
}

// An LLTF buffer whose signal subcarriers all have amplitude `amp` (imag 0, real amp), with bin
// `bump` doubled.
static void lltf(int8_t* buf, int8_t amp, int bump) {
  memset(buf, 0, 128);
  for (int i = 0; i < 52; ++i) {
    const int sc = i < 26 ? 1 + i : 38 + (i - 26);
    buf[2 * sc + 1] = static_cast<int8_t>(i / lab_link::kCsiPerBin == bump ? 2 * amp : amp);
  }
}

static void test_csi_shape_ignores_gain_and_sees_a_change() {
  int8_t quiet[128], loud[128], moved[128];
  lltf(quiet, 10, -1);
  lltf(loud, 40, -1);  // the same channel, received 4x louder
  lltf(moved, 10, 5);  // one band reshaped
  float a[lab_link::kCsiBins], b[lab_link::kCsiBins], c[lab_link::kCsiBins];
  TEST_ASSERT_TRUE(lab_link::csi_shape(quiet, 128, a));
  TEST_ASSERT_TRUE(lab_link::csi_shape(loud, 128, b));
  TEST_ASSERT_TRUE(lab_link::csi_shape(moved, 128, c));
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, a[0]);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, lab_link::csi_change(a, b));
  TEST_ASSERT_GREATER_THAN_FLOAT(0.1f, lab_link::csi_change(a, c));
  int8_t silent[128] = {};
  TEST_ASSERT_FALSE(lab_link::csi_shape(silent, 128, a));
  TEST_ASSERT_FALSE(lab_link::csi_shape(quiet, 100, a));  // not a full LLTF
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_packets_round_trip_and_other_projects_drop);
  RUN_TEST(test_a_body_too_big_does_not_encode);
  RUN_TEST(test_loss_counts_sequence_gaps_across_wrap);
  RUN_TEST(test_a_reboot_is_not_loss);
  RUN_TEST(test_rssi_averages_and_peers_age_out);
  RUN_TEST(test_a_full_table_refuses_new_peers);
  RUN_TEST(test_round_trip_subtracts_the_peers_hold);
  RUN_TEST(test_mac_tag_is_the_last_two_bytes);
  RUN_TEST(test_csi_shape_ignores_gain_and_sees_a_change);
  return UNITY_END();
}
