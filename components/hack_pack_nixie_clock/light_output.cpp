#include "light_output.h"
#include "hack_pack_nixie_clock.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace hack_pack_nixie_clock {

void HackPackNixieEffect::apply() {
  if (this->parent_ != nullptr) {
    this->parent_->set_color_mode(this->mode_);
  }
}

void HackPackNixieLightListener::on_light_remote_values_update() {
  if (!parent_ || parent_->is_syncing_light()) return;
  if (!parent_->get_link_brightness()) return;

  light::LightState *source = (type_ == LightType::PANEL) ? parent_->get_panel_light() : parent_->get_underglow_light();
  light::LightState *target = (type_ == LightType::PANEL) ? parent_->get_underglow_light() : parent_->get_panel_light();

  if (source == nullptr || target == nullptr) return;

  bool src_on = source->remote_values.is_on();
  float src_bri = source->remote_values.get_brightness();

  // Scale underglow to 80% of panel brightness to compensate for the panel's darkening film
  float target_bri = (type_ == LightType::PANEL) ? (src_bri * 0.80f) : std::min(1.0f, src_bri / 0.80f);

  bool tgt_on = target->remote_values.is_on();
  float tgt_bri = target->remote_values.get_brightness();

  if (tgt_on == src_on && std::abs(tgt_bri - target_bri) < 0.005f) return;

  parent_->set_syncing_light(true);
  auto call = target->make_call();
  call.set_state(src_on);
  if (src_on) {
    call.set_brightness(target_bri);
  }
  call.set_transition_length(0);
  call.perform();
  parent_->set_syncing_light(false);
}

void HackPackNixieLightOutput::write_state(light::LightState *state) {
  if (!parent_) return;

  float red = 0.0f, green = 0.0f, blue = 0.0f;
  state->current_values_as_rgb(&red, &green, &blue);
  float brightness = state->current_values.get_brightness();
  bool is_on = state->current_values.is_on();

  uint8_t r = (uint8_t)std::round(red * 255.0f);
  uint8_t g = (uint8_t)std::round(green * 255.0f);
  uint8_t b = (uint8_t)std::round(blue * 255.0f);
  uint8_t bri = is_on ? (uint8_t)std::round(brightness * 255.0f) : 0;

  if (type_ == LightType::PANEL) {
    if (is_on) {
      parent_->set_panel_rgb(r, g, b);
      if (parent_->get_display_mode() == MODE_OFF) {
        parent_->set_display_mode(MODE_TIME);
      }
    }
    parent_->set_panel_brightness(bri);
  } else if (type_ == LightType::UNDERGLOW) {
    if (is_on) {
      parent_->set_underglow_rgb(r, g, b);
    }
    parent_->set_underglow_brightness(bri);
  }
}

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
