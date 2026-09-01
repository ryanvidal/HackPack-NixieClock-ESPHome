#pragma once

#include <cstdint>
#include <cstddef>

namespace esphome {
namespace hack_pack_nixie_clock {

/// @brief Precomputed 256-entry Perceptual Gamma 2.4 lookup table with non-zero minimum floor.
extern const uint8_t GAMMA8_TABLE[256];

/**
 * @brief High-precision, cycle-accurate WS2812 bit-banging transmitter for ESP32-C3.
 * Transmits a raw byte stream with tight interrupt locking per pin to avoid FIFO underrun.
 * 
 * @param pin The GPIO output pin number.
 * @param data Pointer to the GRB byte array.
 * @param len Number of bytes to transmit.
 */
void ws2812_transmit(uint8_t pin, const uint8_t *data, size_t len);

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
