// p005's pure logic: the law, who is who, and three boards simulated over a lossy, late link.
#include <math.h>
#include <unity.h>

#include "draw.h"
#include "law.h"
#include "loom.h"
#include "mood.h"
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
  m.at_us = 1000;  // stamped by the radio just after the loop read the clock
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.9f, m.phase_at(900));
}

static void test_a_beat_stamped_after_now_is_not_ancient() {
  trio::Trio t;
  t.set_self(kMacHi, trio::kRoleS, false);
  t.heard(kMacLo, beat(trio::kRoleS), 5000, -40);
  t.age(4990, 2000000);  // the loop's clock read came 10 us before the stamp
  TEST_ASSERT_EQUAL(1, t.count());
}

static void test_corners_go_by_role_and_mac() {
  trio::Trio t;
  t.set_self(kMacHi, trio::kRoleS, false);
  TEST_ASSERT_EQUAL(trio::kA, t.slot_self());  // alone, an S3 is a
  t.heard(kMacM, beat(trio::kRoleM), 0, -40);
  t.heard(kMacLo, beat(trio::kRoleS), 0, -40);
  TEST_ASSERT_EQUAL(trio::kB, t.slot_self());
  TEST_ASSERT_EQUAL(trio::kM, t.slot_other(0));
  TEST_ASSERT_EQUAL(trio::kA, t.slot_other(1));
  TEST_ASSERT_NOT_NULL(t.m5());
  TEST_ASSERT_NULL(t.heard(kMacHi, beat(trio::kRoleS), 0, -40));  // a fourth board is ignored

  trio::Trio swapped;
  swapped.set_self(kMacHi, trio::kRoleS, true);
  swapped.heard(kMacLo, beat(trio::kRoleS), 0, -40);
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

// RSSI with +-1 dB of packet noise; a hand in the gap drops it 8 dB and scatters it +-4 dB.
static void test_a_link_stirs_for_a_hand_and_settles_after() {
  trio::Link link;
  uint32_t r = 12345;
  auto noise = [&r](float amp) {
    r = r * 1664525u + 1013904223u;
    return amp * (2 * ((r >> 8) * (1.0f / 16777216.0f)) - 1);
  };
  for (int i = 0; i < 1200; ++i) link.push(-45 + noise(1), 0.05f);  // a quiet minute
  TEST_ASSERT_LESS_THAN_FLOAT(0.05f, link.stir(1.5f, 6));
  float peak = 0;
  for (int i = 0; i < 60; ++i) {  // 3 s of a hand
    link.push(-53 + noise(4), 0.05f);
    peak = fmaxf(peak, link.stir(1.5f, 6));
  }
  TEST_ASSERT_GREATER_THAN_FLOAT(0.5f, peak);
  for (int i = 0; i < 60; ++i) link.push(-45 + noise(1), 0.05f);  // 3 s after it left
  TEST_ASSERT_LESS_THAN_FLOAT(0.1f, link.stir(1.5f, 6));
}

static void test_a_new_link_reports_nothing_until_warm() {
  trio::Link link;
  for (uint32_t i = 0; i + 1 < trio::Link::kWarmup; ++i) link.push(i % 2 ? -30 : -70, 0.05f);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, link.stir(1.5f, 6));
}

static void test_edges_combine_own_links_and_what_the_others_hear() {
  TEST_ASSERT_EQUAL(0, trio::edge_of(trio::kA, trio::kM));
  TEST_ASSERT_EQUAL(1, trio::edge_of(trio::kB, trio::kA));
  TEST_ASSERT_EQUAL(2, trio::edge_of(trio::kM, trio::kB));
  trio::Trio t;  // the M5's view
  t.set_self(kMacM, trio::kRoleM, false);
  trio::Beat a = beat(trio::kRoleS), b = beat(trio::kRoleS);
  a.heard[0] = {{kMacHi[4], kMacHi[5]}, 51};   // a hears b at 0.2
  a.heard[1] = {{kMacM[4], kMacM[5]}, 255};    // and the M5 at 1.0
  b.heard[0] = {{kMacLo[4], kMacLo[5]}, 153};  // b hears a at 0.6
  for (int i = 0; i < 50; ++i) {
    t.heard(kMacLo, a, i * 50000u, -40);
    t.heard(kMacHi, b, i * 50000u, -40);
  }
  float e[3];
  trio::edges(t, 1.5f, 6, e);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, e[0]);  // m-a: a's report beats the M5's calm link
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.6f, e[1]);  // a-b: the larger of the two directions
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, e[2]);  // b-m: calm both ways
  trio::fill_heard(t, &a, 1.5f, 6);
  TEST_ASSERT_TRUE(trio::tag_is(a.heard[0].tag, kMacLo));
  TEST_ASSERT_EQUAL(0, a.heard[0].stir);
}

static void test_shown_values_rise_fast_and_fall_slowly() {
  const float up = trio::follow(0, 1, 0.1f, 0.1f, 1.0f);
  const float down = trio::follow(1, 0, 0.1f, 0.1f, 1.0f);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.6f, up);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.9f, down);
}

// Three hues under MoodConfig's defaults (config.h's MOOD_* copy them), stepped together.
struct Palette {
  trio::MoodConfig c;
  trio::Hue h[3];
  trio::Room room;

  explicit Palette(const float* start) {
    const float seeds[3] = {0.1f, 0.55f, 0.9f};
    for (int i = 0; i < 3; ++i) {
      h[i].set_drift(seeds[i], c);
      h[i].h = start[i];
    }
  }

  void run(float seconds, float stir_now) {
    const float dt = 0.05f;
    for (float t = 0; t < seconds; t += dt) {
      room.push(stir_now, dt, c);
      const float k = trio::mood_k(room.value, c);
      const float x[3] = {h[0].h, h[1].h, h[2].h};
      for (int i = 0; i < 3; ++i) {
        float o[2];
        int n = 0;
        for (int j = 0; j < 3; ++j) {
          if (j != i) o[n++] = x[j];
        }
        h[i].step(dt, k, o, n);
      }
    }
  }

  float gap() const {
    const float x[3] = {h[0].h, h[1].h, h[2].h};
    return trio::min_gap(x, 3);
  }
  float spread() const {
    const float x[3] = {h[0].h, h[1].h, h[2].h};
    return trio::spread(x, 3);
  }
};

static void test_the_room_sets_the_sign_of_the_mood() {
  const trio::MoodConfig c;
  TEST_ASSERT_FLOAT_WITHIN(1e-9f, c.k_calm, trio::mood_k(0, c));
  TEST_ASSERT_FLOAT_WITHIN(1e-9f, -c.k_storm, trio::mood_k(1, c));
  TEST_ASSERT_GREATER_THAN_FLOAT(0, trio::mood_k(c.calm_below, c));
  TEST_ASSERT_LESS_THAN_FLOAT(0, trio::mood_k(c.stir_above, c));
}

static void test_a_quiet_room_brings_the_hues_together() {
  const float start[3] = {0.05f, 0.4f, 0.8f};
  Palette p(start);
  p.run(600, 0.02f);
  TEST_ASSERT_LESS_THAN_FLOAT(3 / 360.0f, p.spread());
}

// Five claps: a triad within three minutes, and agreement again within fifteen.
static void test_five_claps_break_into_a_triad_that_heals() {
  const float start[3] = {0.30f, 0.31f, 0.315f};
  Palette p(start);
  p.room.value = 0.02f;
  for (int i = 0; i < 5; ++i) p.room.clap(p.c);
  p.run(180, 0.02f);
  TEST_ASSERT_GREATER_THAN_FLOAT(100 / 360.0f, p.gap());
  p.run(720, 0.02f);
  TEST_ASSERT_LESS_THAN_FLOAT(10 / 360.0f, p.spread());
}

static void test_a_lone_hue_circles_at_its_own_drift() {
  const trio::MoodConfig c;
  trio::Hue h;
  h.set_drift(0.5f, c);  // the middle seed: exactly one lap per lap_s
  for (int i = 0; i < 3600; ++i) h.step(1, c.k_calm, nullptr, 0);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.5f, h.h);
}

static void test_agreement_fans_out_and_a_triad_does_not() {
  const float fan = 12.5f / 360;
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f, trio::shown_hue(0.5f, trio::kM, 1, fan));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f - fan, trio::shown_hue(0.5f, trio::kA, 1, fan));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f + fan, trio::shown_hue(0.5f, trio::kB, 1, fan));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f, trio::shown_hue(0.5f, trio::kB, 0, fan));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1 - fan, trio::shown_hue(0, trio::kA, 1, fan));  // wraps
}

static trio::Thread thread(uint8_t r, uint8_t e0, uint8_t claps) {
  trio::Thread t = {{0, 85, 170}, r, {e0, 0, 0}, claps};
  return t;
}

// Rows of a column in band m's color (either shade of the twill).
static int m_rows(const trio::Thread& t, uint32_t index) {
  lab::Pixel px[135];
  trio::thread_column(t, index, px, 135);
  const lab::Pixel m = lab::hsv(t.hue[0], 210, 255), dim = lab::scale(m, 0.78f);
  int n = 0;
  for (const lab::Pixel& p : px) n += (p.r == m.r && p.g == m.g) || (p.r == dim.r && p.g == dim.g);
  return n;
}

static void test_threads_run_straight_while_the_beat_agrees() {
  TEST_ASSERT_EQUAL(m_rows(thread(255, 0, 0), 10), m_rows(thread(255, 0, 0), 11));
  const int even = m_rows(thread(0, 0, 0), 10), odd = m_rows(thread(0, 0, 0), 11);  // waltz
  TEST_ASSERT_GREATER_THAN(20, abs(even - odd));  // the borders zig-zag thread to thread
  TEST_ASSERT_NOT_EQUAL(m_rows(thread(255, 0, 0), 10), even);
}

static void test_a_disturbed_edge_frays_its_border_and_claps_tie_knots() {
  lab::Pixel calm[135], frayed[135];
  trio::thread_column(thread(255, 0, 0), 7, calm, 135);
  trio::thread_column(thread(255, 255, 0), 7, frayed, 135);
  int changed = 0, far = 0;
  for (int y = 0; y < 135; ++y) {
    const bool diff =
        calm[y].r != frayed[y].r || calm[y].g != frayed[y].g || calm[y].b != frayed[y].b;
    changed += diff;
    far += diff && y > 80;  // the a|b border (row ~87) stays clean: only m-a was disturbed
  }
  TEST_ASSERT_GREATER_THAN(3, changed);
  TEST_ASSERT_EQUAL(0, far);
  int white = 0;
  trio::thread_column(thread(255, 0, 2), 7, frayed, 135);
  for (int y = 0; y < 135; ++y)
    white += frayed[y].r == 255 && frayed[y].g == 255 && frayed[y].b == 255;
  TEST_ASSERT_GREATER_OR_EQUAL(3, white);
  white = 0;
  for (int y = 0; y < 135; ++y) white += calm[y].r == 255 && calm[y].g == 255 && calm[y].b == 255;
  TEST_ASSERT_EQUAL(0, white);
}

static void test_a_weft_averages_hues_round_the_wheel() {
  trio::Weft w;
  const bool all[3] = {true, true, false};
  const float h1[3] = {0.95f, 0.5f, 0}, h2[3] = {0.05f, 0.5f, 0};
  w.hues(h1, all);
  w.hues(h2, all);
  w.order(1);
  w.order(0.5f);
  const float e1[3] = {0.2f, 0, 0}, e2[3] = {0.9f, 0.1f, 0};
  w.edges(e1);
  w.edges(e2);
  w.clap();
  const trio::Thread t = w.finish();
  TEST_ASSERT_TRUE(t.hue[0] < 3 || t.hue[0] > 252);  // 0, not 0.5
  TEST_ASSERT_EQUAL(128, t.hue[1]);
  TEST_ASSERT_EQUAL(191, t.r);
  TEST_ASSERT_EQUAL(230, t.edge[0]);
  TEST_ASSERT_EQUAL(1, t.claps);
  const trio::Thread next = w.finish();  // a fresh weft keeps the last hues, not the claps
  TEST_ASSERT_EQUAL(128, next.hue[1]);
  TEST_ASSERT_EQUAL(0, next.claps);
  TEST_ASSERT_EQUAL(0, next.edge[0]);
}

static void test_the_loom_overwrites_its_oldest_thread() {
  static trio::Loom loom;
  for (int i = 0; i < trio::Loom::kMax; ++i) TEST_ASSERT_EQUAL(i, loom.add(thread(0, 0, 0)));
  TEST_ASSERT_EQUAL(0, loom.add(thread(9, 0, 0)));
  TEST_ASSERT_EQUAL(trio::Loom::kMax + 1, loom.woven());
  TEST_ASSERT_EQUAL(trio::Loom::kMax, loom.columns());
  TEST_ASSERT_EQUAL(9, loom.at(0).r);
}

static void test_a_column_prints_as_rgb565_hex() {
  lab::Pixel px[8];
  for (int y = 0; y < 8; ++y) px[y] = y < 4 ? lab::Pixel(255, 0, 0) : lab::Pixel(0, 0, 255);
  char hex[4 * 2 + 1];
  trio::column_hex(px, 8, 2, hex);
  TEST_ASSERT_EQUAL_STRING("f800001f", hex);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wrap_order_spread_and_gap);
  RUN_TEST(test_an_oscillator_flashes_once_per_period_alone);
  RUN_TEST(test_phase_bytes_round_trip);
  RUN_TEST(test_a_beat_stamped_after_now_is_not_ancient);
  RUN_TEST(test_corners_go_by_role_and_mac);
  RUN_TEST(test_s3s_follow_new_commands_only);
  RUN_TEST(test_positive_k_brings_unison_over_a_bad_link);
  RUN_TEST(test_negative_k_brings_an_even_waltz);
  RUN_TEST(test_a_clap_brings_unison_that_waltz_unfolds);
  RUN_TEST(test_the_space_lights_present_corners_and_pulses);
  RUN_TEST(test_a_link_stirs_for_a_hand_and_settles_after);
  RUN_TEST(test_a_new_link_reports_nothing_until_warm);
  RUN_TEST(test_edges_combine_own_links_and_what_the_others_hear);
  RUN_TEST(test_shown_values_rise_fast_and_fall_slowly);
  RUN_TEST(test_the_room_sets_the_sign_of_the_mood);
  RUN_TEST(test_a_quiet_room_brings_the_hues_together);
  RUN_TEST(test_five_claps_break_into_a_triad_that_heals);
  RUN_TEST(test_a_lone_hue_circles_at_its_own_drift);
  RUN_TEST(test_agreement_fans_out_and_a_triad_does_not);
  RUN_TEST(test_threads_run_straight_while_the_beat_agrees);
  RUN_TEST(test_a_disturbed_edge_frays_its_border_and_claps_tie_knots);
  RUN_TEST(test_a_weft_averages_hues_round_the_wheel);
  RUN_TEST(test_the_loom_overwrites_its_oldest_thread);
  RUN_TEST(test_a_column_prints_as_rgb565_hex);
  return UNITY_END();
}
