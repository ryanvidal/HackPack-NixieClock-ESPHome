#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_effect.h"
#include "types.h"
#include <string>

namespace esphome {
namespace hack_pack_nixie_clock {

class HackPackNixieClock;

/// @brief Bridge between ESPHome native RGB light entities and Nixie hardware channels.
class HackPackNixieLightOutput : public light::LightOutput {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_light_type(LightType type) { type_ = type; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }

  void write_state(light::LightState *state) override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  LightType type_{LightType::PANEL};
};

/// @brief Built-in light effect that delegates animation modes to HackPackNixieClock.
class HackPackNixieEffect : public light::LightEffect {
 public:
  HackPackNixieEffect(const std::string &name, ColorMode mode, HackPackNixieClock *parent)
      : light::LightEffect(nullptr), name_holder_(name), mode_(mode), parent_(parent) {
    this->name_ = this->name_holder_.c_str();
  }
  HackPackNixieEffect(const char *name, ColorMode mode, HackPackNixieClock *parent)
      : light::LightEffect(name), mode_(mode), parent_(parent) {}

  void apply() override;

 protected:
  std::string name_holder_;
  ColorMode mode_{COLOR_RAINBOW};
  HackPackNixieClock *parent_{nullptr};
};

/// @brief Listener that synchronizes linked brightness between Panel and Underglow channels.
class HackPackNixieLightListener : public light::LightRemoteValuesListener {
 public:
  HackPackNixieLightListener(HackPackNixieClock *parent, LightType type) : parent_(parent), type_(type) {}
  void on_light_remote_values_update() override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  LightType type_{LightType::PANEL};
};

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
