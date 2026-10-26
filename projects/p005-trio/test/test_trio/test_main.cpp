// p005's pure logic: the law, who is who, and three boards simulated over a lossy, late link.
#include <math.h>
#include <unity.h>

#include "draw.h"
#include "law.h"
#include "trio.h"

void setUp() {}
void tearDown() {}

namespace {

const uint8_t kMacM[6] = {0xd4, 0xd4, 0xda, 0x98, 0x52, 0xb8};
const uint8_t kMacLo[6] = {0xd4, 0x05, 0x92, 0x44, 0xa5, 0x74};
const uint8_t kMacHi[6] = {0xd4, 0x05, 0x92, 0x48, 0x72, 0x0c};

trio::Beat beat(uint8_t role) {
  trio::Beat b = {};
  b.role = role;
  b.rate_mhz = 1000;
  return b;
}

// Three boards: beats every 50 ms at frame boundaries (10 ms frames), each lost with
// probability `loss`, arriving 2..5 ms late, read at the next frame. As on the boards, the
// arrival is stamped when the radio hands it over, not when the loop gets to it.
struct Sim {
  static constexpr int kN = 3;
  static constexpr uint32_t kFrameUs = 10000;
  static constexpr uint32_t kBeatUs = 50000;
  struct Msg {
    bool live;
    int to, from;
    uint32_t at_us;
    trio::Beat beat;
  };

  trio::Osc osc[kN];
  trio::Member seen[kN][kN];  // seen[board][other]
  Msg msgs[64] = {};
  uint32_t rng;
  float loss;
  uint32_t now_us = 0;

  Sim(uint32_t seed, float spread, float loss_) : rng(seed * 2654435761u | 1), loss(loss_) {
    const float detune[kN] = {-1, 0.35f, 1};
    for (int i = 0; i < kN; ++i) {
      osc[i].period_s = 1 + spread * detune[i];
      osc[i].rate = 1 / osc[i].period_s;
      osc[i].phase = unit();
    }
  }

  float unit() {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (rng >> 8) * (1.0f / 16777216.0f);
  }

  void send(int from) {
    trio::Beat b = beat(trio::kRoleS);
    b.phase = trio::to_u16(osc[from].phase);
    b.rate_mhz = static_cast<uint16_t>(osc[from].rate * 1000 + 0.5f);
    for (int to = 0; to < kN; ++to) {
      if (to == from || unit() < loss) continue;
      for (Msg& m : msgs) {
        if (m.live) continue;
        m = {true, to, from, now_us + 2000 + static_cast<uint32_t>(unit() * 3000), b};
        break;
      }
    }
  }

  void run(float seconds, float k) {
    const uint32_t end = now_us + static_cast<uint32_t>(seconds * 1e6f);
    for (; now_us < end; now_us += kFrameUs) frame(k);
  }

  void frame(float k) {
    for (Msg& m : msgs) {
      if (!m.live || m.at_us > now_us) continue;
      seen[m.to][m.from].present = true;
      seen[m.to][m.from].beat = m.beat;
      seen[m.to][m.from].at_us = m.at_us;
      m.live = false;
    }
    for (int i = 0; i < kN; ++i) {
      float others[kN];
      int n = 0;
      for (int j = 0; j < kN; ++j) {
        if (j != i && seen[i][j].present) others[n++] = seen[i][j].phase_at(now_us);
      }
      osc[i].step(kFrameUs * 1e-6f, k, others, n);
      if ((now_us / kFrameUs) % (kBeatUs / kFrameUs) == static_cast<uint32_t>(i)) send(i);
    }
  }

  void phases(float* x) const {
    for (int i = 0; i < kN; ++i) x[i] = osc[i].phase;
  }
};

}  // namespace

static void test_wrap_order_spread_and_gap() {
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.2f, trio::wrap_diff(0.9f, 0.1f));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.2f, trio::wrap_diff(0.1f, 0.9f));
  const float same[3] = {0.25f, 0.25f, 0.25f};
  const float even[3] = {0.1f, 0.1f + 1 / 3.0f, 0.1f + 2 / 3.0f};
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, trio::order(same, 3));
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, trio::order(even, 3));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, trio::spread(same, 3));
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1 / 3.0f, trio::min_gap(even, 3));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, trio::min_gap(same, 3));
}

static void test_an_oscillator_flashes_once_per_period_alone() {
  trio::Osc o;
  o.period_s = 0.8f;
  int flashes = 0;
  for (int i = 0; i < 810; ++i) flashes += o.step(0.01f, 0.4f, nullptr, 0);
  TEST_ASSERT_EQUAL(10, flashes);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.25f, o.rate);
  o.fire();
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, o.since_s());
}

static void test_phase_bytes_round_trip() {
  TEST_ASSERT_EQUAL(0, trio::to_u16(0));
  TEST_ASSERT_EQUAL(32768, trio::to_u16(0.5f));
  TEST_ASSERT_EQUAL(0, trio::to_u16(1.0f));
  TEST_ASSERT_EQUAL(0, trio::to_u16(-1e-9f));  // wraps to just under 1.0f, which is 1.0f
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.75f, trio::from_u16(trio::to_u16(0.75f)));
  trio::Member m;
  m.beat = beat(trio::kRoleS);
  m.beat.phase = trio::to_u16(0.9f);
  m.beat.rate_mhz = 1000;
  m.at_us = 4294967000u;  // the clock wraps between its beat and now
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.1f, m.phase_at(199704u));  // 0.2 s later
}

static void test_corners_go_by_role_and_mac() {
  trio::Trio t;
  t.set_self(kMacHi, trio::kRoleS, false);
  TEST_ASSERT_EQUAL(trio::kA, t.slot_self());  // alone, an S3 is a
  t.heard(kMacM, beat(trio::kRoleM), 0);
  t.heard(kMacLo, beat(trio::kRoleS), 0);
  TEST_ASSERT_EQUAL(trio::kB, t.slot_self());
  TEST_ASSERT_EQUAL(trio::kM, t.slot_other(0));
  TEST_ASSERT_EQUAL(trio::kA, t.slot_other(1));
  TEST_ASSERT_NOT_NULL(t.m5());
  TEST_ASSERT_NULL(t.heard(kMacHi, beat(trio::kRoleS), 0));  // a fourth board is ignored

  trio::Trio swapped;
  swapped.set_self(kMacHi, trio::kRoleS, true);
  swapped.heard(kMacLo, beat(trio::kRoleS), 0);
  TEST_ASSERT_EQUAL(trio::kA, swapped.slot_self());
  TEST_ASSERT_EQUAL(trio::kB, swapped.slot_other(0));

  t.age(3000000, 2000000);
  TEST_ASSERT_EQUAL(0, t.count());
  TEST_ASSERT_EQUAL(trio::kA, t.slot_self());
}

static void test_s3s_follow_new_commands_only() {
  trio::Follow f;
  trio::Beat m5 = beat(trio::kRoleM);
  m5.cmd = 7;
  m5.scatter = 1;
  m5.claps = 3;
  trio::Follow::Act act = f.update(m5);
  TEST_ASSERT_FALSE(act.scatter || act.fire);  // joining late: nothing to replay
  act = f.update(m5);
  TEST_ASSERT_FALSE(act.scatter || act.fire);
  m5.cmd = 8;
  act = f.update(m5);
  TEST_ASSERT_TRUE(act.scatter);
  TEST_ASSERT_FALSE(act.fire);
  m5.cmd = 9;
  m5.scatter = 0;  // a mode change, not a scatter
  m5.claps = 4;
  act = f.update(m5);
  TEST_ASSERT_FALSE(act.scatter);
  TEST_ASSERT_TRUE(act.fire);
}

// The locked offsets grow as spread / k (waltz holds half as tightly as unison), so these pin
// the config's BEAT_SPREAD 0.01 with BEAT_K 0.3 and above.
// Unison: whatever the start, the three flash within 6 ms of each other after 20 s.
static void test_positive_k_brings_unison_over_a_bad_link() {
  const float ks[] = {0.3f, 0.5f};
  for (float k : ks) {
    for (uint32_t seed = 1; seed <= 6; ++seed) {
      Sim sim(seed, 0.01f, 0.05f);
      sim.run(20, k);
      float x[3];
      sim.phases(x);
      TEST_ASSERT_LESS_THAN_FLOAT(0.006f, trio::spread(x, 3));
    }
  }
}

// Waltz: the three settle a third of a turn apart, within 3 degrees.
static void test_negative_k_brings_an_even_waltz() {
  const float ks[] = {-0.3f, -0.5f};
  for (float k : ks) {
    for (uint32_t seed = 1; seed <= 6; ++seed) {
      Sim sim(seed, 0.01f, 0.05f);
      sim.run(20, k);
      float x[3];
      sim.phases(x);
      TEST_ASSERT_GREATER_THAN_FLOAT(1 / 3.0f - 3 / 360.0f, trio::min_gap(x, 3));
    }
  }
}

// A clap fires all three; in waltz they then unfold back to a third of a turn apart.
static void test_a_clap_brings_unison_that_waltz_unfolds() {
  Sim sim(3, 0.01f, 0.05f);
  sim.run(20, -0.3f);
  for (trio::Osc& o : sim.osc) o.fire();
  float x[3];
  sim.phases(x);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, trio::spread(x, 3));
  sim.run(20, -0.3f);
  sim.phases(x);
  TEST_ASSERT_GREATER_THAN_FLOAT(1 / 3.0f - 3 / 360.0f, trio::min_gap(x, 3));
}

static void test_the_space_lights_present_corners_and_pulses() {
  const lab::Layout layout = lab::Layout::matrix(8, 8, true, true);
  lab::Pixel px[64];
  trio::View v;
  v.present[trio::kM] = v.present[trio::kA] = true;
  v.color[trio::kM] = lab::Pixel(255, 0, 0);
  v.color[trio::kA] = lab::Pixel(0, 0, 255);
  v.since_s[trio::kM] = 10;  // long since its flash
  v.since_s[trio::kA] = 10;
  trio::draw_space(px, layout, v);
  auto at = [&](int col, int row) {
    for (uint16_t i = 0; i < layout.size(); ++i) {
      if (layout.column(i) == col && layout.row(i) == row) return px[i];
    }
    return lab::Pixel(0, 0, 0);
  };
  TEST_ASSERT_GREATER_THAN(0, at(0, 7).b);                     // a's corner glows
  TEST_ASSERT_EQUAL(0, at(7, 7).r + at(7, 7).g + at(7, 7).b);  // b is absent: dark
  const uint8_t calm = at(0, 7).b;
  v.since_s[trio::kA] = 0;  // a flashes
  trio::draw_space(px, layout, v);
  TEST_ASSERT_GREATER_THAN(calm, at(0, 7).b);
  v.since_s[trio::kA] = v.travel_s / 2;  // its pulse is halfway to m
  trio::draw_space(px, layout, v);
  TEST_ASSERT_GREATER_THAN(60, at(2, 4).b);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wrap_order_spread_and_gap);
  RUN_TEST(test_an_oscillator_flashes_once_per_period_alone);
  RUN_TEST(test_phase_bytes_round_trip);
  RUN_TEST(test_corners_go_by_role_and_mac);
  RUN_TEST(test_s3s_follow_new_commands_only);
  RUN_TEST(test_positive_k_brings_unison_over_a_bad_link);
  RUN_TEST(test_negative_k_brings_an_even_waltz);
  RUN_TEST(test_a_clap_brings_unison_that_waltz_unfolds);
  RUN_TEST(test_the_space_lights_present_corners_and_pulses);
  return UNITY_END();
}
