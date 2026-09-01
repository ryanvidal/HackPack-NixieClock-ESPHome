#pragma once

#include <cstdint>
#include <cstddef>

namespace esphome {
namespace hack_pack_nixie_clock {

// Segment mapping layout: {bottom: 0, mid: 1, top: 2, L-lower: 3, L-upper: 4, R-lower: 5, R-upper: 6}
static const char NIXIE_CHARSET_MAP[] = {
  ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '-', '_'
};

static const bool NIXIE_SEGMENT_MAP[][7] = {
  {false, false, false, false, false, false, false}, // ' ' (Space)
  {true,  false, true,  true,  true,  true,  true }, // '0'
  {false, false, false, false, false, true,  true }, // '1'
  {true,  true,  true,  true,  false, false, true }, // '2'
  {true,  true,  true,  false, false, true,  true }, // '3'
  {false, true,  false, false, true,  true,  true }, // '4'
  {true,  true,  true,  false, true,  true,  false}, // '5'
  {true,  true,  true,  true,  true,  true,  false}, // '6'
  {false, false, true,  false, false, true,  true }, // '7'
  {true,  true,  true,  true,  true,  true,  true }, // '8'
  {true,  true,  true,  false, true,  true,  true }, // '9'
  {false, true,  true,  true,  true,  true,  true }, // 'a' (A)
  {true,  true,  false, true,  true,  true,  false}, // 'b'
  {true,  false, true,  true,  true,  false, false}, // 'c' (C)
  {true,  true,  false, true,  false, true,  true }, // 'd'
  {true,  true,  true,  true,  true,  false, false}, // 'e' (E)
  {false, true,  true,  false, true,  false, false}, // 'f' (F)
  {true,  false, true,  true,  true,  true,  false}, // 'g' (G)
  {false, true,  false, true,  true,  true,  true }, // 'h' (H)
  {false, false, false, false, false, true,  true }, // 'i' (I)
  {true,  false, false, true,  false, true,  true }, // 'j' (J)
  {false, true,  false, true,  true,  false, true }, // 'k'
  {true,  false, false, true,  true,  false, false}, // 'l' (L)
  {false, true,  true,  false, false, false, false}, // 'm'
  {false, true,  false, true,  false, true,  false}, // 'n'
  {true,  true,  false, true,  false, true,  false}, // 'o'
  {false, true,  true,  true,  true,  false, true }, // 'p' (P)
  {false, true,  true,  false, true,  true,  true }, // 'q' (Q)
  {false, true,  false, true,  false, false, false}, // 'r'
  {true,  true,  true,  false, true,  true,  false}, // 's' (S)
  {true,  true,  false, true,  true,  false, false}, // 't'
  {true,  false, false, true,  true,  true,  true }, // 'u' (U)
  {true,  false, false, true,  false, true,  false}, // 'v'
  {false, true,  false, false, false, false, false}, // 'w'
  {false, true,  false, true,  false, true,  false}, // 'x'
  {true,  true,  false, false, true,  true,  true }, // 'y' (Y)
  {true,  true,  true,  true,  false, false, true }, // 'z' (Z)
  {false, true,  false, false, false, false, false}, // '-' (Minus)
  {true,  false, false, false, false, false, false}  // '_' (Underscore)
};

/**
 * @brief Converts a character (with case-insensitivity) to its 7-segment representation.
 * @param val The ASCII character to convert.
 * @param out7 Array of 7 booleans receiving the segment state.
 */
inline void map_char_to_segments(char val, bool out7[7]) {
  char lower = (val >= 'A' && val <= 'Z') ? (val + 32) : val;

  for (size_t i = 0; i < sizeof(NIXIE_CHARSET_MAP); ++i) {
    if (NIXIE_CHARSET_MAP[i] == lower || NIXIE_CHARSET_MAP[i] == val) {
      for (int s = 0; s < 7; s++) out7[s] = NIXIE_SEGMENT_MAP[i][s];
      return;
    }
  }
  // Default to blank for unsupported characters
  for (int s = 0; s < 7; s++) out7[s] = false;
}

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
