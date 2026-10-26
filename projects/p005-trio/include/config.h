#pragma once
// p005-trio: the two S3 Minis and the M5StickC Plus as one piece. One law (each nudges toward or
// away from the other two) runs at three tempos: the flash beat, the color mood, and a loom that
// records them. Same firmware on every board; pins resolve through boards/<name>/board.h.
#include "lab_board.h"

#define LAB_PROJECT "p005-trio"

// LEDs: the S3s show their own onboard pixel; the M5 drives the p002/p004 matrix (Grove white,
// G33, wired down columns; hold it with the cable at the top).
#define LED_TYPE WS2812B
#define LED_RGBW 0
#ifdef BOARD_ONBOARD_RGB
#define NUM_LEDS          1
#define LED_DATA_PIN      BOARD_ONBOARD_RGB
#define LED_ORDER         BOARD_ONBOARD_RGB_ORDER
#define MATRIX_W          0
#define MATRIX_H          0
#define MATRIX_SERPENTINE 0
#define MATRIX_COLUMNS    0
#define POWER_MAX_MA      100  // one pixel: ~60 mA at full white
#else
#define NUM_LEDS          64
#define LED_DATA_PIN      BOARD_LED_PIN_ALT
#define LED_ORDER         GRB
#define MATRIX_W          8
#define MATRIX_H          8
#define MATRIX_SERPENTINE 1
#define MATRIX_COLUMNS    1
#define POWER_MAX_MA      400  // Grove 5 V on laptop USB; the M5 itself takes ~145 mA
#endif
#define POWER_VOLTS 5

// Audio: the M5's PDM mic hears claps; the S3s have no mic.
#if BOARD_HAS_PDM_MIC
#define AUDIO_MODE 2
#else
#define AUDIO_MODE 0
#endif
#define MIC_RATE_HZ 16000
#define MIC_BLOCK   256  // 16 ms per block

// Connectivity: no router. The radio belongs to lab_link (ESP-NOW broadcast on one channel).
#define WIFI_ENABLED 0
#define OTA_ENABLED  0
#define MDNS_NAME    "p005-trio"
#define LINK_CHANNEL 1
#define LINK_TX_DBM  8   // 2..20; low enough that a hand in the gap moves the RSSI
#define LINK_BEAT_HZ 20  // beats per second from every board

// Beat: flash phase; natural period 1 s, spread +-BEAT_SPREAD by MAC; K > 0 unison, K < 0 waltz
#define BEAT_PERIOD_MS 1000
#define BEAT_SPREAD    0.01f  // waltz settles ~2.7 deg off even at K 0.3 (test_trio)
#define BEAT_K         0.3f   // 1/s; unison and waltz both settle within 10 s
#define BEAT_BOOT_MODE 0      // 0 unison, 1 waltz (the M5 decides; A hold flips it)
#define FLASH_MS       140    // flash decay time constant
#define CLAP_GAP_MS    300    // claps closer than this are one clap

// Layout: which S3 the matrix draws bottom-left (0: the lower MAC)
#define TRIO_SWAP_AB 0
