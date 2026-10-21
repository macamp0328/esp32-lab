#pragma once
// Stateful simulations for LED layouts. Each is a caller-owned struct:
//   sim.reset(seed, layout); then per tick sim.step(input); per frame sim.draw(px, layout).
// Intended tick rates: Life 6 Hz, Ant 20 Hz, Snake 8 Hz, Tetris 8 Hz, Sand 30 Hz, Marble 60 Hz.

#include "lab_ant.h"
#include "lab_life.h"
#include "lab_marble.h"
#include "lab_sand.h"
#include "lab_sim_base.h"
#include "lab_snake.h"
#include "lab_tetris.h"
