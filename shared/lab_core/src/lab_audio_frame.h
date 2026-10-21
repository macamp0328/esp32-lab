#pragma once
// One frame of audio features handed to every effect. Produced by lab_audio (M3); silence by
// default.

namespace lab {

struct AudioFrame {
  float level = 0;  // overall loudness, 0..1
  float bass = 0;   // band energies, 0..1
  float mid = 0;
  float treble = 0;
  bool beat = false;  // true on the frame a beat is detected
};

}  // namespace lab
