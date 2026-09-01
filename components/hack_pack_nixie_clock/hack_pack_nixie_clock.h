#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "esphome/core/automation.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_effect.h"

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_TEXT
#include "esphome/components/text/text.h"
#endif
#ifdef USE_DATETIME_TIME
#include "esphome/components/datetime/time_entity.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

#include <string>
#include <cstdint>

namespace esphome {
namespace hack_pack_nixie_clock {

enum DisplayMode : uint8_t {
  MODE_TIME = 0,
  MODE_TIMER,
  MODE_ALARM,
  MODE_CUSTOM_TEXT,
  MODE_FACE,
  MODE_SLOT_MACHINE,
  MODE_OFF
};

enum ColorMode : uint8_t {
  COLOR_RAINBOW = 0,
  COLOR_SOLID,
  COLOR_GRADIENT,
  COLOR_FLOW,
  COLOR_WIPE,
  COLOR_PULSE,
  COLOR_BOUNCE
};

enum ColonMode : uint8_t {
  COLON_AUTO_BLEND = 0,
  COLON_MATCH_UNDERGLOW,
  COLON_FIXED
};

enum CustomTextPhase : uint8_t {
  TEXT_PHASE_IDLE = 0,
  TEXT_PHASE_BLANK_START,
  TEXT_PHASE_FLASH_ALERT,
  TEXT_PHASE_SCROLL,
  TEXT_PHASE_BLANK_END
};

enum class LightType : uint8_t {
  PANEL = 0,
  UNDERGLOW = 1
};

#ifdef USE_SWITCH
enum class SwitchType : uint8_t {
  FORMAT_24HR,
  LEADING_ZERO,
  COLON_BLINKING,
  AM_PM_INDICATORS,
  PERIODIC_FACES,
  PHYSICAL_BUTTONS,
  LINK_BRIGHTNESS,
  ALARM_ENABLED,
  RECORD_SOUND,
};
class HackPackNixieSwitch;
#endif

#ifdef USE_BUTTON
enum class ButtonType : uint8_t {
  START_TIMER,
  STOP_TIMER,
  STOP_ALARM,
  TRIGGER_FACE,
  TRIGGER_SLOT_MACHINE,
  PLAY_SOUND,
};
class HackPackNixieButton;
#endif

#ifdef USE_SELECT
enum class SelectType : uint8_t {
  DISPLAY_MODE,
  COLON_COLOR_MODE,
};
class HackPackNixieSelect;
#endif

#ifdef USE_NUMBER
enum class NumberType : uint8_t {
  TIMER_DURATION_MINUTES,
};
class HackPackNixieNumber;
#endif

#ifdef USE_TEXT
enum class TextType : uint8_t {
  TIMER_DURATION,
};
class HackPackNixieText;
#endif

#ifdef USE_DATETIME_TIME
enum class TimeType : uint8_t {
  ALARM_TIME,
};
class HackPackNixieTime;
#endif

struct ButtonState {
  uint8_t pin;
  bool last_reading;
  bool is_pressed;
  uint32_t press_start;
  bool long_press_handled;
};

struct NixieClockStorage {
  bool hr24_mode{false};
  bool show_lead_zero{false};
  bool colon_blinking{false};
  bool am_pm_enabled{true};
  bool face_anim_enabled{false};
  bool physical_buttons_enabled{true};
  bool link_brightness{false};
  bool alarm_enabled{false};
  uint8_t alarm_hour{7};
  uint8_t alarm_minute{0};
  uint8_t colon_mode{0};
  uint32_t timer_duration_sec{300};
} __attribute__((packed));

class HackPackNixieClock;

class HackPackNixieLightListener : public light::LightRemoteValuesListener {
 public:
  HackPackNixieLightListener(HackPackNixieClock *parent, LightType type) : parent_(parent), type_(type) {}
  void on_light_remote_values_update() override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  LightType type_{LightType::PANEL};
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
#ifdef USE_SENSOR
  void set_timer_remaining_sensor(sensor::Sensor *s) {
    sensor_timer_remaining_sec_ = s;
    if (s != nullptr) s->publish_state(timer_remaining_sec_);
  }
#endif
#ifdef USE_TEXT_SENSOR
  void set_timer_remaining_text_sensor(text_sensor::TextSensor *s) {
    text_sensor_timer_remaining_ = s;
    if (s != nullptr) s->publish_state("00:00:00");
  }
  void set_active_display_chars_text_sensor(text_sensor::TextSensor *s) {
    text_sensor_active_display_chars_ = s;
    if (s != nullptr) s->publish_state(std::string(current_display_chars_));
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
  select::Select *sel_display_mode_{nullptr};
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
#ifdef USE_SENSOR
  sensor::Sensor *sensor_timer_remaining_sec_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *text_sensor_timer_remaining_{nullptr};
  text_sensor::TextSensor *text_sensor_active_display_chars_{nullptr};
#endif
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *bs_alarm_ringing_{nullptr};
  binary_sensor::BinarySensor *bs_timer_running_{nullptr};
  binary_sensor::BinarySensor *bs_timer_ringing_{nullptr};
#endif

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
  void update_colons_(uint32_t base_ug);
  void update_ampm_indicators_(const ESPTime &now_time);
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

// =============================================================================
// Light Output Sub-component (Exposes Native Home Assistant Color Pickers)
// =============================================================================
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

// =============================================================================
// Native Built-in Light Effects
// =============================================================================
class HackPackNixieEffect : public light::LightEffect {
 public:
  HackPackNixieEffect(const std::string &name, ColorMode mode, HackPackNixieClock *parent)
      : light::LightEffect(nullptr), name_holder_(name), mode_(mode), parent_(parent) {
    this->name_ = this->name_holder_.c_str();
  }
  HackPackNixieEffect(const char *name, ColorMode mode, HackPackNixieClock *parent)
      : light::LightEffect(name), mode_(mode), parent_(parent) {}

  void apply() override {
    if (this->parent_ != nullptr) {
      this->parent_->set_color_mode(this->mode_);
    }
  }

 protected:
  std::string name_holder_;
  ColorMode mode_;
  HackPackNixieClock *parent_{nullptr};
};

// =============================================================================
// Switch Entity Sub-platform
// =============================================================================
#ifdef USE_SWITCH
class HackPackNixieSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(SwitchType type) { type_ = type; }
  void setup() override;
  void write_state(bool state) override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  SwitchType type_{SwitchType::FORMAT_24HR};
};
#endif

// =============================================================================
// Button Entity Sub-platform
// =============================================================================
#ifdef USE_BUTTON
class HackPackNixieButton : public button::Button, public Component {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(ButtonType type) { type_ = type; }
  void press_action() override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  ButtonType type_{ButtonType::START_TIMER};
};
#endif

// =============================================================================
// Select Entity Sub-platform
// =============================================================================
#ifdef USE_SELECT
class HackPackNixieSelect : public select::Select, public Component {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(SelectType type) { type_ = type; }
  void setup() override;
  void control(const std::string &value) override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  SelectType type_{SelectType::DISPLAY_MODE};
};
#endif

// =============================================================================
// Number Entity Sub-platform
// =============================================================================
#ifdef USE_NUMBER
class HackPackNixieNumber : public number::Number, public Component {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(NumberType type) { type_ = type; }
  void setup() override;
  void control(float value) override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  NumberType type_{NumberType::TIMER_DURATION_MINUTES};
};
#endif

// =============================================================================
// Text Entity Sub-platform
// =============================================================================
#ifdef USE_TEXT
class HackPackNixieText : public text::Text, public Component {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(TextType type) { type_ = type; }
  void setup() override;
  void control(const std::string &value) override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  TextType type_{TextType::TIMER_DURATION};
};
#endif

// =============================================================================
// Time Entity Sub-platform
// =============================================================================
#ifdef USE_DATETIME_TIME
class HackPackNixieTime : public datetime::TimeEntity, public Component {
 public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(TimeType type) { type_ = type; }
  void setup() override;
  void control(const datetime::TimeCall &call) override;

 protected:
  HackPackNixieClock *parent_{nullptr};
  TimeType type_{TimeType::ALARM_TIME};
};
#endif

// =============================================================================
// Automation Actions
// =============================================================================
template<typename... Ts>
class DisplayTextAction : public Action<Ts...> {
 public:
  DisplayTextAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, message)
  TEMPLATABLE_VALUE(uint32_t, scroll_speed_ms)

  void play(const Ts &...x) override {
    auto msg = this->message_.value(x...);
    auto speed = this->scroll_speed_ms_.value(x...);
    this->parent_->show_scrolling_text(msg, speed > 0 ? speed : 350);
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class TriggerSlotMachineAction : public Action<Ts...> {
 public:
  TriggerSlotMachineAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint32_t, duration_ms)

  void play(const Ts &...x) override {
    auto dur = this->duration_ms_.value(x...);
    this->parent_->trigger_slot_machine(dur > 0 ? dur : 2500);
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class TriggerFaceAnimationAction : public Action<Ts...> {
 public:
  TriggerFaceAnimationAction(HackPackNixieClock *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->trigger_face_animation();
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class PlaySoundAction : public Action<Ts...> {
 public:
  PlaySoundAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint32_t, duration_ms)

  void play(const Ts &...x) override {
    auto dur = this->duration_ms_.value(x...);
    this->parent_->play_beep(dur > 0 ? dur : 150);
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class StartTimerAction : public Action<Ts...> {
 public:
  StartTimerAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, duration)

  void play(const Ts &...x) override {
    if (this->duration_.has_value()) {
      auto dur = this->duration_.value(x...);
      if (!dur.empty()) {
        this->parent_->set_timer_duration_string(dur);
      }
    }
    this->parent_->start_timer();
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class StopTimerAction : public Action<Ts...> {
 public:
  StopTimerAction(HackPackNixieClock *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->stop_timer();
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class StopAlarmAction : public Action<Ts...> {
 public:
  StopAlarmAction(HackPackNixieClock *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->stop_alarm();
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class ArmAlarmAction : public Action<Ts...> {
 public:
  ArmAlarmAction(HackPackNixieClock *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->arm_alarm();
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class DisarmAlarmAction : public Action<Ts...> {
 public:
  DisarmAlarmAction(HackPackNixieClock *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->disarm_alarm();
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class SetAlarmAction : public Action<Ts...> {
 public:
  SetAlarmAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint8_t, hour)
  TEMPLATABLE_VALUE(uint8_t, minute)

  void play(const Ts &...x) override {
    this->parent_->set_alarm(this->hour_.value(x...), this->minute_.value(x...));
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class SetDisplayModeAction : public Action<Ts...> {
 public:
  SetDisplayModeAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(DisplayMode, mode)

  void play(const Ts &...x) override {
    this->parent_->set_display_mode(this->mode_.value(x...));
  }

 protected:
  HackPackNixieClock *parent_;
};

template<typename... Ts>
class SetColorModeAction : public Action<Ts...> {
 public:
  SetColorModeAction(HackPackNixieClock *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(ColorMode, mode)

  void play(const Ts &...x) override {
    this->parent_->set_color_mode(this->mode_.value(x...));
  }

 protected:
  HackPackNixieClock *parent_;
};

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
