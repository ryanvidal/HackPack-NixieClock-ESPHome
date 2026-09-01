#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "types.h"
#include "font_map.h"
#include "hardware_leds.h"
#include "light_output.h"
#include "entities.h"
#include "color_animations.h"

#include <string>
#include <cstdint>
#include <memory>

namespace esphome {
namespace hack_pack_nixie_clock {

/**
 * @brief Core controller component for the CrunchLabs Hack Pack Nixie Clock (Box 15).
 * 
 * Manages the dual-bus WS2812 LED rendering pipeline (42 panel LEDs + 13 underglow LEDs),
 * real-time clock rendering, countdown timer, alarm scheduling, animated face sequences,
 * audio beeper, cathode anti-poisoning slot machine, and Home Assistant bi-directional synchronization.
 */
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
  void set_ready_for_ota();

  // Public Controls & Actions
  void set_display_mode(DisplayMode mode);
  void set_color_mode(ColorMode mode);
  void set_colon_mode(ColonMode mode);
  
  void set_24hr_mode(bool enable);
  void set_leading_zero(bool enable);
  void set_colon_blinking(bool enable);
  void set_am_pm_indicators(bool enable);
  void set_face_animations(bool enable);
  void set_physical_buttons(bool enable);
  void set_link_brightness(bool enable);

  void set_panel_brightness(uint8_t brightness);
  void set_underglow_brightness(uint8_t brightness);
  void set_panel_color_pos(uint8_t pos);
  void set_underglow_color_pos(uint8_t pos);
  
  void set_panel_rgb(uint8_t r, uint8_t g, uint8_t b);
  void set_underglow_rgb(uint8_t r, uint8_t g, uint8_t b);

  // Alarm & Timer Controls
  void set_alarm(uint8_t hour, uint8_t minute);
  void arm_alarm();
  void disarm_alarm();
  void stop_alarm();

  void set_timer_duration(uint32_t seconds);
  uint32_t set_timer_duration_string(const std::string &str);
  static uint32_t parse_duration_string(const std::string &input);
  void start_timer(uint32_t hours, uint32_t minutes, uint32_t seconds);
  void start_timer(uint32_t duration_sec = 0);
  void stop_timer();

  // Sequences, Messages & Audio
  void play_beep(uint32_t duration_ms = 150);
  void set_record_sound(bool active);
  void show_custom_text(const std::string &text, uint32_t duration_seconds = 5);
  void show_scrolling_text(const std::string &message, uint32_t scroll_speed_ms = 350);
  void trigger_face_animation();
  void trigger_slot_machine(uint32_t duration_ms = 2500);

  // Status & Property Getters
  DisplayMode get_display_mode() const { return display_mode_; }
  ColorMode get_color_mode() const { return color_mode_; }
  ColonMode get_colon_mode() const { return colon_mode_; }
  
  bool get_24hr_mode() const { return hr24_mode_; }
  bool get_leading_zero() const { return show_lead_zero_; }
  bool get_colon_blinking() const { return colon_blinking_; }
  bool get_am_pm_indicators() const { return am_pm_enabled_; }
  bool get_face_animations() const { return face_anim_enabled_; }
  bool get_physical_buttons() const { return physical_buttons_enabled_; }
  bool get_link_brightness() const { return link_brightness_; }

  uint8_t get_panel_brightness() const { return panel_brightness_; }
  uint8_t get_underglow_brightness() const { return underglow_brightness_; }
  uint8_t get_panel_color_pos() const { return panel_color_pos_; }
  uint8_t get_underglow_color_pos() const { return underglow_color_pos_; }

  uint8_t get_panel_r() const;
  uint8_t get_panel_g() const;
  uint8_t get_panel_b() const;
  uint8_t get_ug_r() const;
  uint8_t get_ug_g() const;
  uint8_t get_ug_b() const;

  bool is_alarm_armed() const { return alarm_enabled_; }
  uint8_t get_alarm_hour() const { return alarm_hour_; }
  uint8_t get_alarm_minute() const { return alarm_minute_; }
  bool is_alarm_ringing() const { return alarm_ringing_; }

  uint32_t get_timer_duration() const { return timer_duration_sec_; }
  bool is_timer_running() const { return timer_running_; }
  bool is_timer_ringing() const { return timer_ringing_; }
  uint32_t get_timer_remaining_sec() const { return timer_remaining_sec_; }
  const char* get_current_display_text() const { return current_display_chars_; }

  // Entity Registrations & Pointers
  void register_light(LightType type, light::LightState *st);
  bool is_syncing_light() const { return syncing_light_; }
  void set_syncing_light(bool s) { syncing_light_ = s; }
  light::LightState *get_panel_light() const { return panel_light_; }
  light::LightState *get_underglow_light() const { return underglow_light_; }

#ifdef USE_SWITCH
  void register_switch(SwitchType type, switch_::Switch *sw);
#endif
#ifdef USE_SELECT
  void register_select(SelectType type, select::Select *sel);
#endif
#ifdef USE_NUMBER
  void register_number(NumberType type, number::Number *num);
#endif
#ifdef USE_TEXT
  void register_text(TextType type, text::Text *txt);
#endif
#ifdef USE_DATETIME_TIME
  void register_time(TimeType type, datetime::TimeEntity *tm);
#endif
#ifdef USE_TEXT_SENSOR
  void set_timer_remaining_text_sensor(text_sensor::TextSensor *s) {
    text_sensor_timer_remaining_ = s;
    if (s != nullptr) s->publish_state("00:00:00");
  }
#endif
#ifdef USE_BINARY_SENSOR
  void set_alarm_ringing_binary_sensor(binary_sensor::BinarySensor *s) {
    bs_alarm_ringing_ = s;
    if (s != nullptr) s->publish_state(alarm_ringing_);
  }
  void set_timer_running_binary_sensor(binary_sensor::BinarySensor *s) {
    bs_timer_running_ = s;
    if (s != nullptr) s->publish_state(timer_running_);
  }
  void set_timer_ringing_binary_sensor(binary_sensor::BinarySensor *s) {
    bs_timer_ringing_ = s;
    if (s != nullptr) s->publish_state(timer_ringing_);
  }
#endif

  // Methods for polymorphic animations & color calculations
  void set_panel_segment_rgb(int panel, int segment, uint8_t r, uint8_t g, uint8_t b) {
    if (panel >= 0 && panel < 6 && segment >= 0 && segment < 7) {
      panel_rgb_[panel][segment][0] = r;
      panel_rgb_[panel][segment][1] = g;
      panel_rgb_[panel][segment][2] = b;
    }
  }
  uint32_t get_panel_base_color() const {
    return use_panel_custom_rgb_ ? make_rgb_(custom_r_, custom_g_, custom_b_) : wheel_(panel_color_pos_);
  }

  static uint32_t wheel_(uint8_t pos);
  static uint8_t red_(uint32_t c) { return (c >> 16) & 0xFF; }
  static uint8_t green_(uint32_t c) { return (c >> 8) & 0xFF; }
  static uint8_t blue_(uint32_t c) { return c & 0xFF; }
  static uint32_t make_rgb_(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }
  static uint32_t color_fade_(uint32_t c1, uint32_t c2, int step, int maxSteps);

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
  bool link_brightness_{false};

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

  // NVS Preferences
  ESPPreferenceObject pref_;
  uint32_t pref_save_timeout_{0};
  uint32_t pref_save_start_{0};
  void schedule_save_preferences_();
  void save_preferences_now_();
  void load_preferences_();

  // Sequence tracking
  uint32_t custom_text_expiry_{0};
  CustomTextPhase text_phase_{TEXT_PHASE_IDLE};
  uint32_t text_phase_end_{0};
  uint32_t flash_toggle_time_{0};
  bool flash_state_{false};
  std::string text_message_{""};
  size_t scroll_pos_{0};
  uint32_t scroll_speed_ms_{350};
  uint32_t scroll_next_step_{0};

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
  uint32_t last_sync_warn_time_{0};

  // Polymorphic Animation Engine
  std::unique_ptr<ColorModeAnimation> current_animation_{nullptr};
  uint32_t last_anim_update_{0};
  int anim_step_{0};

  // Entity Pointers
  light::LightState *panel_light_{nullptr};
  light::LightState *underglow_light_{nullptr};
  HackPackNixieLightListener panel_listener_{this, LightType::PANEL};
  HackPackNixieLightListener underglow_listener_{this, LightType::UNDERGLOW};
  bool syncing_light_{false};

#ifdef USE_SWITCH
  switch_::Switch *sw_format_24hr_{nullptr};
  switch_::Switch *sw_leading_zero_{nullptr};
  switch_::Switch *sw_colon_blinking_{nullptr};
  switch_::Switch *sw_am_pm_indicators_{nullptr};
  switch_::Switch *sw_periodic_faces_{nullptr};
  switch_::Switch *sw_physical_buttons_{nullptr};
  switch_::Switch *sw_link_brightness_{nullptr};
  switch_::Switch *sw_alarm_enabled_{nullptr};
  switch_::Switch *sw_record_sound_{nullptr};
#endif
#ifdef USE_SELECT
  select::Select *sel_colon_color_mode_{nullptr};
#endif
#ifdef USE_NUMBER
  number::Number *num_timer_duration_minutes_{nullptr};
#endif
#ifdef USE_TEXT
  text::Text *txt_timer_duration_{nullptr};
#endif
#ifdef USE_DATETIME_TIME
  datetime::TimeEntity *tm_alarm_time_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *text_sensor_timer_remaining_{nullptr};
#endif
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *bs_alarm_ringing_{nullptr};
  binary_sensor::BinarySensor *bs_timer_running_{nullptr};
  binary_sensor::BinarySensor *bs_timer_ringing_{nullptr};
#endif

  // Internal Logic & Render Helpers
  void init_hardware_();
  void poll_buttons_();
  void handle_button_(ButtonState &btn, const char *name, void (HackPackNixieClock::*on_click)(), void (HackPackNixieClock::*on_long_press)());

  void btn_top_click_();
  void btn_top_long_();
  void btn_center_click_();
  void btn_center_long_();
  void btn_up_click_();
  void btn_down_click_();
  void btn_left_click_();
  void btn_right_click_();

  void set_display_chars_(char c0, char c1, char c2, char c3, char c4, char c5);
  void show_string_(const char *s);

  void step_animation_();

  void update_display_state_();
  void update_underglow_();
  void update_colons_(uint32_t base_ug);
  void update_ampm_indicators_(const ESPTime &now_time);
  void render_hardware_leds_();

  uint32_t average_panel_color_(int panelIndex) const;
  uint32_t blend_two_panels_(int p1, int p2) const;
};

}  // namespace hack_pack_nixie_clock
}  // namespace esphome

#include "automation.h"
