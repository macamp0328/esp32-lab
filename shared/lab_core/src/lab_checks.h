#pragma once
// Compile-time project rules. Include in src/main.cpp after <FastLED.h> and config.h.

#include "lab_board.h"

#if defined(FASTLED_ESP32_I2S) || defined(FASTLED_ESP32_USE_CLOCKLESS_SPI)
#error "Clockless LEDs use FastLED's RMT driver; I2S stays free for audio (AGENTS.md)"
#endif
#if LED_RGBW
#error "LED_RGBW 1 is not supported yet: the template sends 3-byte RGB frames"
#endif
#if AUDIO_MODE != 0
#error "AUDIO_MODE != 0 needs lab_audio (milestone M3); keep it 0 for now"
#endif
#if OTA_ENABLED && !WIFI_ENABLED
#error "OTA_ENABLED 1 needs WIFI_ENABLED 1"
#endif
#if WIFI_ENABLED && !__has_include("lab_net.h")
#error "WIFI_ENABLED 1: delete `lib_ignore = lab_net` from this project's platformio.ini"
#endif

#if !defined(POWER_VOLTS) || !defined(POWER_MAX_MA)
#error "config.h must define POWER_VOLTS and POWER_MAX_MA"
#endif
#if POWER_MAX_MA > 2000 && !defined(POWER_README_TABLE)
#error "POWER_MAX_MA > 2000 needs a README power table naming supply and fuse; then define it"
#endif
#if MATRIX_W > 0 && MATRIX_W * MATRIX_H != NUM_LEDS
#error "MATRIX_W * MATRIX_H must equal NUM_LEDS"
#endif
static_assert(lab_board::led_pin_ok(LED_DATA_PIN), "LED_DATA_PIN is not in BOARD_SAFE_PINS");
