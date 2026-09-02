#pragma once

#include "types.h"
#include <cstdint>
#include <memory>

namespace esphome {
namespace hack_pack_nixie_clock {

class HackPackNixieClock;

/**
 * @brief Abstract strategy base class for tube panel LED color animation modes.
 * Encapsulates mode-specific timing, step generation, and cycle transitions
 * polymorphically.
 */
class ColorModeAnimation {
public:
  virtual ~ColorModeAnimation() = default;

  /// @brief Initialize or re-randomize color targets when activating this mode.
  virtual void setup(HackPackNixieClock *clock) {}

  /// @brief Calculate and write RGB values to panel buffers for the current
  /// animation frame.
  virtual void step(HackPackNixieClock *clock, int step, int total_steps) = 0;

  /// @brief Lifecycle hook called when the step counter reaches
  /// get_total_steps().
  virtual void on_complete(HackPackNixieClock *clock) {}

  /// @brief Frame interval in milliseconds between animation steps.
  virtual uint32_t get_frame_ms() const { return 40; }

  /// @brief Total number of steps in a full animation loop.
  virtual int get_total_steps() const { return 255; }
};

/// @brief Factory method returning the appropriate polymorphic animation
/// instance for a ColorMode.
std::unique_ptr<ColorModeAnimation> create_color_animation(ColorMode mode);

} // namespace hack_pack_nixie_clock
} // namespace esphome
