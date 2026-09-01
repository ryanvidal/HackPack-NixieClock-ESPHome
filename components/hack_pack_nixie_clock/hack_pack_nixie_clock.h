#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/time/real_time_clock.h"
#include <string>
#include <cstdint>

namespace esphome {
namespace hack_pack_nixie_clock {

enum DisplayMode {
  MODE_TIME = 0,
  MODE_TIMER,
  MODE_ALARM,
  MODE_CUSTOM_TEXT,
  MODE_FACE,
  MODE_SLOT_MACHINE,
  MODE_OFF
};

enum ColorMode {
  COLOR_RAINBOW = 0,
  COLOR_SOLID,
  COLOR_GRADIENT,
  COLOR_FLOW,
  COLOR_WIPE,
  COLOR_PULSE,
  COLOR_BOUNCE
};

enum ColonMode {
  COLON_AUTO_BLEND = 0,
  COLON_MATCH_UNDERGLOW,
  COLON_FIXED
};

struct ButtonState {
  uint8_t pin;
  bool last_reading;
  bool is_pressed;
  uint32_t press_start;
  bool long_press_handled;
};

class HackPackNixieClock : public Component {
 public:
  HackPackNixieClock();

  // Pin Configuration
  void set_panel_pin(uint8_t pin) { panel_pin_ = pin; }
  void set_underglow_pin(uint8_t pin) { underglow_pin_ = pin; }
  void set_play_pin(uint8_t pin) { play_pin_ = pin; }
  void set_rec_pin(uint8_t pin) { rec_pin_ = pin; }

  void set_btn_top_pin(uint8_t pin) { btn_top_.pin = pin; }
  void set_btn_center_pin(uint8_t pin) { btn_center_.pin = pin; }
  void set_btn_up_pin(uint8_t pin) { btn_up_.pin = pin; }
  void set_btn_down_pin(uint8_t pin) { btn_down_.pin = pin; }
  void set_btn_left_pin(uint8_t pin) { btn_left_.pin = pin; }
  void set_btn_right_pin(uint8_t pin) { btn_right_.pin = pin; }

  void set_time_source(time::RealTimeClock *time_source) { time_source_ = time_source; }

  // Component Lifecycle
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // Public Controls & Actions
  void set_display_mode(DisplayMode mode);
  void set_color_mode(ColorMode mode);
  void set_colon_mode(ColonMode mode) { colon_mode_ = mode; }
  
  void set_24hr_mode(bool enable) { hr24_mode_ = enable; }
  void set_leading_zero(bool enable) { show_lead_zero_ = enable; }
  void set_colon_blinking(bool enable) { colon_blinking_ = enable; }
  void set_am_pm_indicators(bool enable) { am_pm_enabled_ = enable; }
  void set_face_animations(bool enable) { face_anim_enabled_ = enable; }
  void set_physical_buttons(bool enable) { physical_buttons_enabled_ = enable; }

  void set_panel_brightness(uint8_t brightness) { panel_brightness_ = brightness; }
  void set_underglow_brightness(uint8_t brightness) { underglow_brightness_ = brightness; }
  void set_panel_color_pos(uint8_t pos) { panel_color_pos_ = pos; anim_mode_changed_ = true; use_panel_custom_rgb_ = false; }
  void set_underglow_color_pos(uint8_t pos) { underglow_color_pos_ = pos; use_ug_custom_rgb_ = false; }
  
  void set_panel_rgb(uint8_t r, uint8_t g, uint8_t b);
  void set_underglow_rgb(uint8_t r, uint8_t g, uint8_t b);

  // Alarm & Timer Controls
  void set_alarm(uint8_t hour, uint8_t minute);
  void arm_alarm() { alarm_enabled_ = true; }
  void disarm_alarm();
  void stop_alarm();

  void start_timer(uint32_t hours, uint32_t minutes, uint32_t seconds);
  void stop_timer();

  // Sequences & Audio
  void play_beep(uint32_t duration_ms = 150);
  void set_record_sound(bool active);
  void show_custom_text(const std::string &text, uint32_t duration_seconds = 5);
  void trigger_face_animation();
  void trigger_slot_machine(uint32_t duration_ms = 2500);

  // Status Getters
  bool is_alarm_ringing() const { return alarm_ringing_; }
  bool is_timer_running() const { return timer_running_; }
  bool is_timer_ringing() const { return timer_ringing_; }
  uint32_t get_timer_remaining_sec() const { return timer_remaining_sec_; }
  const char* get_current_display_text() const { return current_display_chars_; }

 protected:
  // Hardware Pins
  uint8_t panel_pin_{0};
  uint8_t underglow_pin_{1};
  uint8_t play_pin_{5};
  uint8_t rec_pin_{4};

  ButtonState btn_top_{19, true, false, 0, false};
  ButtonState btn_center_{10, true, false, 0, false};
  ButtonState btn_up_{9, true, false, 0, false};
  ButtonState btn_down_{8, true, false, 0, false};
  ButtonState btn_left_{7, true, false, 0, false};
  ButtonState btn_right_{6, true, false, 0, false};

  time::RealTimeClock *time_source_{nullptr};

  // State Variables
  DisplayMode display_mode_{MODE_TIME};
  DisplayMode return_mode_{MODE_TIME};
  ColorMode color_mode_{COLOR_RAINBOW};
  ColonMode colon_mode_{COLON_AUTO_BLEND};

  bool hr24_mode_{false};
  bool show_lead_zero_{false};
  bool colon_blinking_{false};
  bool am_pm_enabled_{true};
  bool face_anim_enabled_{false};
  bool physical_buttons_enabled_{true};

  uint8_t panel_brightness_{255};
  uint8_t underglow_brightness_{255};
  uint8_t panel_color_pos_{200};
  uint8_t underglow_color_pos_{200};

  bool use_panel_custom_rgb_{false};
  bool use_ug_custom_rgb_{false};
  uint8_t custom_r_{255}, custom_g_{140}, custom_b_{0};
  uint8_t ug_custom_r_{255}, ug_custom_g_{140}, ug_custom_b_{0};

  bool alarm_enabled_{false};
  uint8_t alarm_hour_{7};
  uint8_t alarm_minute_{0};
  bool alarm_ringing_{false};
  uint32_t alarm_ring_start_{0};

  bool timer_running_{false};
  bool timer_ringing_{false};
  uint32_t timer_duration_sec_{300};
  uint32_t timer_end_millis_{0};
  uint32_t timer_ring_start_{0};
  uint32_t timer_remaining_sec_{0};

  // Sequence tracking
  uint32_t custom_text_expiry_{0};
  uint32_t slot_machine_end_{0};
  uint32_t last_slot_step_{0};
  uint8_t slot_digit_{0};

  uint32_t next_periodic_face_{0};
  uint8_t face_step_{0};
  uint32_t face_step_end_{0};

  uint32_t beep_end_{0};

  // Rendering Buffers
  bool panel_segments_[6][7];
  uint8_t panel_rgb_[6][7][3];
  char current_display_chars_[7];
  uint8_t underglow_rgb_[13][3];
  bool colon_blink_state_{false};
  uint8_t last_sec_{255};

  // Animation Engine
  uint32_t last_anim_update_{0};
  int anim_step_{0};
  int anim_total_steps_{255};
  int anim_frame_ms_{40};
  bool anim_mode_changed_{true};

  bool wipe_col_{true};
  int wipe_index_{0};
  int pulse_index_{0};
  bool pulse_dir_{true};
  int bounce_index_{0};
  bool bounce_dir_{true};

  uint32_t strt_col1_{0}, strt_col2_{0};
  uint32_t end_col1_{0}, end_col2_{0};
  uint32_t now_col1_{0}, now_col2_{0};

  // Internal Logic & Render Helpers
  void init_hardware_();
  void poll_buttons_();
  void handle_button_(ButtonState &btn, void (HackPackNixieClock::*on_click)(), void (HackPackNixieClock::*on_long_press)());

  void btn_top_click_();
  void btn_top_long_();
  void btn_center_click_();
  void btn_center_long_();
  void btn_up_click_();
  void btn_down_click_();
  void btn_left_click_();
  void btn_right_click_();

  void map_char_to_segments_(char val, bool out7[7]) const;
  void set_display_chars_(char c0, char c1, char c2, char c3, char c4, char c5);
  void show_string_(const char *s);

  void setup_animation_();
  void step_animation_();
  void on_animation_complete_();
  void update_colors_();

  void update_display_state_();
  void update_underglow_();
  void render_hardware_leds_();

  uint32_t average_panel_color_(int panelIndex) const;
  uint32_t blend_two_panels_(int p1, int p2) const;

  static uint32_t wheel_(uint8_t pos);
  static uint8_t red_(uint32_t c) { return (c >> 16) & 0xFF; }
  static uint8_t green_(uint32_t c) { return (c >> 8) & 0xFF; }
  static uint8_t blue_(uint32_t c) { return c & 0xFF; }
  static uint32_t make_rgb_(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }
  static uint32_t color_fade_(uint32_t c1, uint32_t c2, int step, int maxSteps);
};

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
