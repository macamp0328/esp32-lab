// Simulations: rules, invariants and drawing bounds, with fixed seeds.
#include <unity.h>

#include "lab_sim.h"

using lab::Layout;
using lab::Pixel;

void setUp() {}
void tearDown() {}

static const lab_sim::Input kDown;
static const Pixel kBlack(0, 0, 0);

static void test_life_rules_blinker_block_glider() {
  const lab_sim::Grid g = lab_sim::Grid::of(Layout::matrix(8, 8, true));
  uint8_t a[64] = {}, b[64] = {};
  a[g.at(2, 3)] = a[g.at(3, 3)] = a[g.at(4, 3)] = 1;                  // blinker
  a[g.at(6, 6)] = a[g.at(7, 6)] = a[g.at(6, 7)] = a[g.at(7, 7)] = 1;  // block
  lab_sim::life_next(a, b, g);
  for (int y = 2; y <= 4; ++y) TEST_ASSERT_TRUE(b[g.at(3, y)]);
  TEST_ASSERT_FALSE(b[g.at(2, 3)] || b[g.at(4, 3)]);
  TEST_ASSERT_EQUAL_UINT8(2, b[g.at(6, 6)]);  // the block survives and ages
  lab_sim::life_next(b, a, g);
  for (int x = 2; x <= 4; ++x) TEST_ASSERT_TRUE(a[g.at(x, 3)]);

  uint8_t glider[64] = {}, tmp[64];
  const int cells[5][2] = {{1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2}};
  for (const auto& c : cells) glider[g.at(c[0], c[1])] = 1;
  for (int gen = 0; gen < 4; ++gen) {
    lab_sim::life_next(glider, tmp, g);
    memcpy(glider, tmp, sizeof glider);
  }
  int alive = 0;
  for (uint8_t v : glider) alive += v > 0;
  TEST_ASSERT_EQUAL_INT(5, alive);
  for (const auto& c : cells) TEST_ASSERT_TRUE(glider[g.at(c[0] + 1, c[1] + 1)]);  // moved (1, 1)
}

static void test_life_reseeds_after_dying_or_repeating() {
  const Layout l = Layout::matrix(8, 8, true);
  lab_sim::Life life;
  life.reset(7, l);
  TEST_ASSERT_TRUE(life.population() > 5);
  life.clear();
  life.set(3, 3, true);  // a lone cell dies at once
  for (int k = 0; k < lab_sim::Life::kLinger + 2; ++k) life.step(kDown);
  TEST_ASSERT_EQUAL_UINT32(1, life.reseeds());
  TEST_ASSERT_TRUE(life.population() > 5);
  life.clear();
  life.set(1, 1, true);  // a still block repeats forever
  life.set(2, 1, true);
  life.set(1, 2, true);
  life.set(2, 2, true);
  for (int k = 0; k < lab_sim::Life::kLinger + 2; ++k) life.step(kDown);
  TEST_ASSERT_EQUAL_UINT32(2, life.reseeds());
}

static void test_ants_follow_the_rule() {
  const Layout l = Layout::matrix(8, 8, true, true);
  lab_sim::Ant ant;
  ant.reset(11, l);
  const lab_sim::Ant::Walker a0 = ant.ant(0), a1 = ant.ant(1);
  ant.step(kDown);
  static const int kDx[] = {0, 1, 0, -1}, kDy[] = {-1, 0, 1, 0};
  for (uint8_t k = 0; k < 2; ++k) {
    const lab_sim::Ant::Walker before = k ? a1 : a0, after = ant.ant(k);
    TEST_ASSERT_EQUAL_UINT8(k + 1, ant.cell(before.x, before.y));  // empty cell: painted
    TEST_ASSERT_EQUAL_UINT8((before.dir + 1) & 3, after.dir);      // and a right turn
    TEST_ASSERT_EQUAL_INT((before.x + kDx[after.dir] + 8) % 8, after.x);
    TEST_ASSERT_EQUAL_INT((before.y + kDy[after.dir] + 8) % 8, after.y);
  }
}

static void test_snake_never_hits_itself_and_grows_when_it_eats() {
  const Layout l = Layout::matrix(8, 8, true);
  for (uint32_t seed = 1; seed <= 5; ++seed) {
    lab_sim::Snake snake;
    snake.reset(seed, l);
    for (int s = 0; s < 3000; ++s) {
      const uint32_t eaten = snake.eaten(), restarts = snake.restarts();
      const uint16_t len = snake.length();
      snake.step(kDown);
      if (snake.restarts() != restarts) {
        TEST_ASSERT_EQUAL_UINT16(3, snake.length());  // a clean restart
      } else if (snake.eaten() != eaten) {
        TEST_ASSERT_EQUAL_UINT16(len + 1, snake.length());
      }
      uint8_t seen[64] = {};
      for (uint16_t k = 0; k < snake.length(); ++k) {
        const uint16_t c = snake.segment(k);
        TEST_ASSERT_TRUE(c < 64);
        TEST_ASSERT_FALSE(seen[c]);
        seen[c] = 1;
        if (k > 0) {
          const int p = snake.segment(k - 1);
          const int d = abs(p % 8 - c % 8) + abs(p / 8 - c / 8);
          TEST_ASSERT_EQUAL_INT(1, d);
        }
      }
      if (snake.apple() >= 0) TEST_ASSERT_FALSE(seen[snake.apple()]);
    }
    TEST_ASSERT_TRUE(snake.eaten() > 60);
  }
}

static void test_tetris_collapse_removes_full_rows() {
  const lab_sim::Grid g = lab_sim::Grid::of(Layout::matrix(4, 4, true));
  uint8_t well[16] = {0, 0, 0, 0,  //
                      1, 0, 0, 0,  //
                      2, 2, 2, 2,  //
                      3, 3, 3, 3};
  TEST_ASSERT_EQUAL_UINT8(2, lab_sim::tetris_collapse(well, g));
  const uint8_t want[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(want, well, 16);
}

static void test_tetris_plays_clean_and_clears_lines() {
  const Layout l = Layout::matrix(8, 8, true);
  lab_sim::Tetris tetris;
  tetris.reset(3, l);
  for (int s = 0; s < 4000; ++s) {
    tetris.step(kDown);
    for (int y = 0; y < 8; ++y) {
      for (int x = 0; x < 8; ++x) TEST_ASSERT_FALSE(tetris.piece_covers(x, y) && tetris.cell(x, y));
    }
  }
  TEST_ASSERT_TRUE(tetris.lines() > 10);
}

static void test_tetris_restarts_when_topped_out() {
  const Layout l = Layout::matrix(8, 8, true);
  lab_sim::Tetris tetris;
  tetris.reset(5, l);
  for (int y = 1; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) tetris.set_cell(x, y, (x + y) % 2 ? 0 : 4);  // no full rows
  }
  for (int s = 0; s < 40 && tetris.resets() == 0; ++s) tetris.step(kDown);
  TEST_ASSERT_EQUAL_UINT32(1, tetris.resets());
  int filled = 0;
  for (int y = 4; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) filled += tetris.cell(x, y) != 0;
  }
  TEST_ASSERT_TRUE(filled < 8);  // the old stack is gone
}

static void test_sand_quantizes_gravity() {
  int dx, dy;
  lab_sim::Sand::direction({0, 0}, &dx, &dy);
  TEST_ASSERT_TRUE(dx == 0 && dy == 1);  // flat: straight down
  lab_sim::Sand::direction({1, 0.1f}, &dx, &dy);
  TEST_ASSERT_TRUE(dx == 1 && dy == 0);
  lab_sim::Sand::direction({-0.7f, -0.7f}, &dx, &dy);
  TEST_ASSERT_TRUE(dx == -1 && dy == -1);
}

static void test_sand_falls_settles_and_follows_gravity() {
  const Layout l = Layout::matrix(8, 8, true, true);
  lab_sim::Sand sand;
  sand.reset(9, l);
  sand.pour = false;
  for (int x = 1; x < 6; ++x) {
    sand.add(x, 0, 10);
    sand.add(x, 1, 20);
  }
  for (int s = 0; s < 40; ++s) sand.step(kDown);
  TEST_ASSERT_EQUAL_UINT16(10, sand.count());
  int seen = 0;
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      if (!sand.cell(x, y)) continue;
      ++seen;
      TEST_ASSERT_TRUE(y == 7 || sand.cell(x, y + 1));  // resting on the floor or a grain
    }
  }
  TEST_ASSERT_EQUAL_INT(10, seen);
  for (int s = 0; s < 40; ++s) sand.step({1, 0});  // tilt right
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      if (sand.cell(x, y)) TEST_ASSERT_TRUE(x == 7 || sand.cell(x + 1, y));
    }
  }
  TEST_ASSERT_EQUAL_UINT16(10, sand.count());
}

static void test_sand_fills_then_drains() {
  const Layout l = Layout::matrix(8, 8, true);
  lab_sim::Sand sand;
  sand.reset(4, l);
  bool drained = false, refilled = false;
  for (int s = 0; s < 4000; ++s) {
    sand.step(kDown);
    drained = drained || sand.draining();
    refilled = refilled || (drained && !sand.draining() && sand.count() > 0);
  }
  TEST_ASSERT_TRUE(drained && refilled);
}

static void test_marble_rolls_bounces_and_slows() {
  const Layout l = Layout::matrix(8, 8, true);
  lab_sim::Marble m;
  m.reset(2, l);
  m.place(4, 4, 0, 0);
  for (int s = 0; s < 10; ++s) m.step(kDown);
  TEST_ASSERT_TRUE(m.speed() > 1 && m.y() > 4);  // gravity pulls it down
  m.place(2, 4, 5, 0);
  for (int s = 0; s < 30; ++s) m.step({0, 0});
  TEST_ASSERT_TRUE(m.speed() < 5);  // friction
  lab_sim::Rng rng(8);
  lab_sim::Input in;
  for (int s = 0; s < 5000; ++s) {
    if (s % 60 == 0) in = {2 * rng.unit() - 1, 2 * rng.unit() - 1};
    m.step(in);
    TEST_ASSERT_TRUE(m.x() >= 0.5f && m.x() <= 7.5f && m.y() >= 0.5f && m.y() <= 7.5f);
  }
}

// Every sim writes exactly layout.size() pixels and lights something, on three layout shapes.
static void test_every_sim_draws_in_bounds() {
  const Layout layouts[] = {Layout::matrix(8, 8, true), Layout::matrix(8, 8, true, true),
                            Layout::matrix(5, 3, false)};
  const Pixel sentinel(1, 2, 3);
  for (const Layout& l : layouts) {
    lab_sim::Life life;
    lab_sim::Ant ant;
    lab_sim::Snake snake;
    lab_sim::Tetris tetris;
    lab_sim::Sand sand;
    lab_sim::Marble marble;
    life.reset(1, l);
    ant.reset(1, l);
    snake.reset(1, l);
    tetris.reset(1, l);
    sand.reset(1, l);
    marble.reset(1, l);
    bool lit[6] = {};
    for (int s = 0; s < 200; ++s) {
      Pixel px[6][64 + 4];
      for (auto& frame : px) {
        for (Pixel& p : frame) p = sentinel;
      }
      life.step(kDown);
      ant.step(kDown);
      snake.step(kDown);
      tetris.step(kDown);
      sand.step(kDown);
      marble.step(kDown);
      life.draw(px[0], l);
      ant.draw(px[1], l);
      snake.draw(px[2], l);
      tetris.draw(px[3], l);
      sand.draw(px[4], l);
      marble.draw(px[5], l);
      for (int k = 0; k < 6; ++k) {
        for (uint16_t i = 0; i < l.size(); ++i) lit[k] = lit[k] || !(px[k][i] == kBlack);
        for (uint16_t i = l.size(); i < l.size() + 4; ++i) TEST_ASSERT_TRUE(px[k][i] == sentinel);
      }
    }
    for (int k = 0; k < 6; ++k) {
      if (k == 3 && (l.width() < 4 || l.height() < 4)) continue;  // no well smaller than a piece
      TEST_ASSERT_TRUE(lit[k]);
    }
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_life_rules_blinker_block_glider);
  RUN_TEST(test_life_reseeds_after_dying_or_repeating);
  RUN_TEST(test_ants_follow_the_rule);
  RUN_TEST(test_snake_never_hits_itself_and_grows_when_it_eats);
  RUN_TEST(test_tetris_collapse_removes_full_rows);
  RUN_TEST(test_tetris_plays_clean_and_clears_lines);
  RUN_TEST(test_tetris_restarts_when_topped_out);
  RUN_TEST(test_sand_quantizes_gravity);
  RUN_TEST(test_sand_falls_settles_and_follows_gravity);
  RUN_TEST(test_sand_fills_then_drains);
  RUN_TEST(test_marble_rolls_bounces_and_slows);
  RUN_TEST(test_every_sim_draws_in_bounds);
  return UNITY_END();
}
