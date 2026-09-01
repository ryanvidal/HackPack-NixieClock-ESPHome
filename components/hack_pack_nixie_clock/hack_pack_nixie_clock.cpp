#include "hack_pack_nixie_clock.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#if defined(USE_ESP_IDF)
#include "driver/gpio.h"
#include "esp_cpu.h"
#include "esp_attr.h"
#include "soc/gpio_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#elif defined(USE_ARDUINO)
#include <Arduino.h>
#endif

namespace esphome {
namespace hack_pack_nixie_clock {

static const char *const TAG = "hack_pack_nixie_clock";

// 7-Segment Glyphs Lookup Table
static const char NIXIE_CHARSET_MAP[] = {
  ' ', '0','1','2','3','4','5','6','7','8','9',
  'a','b','c','d','e','f','g','h','i','j','k',
  'l','m','n','o','p','q','r','s','t','u','v',
  'w','x','y','z','-','_','^','@','#','<','>','O',
  '!', '?', '.', ':', ',', '=', '/', '\\', '[', ']'
};

static const bool NIXIE_SEGMENT_MAP[][7] = {
  {0, 0, 0, 0, 0, 0, 0}, // ' '
  {1, 0, 1, 1, 1, 1, 1}, // '0'
  {0, 0, 0, 0, 0, 1, 1}, // '1'
  {1, 1, 1, 1, 0, 0, 1}, // '2'
  {1, 1, 1, 0, 0, 1, 1}, // '3'
  {0, 1, 0, 0, 1, 1, 1}, // '4'
  {1, 1, 1, 0, 1, 1, 0}, // '5'
  {1, 1, 1, 1, 1, 1, 0}, // '6'
  {0, 0, 1, 0, 0, 1, 1}, // '7'
  {1, 1, 1, 1, 1, 1, 1}, // '8'
  {0, 1, 1, 0, 1, 1, 1}, // '9'
  {0, 1, 1, 1, 1, 1, 1}, // 'a' 
  {1, 1, 0, 1, 1, 1, 0}, // 'b' 
  {1, 0, 1, 1, 1, 0, 0}, // 'c'
  {1, 0, 0, 1, 0, 1, 1}, // 'd'
  {1, 1, 1, 1, 1, 0, 0}, // 'e'
  {0, 1, 1, 1, 1, 0, 0}, // 'f'
  {1, 0, 1, 1, 1, 1, 0}, // 'g'
  {0, 1, 0, 1, 1, 1, 1}, // 'h'
  {0, 0, 0, 1, 1, 0, 0}, // 'i'
  {1, 0, 0, 1, 0, 1, 1}, // 'j'
  {1, 1, 0, 1, 1, 0, 1}, // 'k'
  {1, 0, 0, 1, 1, 0, 0}, // 'l'
  {1, 0, 1, 0, 1, 0, 1}, // 'm'
  {0, 1, 0, 1, 0, 1, 0}, // 'n'
  {1, 1, 0, 1, 0, 1, 0}, // 'o'
  {0, 1, 1, 1, 1, 0, 1}, // 'p'
  {0, 1, 1, 0, 1, 1, 1}, // 'q'
  {0, 1, 0, 1, 0, 0, 0}, // 'r'
  {1, 1, 1, 0, 1, 1, 0}, // 's'
  {1, 1, 0, 1, 1, 0, 0}, // 't'
  {1, 0, 0, 1, 0, 1, 0}, // 'u'
  {0, 1, 0, 0, 1, 0, 1}, // 'v'
  {1, 1, 0, 0, 1, 0, 1}, // 'w'
  {1, 1, 1, 0, 0, 0, 0}, // 'x'
  {1, 1, 0, 0, 1, 1, 1}, // 'y'
  {1, 0, 1, 1, 0, 0, 1}, // 'z'
  {0, 1, 0, 0, 0, 0, 0}, // '-'
  {1, 0, 0, 0, 0, 0, 0}, // '_'
  {0, 0, 1, 0, 1, 0, 1}, // '^'
  {0, 0, 1, 0, 1, 0, 0}, // '@'
  {0, 0, 1, 0, 0, 0, 1}, // '#'
  {1, 0, 0, 1, 0, 0, 0}, // '<'
  {1, 0, 0, 0, 0, 1, 0}, // '>'
  {0, 1, 1, 0, 1, 0, 1}, // 'O'
  {1, 0, 0, 0, 0, 0, 1}, // '!' (bottom + R-upper)
  {1, 1, 1, 0, 0, 0, 1}, // '?' (bottom + mid + top + R-upper)
  {1, 0, 0, 0, 0, 0, 0}, // '.' (bottom)
  {1, 0, 1, 0, 0, 0, 0}, // ':' (bottom + top)
  {0, 0, 0, 1, 0, 0, 0}, // ',' (L-lower)
  {1, 1, 0, 0, 0, 0, 0}, // '=' (bottom + mid)
  {0, 1, 0, 1, 0, 0, 1}, // '/' (mid + L-lower + R-upper)
  {0, 1, 0, 0, 1, 1, 0}, // '\' (mid + L-upper + R-lower)
  {1, 0, 1, 1, 1, 0, 0}, // '[' (bottom + top + L-lower + L-upper)
  {1, 0, 1, 0, 0, 1, 1}  // ']' (bottom + top + R-lower + R-upper)
};

HackPackNixieClock::HackPackNixieClock() {
  strncpy(current_display_chars_, "      ", sizeof(current_display_chars_));
  for (int p = 0; p < 6; p++) {
    for (int s = 0; s < 7; s++) {
      panel_segments_[p][s] = false;
      panel_rgb_[p][s][0] = 0;
      panel_rgb_[p][s][1] = 0;
      panel_rgb_[p][s][2] = 0;
    }
  }
  for (int i = 0; i < 13; i++) {
    underglow_rgb_[i][0] = 0;
    underglow_rgb_[i][1] = 0;
    underglow_rgb_[i][2] = 0;
  }
}

// =============================================================================
// Lifecycle: setup, loop, dump_config
// =============================================================================
void HackPackNixieClock::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Hack Pack Nixie Clock...");
  init_hardware_();
  load_preferences_();
  setup_animation_();
  next_periodic_face_ = millis() + 30000;

#ifdef USE_BINARY_SENSOR
  if (bs_alarm_ringing_ != nullptr) bs_alarm_ringing_->publish_state(alarm_ringing_);
  if (bs_timer_running_ != nullptr) bs_timer_running_->publish_state(timer_running_);
  if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(timer_ringing_);
#endif
#ifdef USE_SENSOR
  if (sensor_timer_remaining_sec_ != nullptr) sensor_timer_remaining_sec_->publish_state(timer_remaining_sec_);
#endif
#ifdef USE_TEXT_SENSOR
  if (text_sensor_timer_remaining_ != nullptr) text_sensor_timer_remaining_->publish_state("00:00:00");
  if (text_sensor_active_display_chars_ != nullptr) text_sensor_active_display_chars_->publish_state(std::string(current_display_chars_));
#endif
}

void HackPackNixieClock::dump_config() {
  ESP_LOGCONFIG(TAG, "Hack Pack Nixie Clock Configuration:");
  ESP_LOGCONFIG(TAG, "  Panel LED Pin: GPIO%u (42 LEDs)", (unsigned int)panel_pin_);
  ESP_LOGCONFIG(TAG, "  Underglow Pin: GPIO%u (13 LEDs)", (unsigned int)underglow_pin_);
  ESP_LOGCONFIG(TAG, "  Play Pin: GPIO%u", (unsigned int)play_pin_);
  ESP_LOGCONFIG(TAG, "  Rec Pin: GPIO%u", (unsigned int)rec_pin_);
  ESP_LOGCONFIG(TAG, "  Button Top: GPIO%u", (unsigned int)btn_top_.pin);
  ESP_LOGCONFIG(TAG, "  Button Center: GPIO%u", (unsigned int)btn_center_.pin);
  ESP_LOGCONFIG(TAG, "  Button Up: GPIO%u", (unsigned int)btn_up_.pin);
  ESP_LOGCONFIG(TAG, "  Button Down: GPIO%u", (unsigned int)btn_down_.pin);
  ESP_LOGCONFIG(TAG, "  Button Left: GPIO%u", (unsigned int)btn_left_.pin);
  ESP_LOGCONFIG(TAG, "  Button Right: GPIO%u", (unsigned int)btn_right_.pin);
}

void HackPackNixieClock::loop() {
  uint32_t now_ms = millis();

  // 1. Debounced Preference Flash Save
  if (pref_save_timeout_ > 0 && now_ms >= pref_save_timeout_) {
    save_preferences_now_();
  }

  // 2. Poll Physical Buttons
  poll_buttons_();

  // 3. Step Animation Engine
  step_animation_();

  // 4. Update Display & Sequence State
  update_display_state_();

  // 5. Update Underglow & Colons
  update_underglow_();

  // 6. Beep Pulse Management
  if (beep_end_ > 0) {
    if (now_ms < beep_end_) {
#if defined(USE_ESP_IDF)
      gpio_set_level((gpio_num_t)play_pin_, 1);
#else
      digitalWrite(play_pin_, HIGH);
#endif
    } else {
#if defined(USE_ESP_IDF)
      gpio_set_level((gpio_num_t)play_pin_, 0);
#else
      digitalWrite(play_pin_, LOW);
#endif
      beep_end_ = 0;
    }
  }

  // 7. Alarm/Timer Repeating Beeper
  if (alarm_ringing_ || timer_ringing_) {
    uint32_t start_ref = alarm_ringing_ ? alarm_ring_start_ : timer_ring_start_;
    if ((now_ms - start_ref) % 10000 < 100) {
      if (beep_end_ == 0) play_beep(100);
    }
  }

  // 8. Render Hardware LEDs at ~50Hz (20ms intervals)
  static uint32_t last_render = 0;
  if (now_ms - last_render >= 20) {
    last_render = now_ms;
    render_hardware_leds_();
  }
}

// =============================================================================
// NVS Preferences Persistence
// =============================================================================
void HackPackNixieClock::load_preferences_() {
  pref_ = global_preferences->make_preference<NixieClockStorage>(fnv1_hash("hack_pack_nixie_clock_cfg"));
  NixieClockStorage storage;
  if (pref_.load(&storage)) {
    ESP_LOGI(TAG, "Restored Nixie Clock settings from flash memory.");
    hr24_mode_ = storage.hr24_mode;
    show_lead_zero_ = storage.show_lead_zero;
    colon_blinking_ = storage.colon_blinking;
    am_pm_enabled_ = storage.am_pm_enabled;
    face_anim_enabled_ = storage.face_anim_enabled;
    physical_buttons_enabled_ = storage.physical_buttons_enabled;
    link_brightness_ = storage.link_brightness;
    alarm_enabled_ = storage.alarm_enabled;
    alarm_hour_ = storage.alarm_hour;
    alarm_minute_ = storage.alarm_minute;
    colon_mode_ = (ColonMode)(storage.colon_mode % 3);
    timer_duration_sec_ = (storage.timer_duration_sec > 0) ? storage.timer_duration_sec : 300;
  } else {
    ESP_LOGI(TAG, "No saved settings found in flash. Using defaults.");
  }
}

void HackPackNixieClock::schedule_save_preferences_() {
  pref_save_timeout_ = millis() + 500;
}

void HackPackNixieClock::save_preferences_now_() {
  NixieClockStorage storage;
  storage.hr24_mode = hr24_mode_;
  storage.show_lead_zero = show_lead_zero_;
  storage.colon_blinking = colon_blinking_;
  storage.am_pm_enabled = am_pm_enabled_;
  storage.face_anim_enabled = face_anim_enabled_;
  storage.physical_buttons_enabled = physical_buttons_enabled_;
  storage.link_brightness = link_brightness_;
  storage.alarm_enabled = alarm_enabled_;
  storage.alarm_hour = alarm_hour_;
  storage.alarm_minute = alarm_minute_;
  storage.colon_mode = (uint8_t)colon_mode_;
  storage.timer_duration_sec = timer_duration_sec_;
  pref_.save(&storage);
  global_preferences->sync();
  pref_save_timeout_ = 0;
  ESP_LOGD(TAG, "Nixie Clock settings successfully written to flash.");
}

// =============================================================================
// RGB Getters
// =============================================================================
uint8_t HackPackNixieClock::get_panel_r() const {
  return use_panel_custom_rgb_ ? custom_r_ : red_(wheel_(panel_color_pos_));
}
uint8_t HackPackNixieClock::get_panel_g() const {
  return use_panel_custom_rgb_ ? custom_g_ : green_(wheel_(panel_color_pos_));
}
uint8_t HackPackNixieClock::get_panel_b() const {
  return use_panel_custom_rgb_ ? custom_b_ : blue_(wheel_(panel_color_pos_));
}
uint8_t HackPackNixieClock::get_ug_r() const {
  return use_ug_custom_rgb_ ? ug_custom_r_ : red_(wheel_(underglow_color_pos_));
}
uint8_t HackPackNixieClock::get_ug_g() const {
  return use_ug_custom_rgb_ ? ug_custom_g_ : green_(wheel_(underglow_color_pos_));
}
uint8_t HackPackNixieClock::get_ug_b() const {
  return use_ug_custom_rgb_ ? ug_custom_b_ : blue_(wheel_(underglow_color_pos_));
}

// =============================================================================
// Hardware Initialization
// =============================================================================
void HackPackNixieClock::init_hardware_() {
#if defined(USE_ESP_IDF)
  gpio_config_t out_conf{};
  out_conf.intr_type = GPIO_INTR_DISABLE;
  out_conf.mode = GPIO_MODE_OUTPUT;
  out_conf.pin_bit_mask = (1ULL << panel_pin_) | (1ULL << underglow_pin_) | (1ULL << play_pin_) | (1ULL << rec_pin_);
  out_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  out_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&out_conf);

  gpio_set_level((gpio_num_t)panel_pin_, 0);
  gpio_set_level((gpio_num_t)underglow_pin_, 0);
  gpio_set_level((gpio_num_t)play_pin_, 0);
  gpio_set_level((gpio_num_t)rec_pin_, 0);

  gpio_config_t in_conf{};
  in_conf.intr_type = GPIO_INTR_DISABLE;
  in_conf.mode = GPIO_MODE_INPUT;
  in_conf.pin_bit_mask = (1ULL << btn_top_.pin) | (1ULL << btn_center_.pin) |
                          (1ULL << btn_up_.pin) | (1ULL << btn_down_.pin) |
                          (1ULL << btn_left_.pin) | (1ULL << btn_right_.pin);
  in_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  in_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&in_conf);

#elif defined(USE_ARDUINO)
  pinMode(panel_pin_, OUTPUT);
  pinMode(underglow_pin_, OUTPUT);
  pinMode(play_pin_, OUTPUT);
  pinMode(rec_pin_, OUTPUT);

  digitalWrite(panel_pin_, LOW);
  digitalWrite(underglow_pin_, LOW);
  digitalWrite(play_pin_, LOW);
  digitalWrite(rec_pin_, LOW);

  pinMode(btn_top_.pin, INPUT_PULLUP);
  pinMode(btn_center_.pin, INPUT_PULLUP);
  pinMode(btn_up_.pin, INPUT_PULLUP);
  pinMode(btn_down_.pin, INPUT_PULLUP);
  pinMode(btn_left_.pin, INPUT_PULLUP);
  pinMode(btn_right_.pin, INPUT_PULLUP);
#endif
}

// =============================================================================
// Physical Buttons Debounce & Navigation
// =============================================================================
void HackPackNixieClock::handle_button_(ButtonState &btn, void (HackPackNixieClock::*on_click)(), void (HackPackNixieClock::*on_long_press)()) {
#if defined(USE_ESP_IDF)
  bool reading = gpio_get_level((gpio_num_t)btn.pin) == 0; // Inverted / Active LOW
#else
  bool reading = digitalRead(btn.pin) == LOW;
#endif

  uint32_t now = millis();

  if (reading != btn.last_reading) {
    btn.press_start = now;
    btn.last_reading = reading;
  }

  if (reading) {
    if (!btn.is_pressed && (now - btn.press_start >= 50)) {
      btn.is_pressed = true;
      btn.long_press_handled = false;
    }
    if (btn.is_pressed && !btn.long_press_handled && (now - btn.press_start >= 800)) {
      btn.long_press_handled = true;
      if (physical_buttons_enabled_ && on_long_press) {
        (this->*on_long_press)();
      }
    }
  } else {
    if (btn.is_pressed) {
      if (!btn.long_press_handled && (now - btn.press_start < 800)) {
        if (physical_buttons_enabled_ && on_click) {
          (this->*on_click)();
        }
      }
      btn.is_pressed = false;
    }
  }
}

void HackPackNixieClock::poll_buttons_() {
  handle_button_(btn_top_, &HackPackNixieClock::btn_top_click_, &HackPackNixieClock::btn_top_long_);
  handle_button_(btn_center_, &HackPackNixieClock::btn_center_click_, &HackPackNixieClock::btn_center_long_);
  handle_button_(btn_up_, &HackPackNixieClock::btn_up_click_, nullptr);
  handle_button_(btn_down_, &HackPackNixieClock::btn_down_click_, nullptr);
  handle_button_(btn_left_, &HackPackNixieClock::btn_left_click_, nullptr);
  handle_button_(btn_right_, &HackPackNixieClock::btn_right_click_, nullptr);
}

void HackPackNixieClock::btn_top_click_() {
  stop_alarm();
  stop_timer();
}

void HackPackNixieClock::btn_top_long_() {
  disarm_alarm();
  stop_timer();
  set_display_mode(MODE_TIME);
}

void HackPackNixieClock::btn_center_click_() {
  if (display_mode_ == MODE_TIME) {
    set_display_mode(MODE_TIMER);
  } else {
    set_display_mode(MODE_TIME);
  }
}

void HackPackNixieClock::btn_center_long_() {
  trigger_slot_machine(2500);
}

void HackPackNixieClock::btn_up_click_() {
  set_panel_color_pos((panel_color_pos_ + 16) % 256);
}

void HackPackNixieClock::btn_down_click_() {
  set_panel_color_pos((panel_color_pos_ + 240) % 256);
}

void HackPackNixieClock::btn_left_click_() {
  int m = (int)color_mode_ - 1;
  if (m < 0) m = 6;
  set_color_mode((ColorMode)m);
}

void HackPackNixieClock::btn_right_click_() {
  int m = ((int)color_mode_ + 1) % 7;
  set_color_mode((ColorMode)m);
}

// =============================================================================
// Public Controls & Actions
// =============================================================================
void HackPackNixieClock::set_display_mode(DisplayMode mode) {
  display_mode_ = mode;
#ifdef USE_SELECT
  if (sel_display_mode_ != nullptr) {
    const char *mode_str = "Time";
    switch (display_mode_) {
      case MODE_TIME: mode_str = "Time"; break;
      case MODE_TIMER: mode_str = "Timer"; break;
      case MODE_ALARM: mode_str = "Alarm View"; break;
      case MODE_SLOT_MACHINE: mode_str = "Slot Machine"; break;
      case MODE_FACE: mode_str = "Faces"; break;
      case MODE_CUSTOM_TEXT: mode_str = "Custom Text"; break;
      case MODE_OFF: mode_str = "Off"; break;
    }
    sel_display_mode_->publish_state(mode_str);
  }
#endif
}

void HackPackNixieClock::set_color_mode(ColorMode mode) {
  color_mode_ = mode;
  if (mode != COLOR_SOLID) {
    use_panel_custom_rgb_ = false;
  }
  anim_mode_changed_ = true;
  schedule_save_preferences_();
}

void HackPackNixieClock::set_colon_mode(ColonMode mode) {
  colon_mode_ = mode;
#ifdef USE_SELECT
  if (sel_colon_color_mode_ != nullptr) {
    if (mode == COLON_AUTO_BLEND) sel_colon_color_mode_->publish_state("Auto Blend");
    else if (mode == COLON_MATCH_UNDERGLOW) sel_colon_color_mode_->publish_state("Match Underglow");
    else sel_colon_color_mode_->publish_state("Fixed");
  }
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_24hr_mode(bool enable) {
  hr24_mode_ = enable;
#ifdef USE_SWITCH
  if (sw_format_24hr_ != nullptr) sw_format_24hr_->publish_state(enable);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_leading_zero(bool enable) {
  show_lead_zero_ = enable;
#ifdef USE_SWITCH
  if (sw_leading_zero_ != nullptr) sw_leading_zero_->publish_state(enable);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_colon_blinking(bool enable) {
  colon_blinking_ = enable;
#ifdef USE_SWITCH
  if (sw_colon_blinking_ != nullptr) sw_colon_blinking_->publish_state(enable);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_am_pm_indicators(bool enable) {
  am_pm_enabled_ = enable;
#ifdef USE_SWITCH
  if (sw_am_pm_indicators_ != nullptr) sw_am_pm_indicators_->publish_state(enable);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_face_animations(bool enable) {
  face_anim_enabled_ = enable;
#ifdef USE_SWITCH
  if (sw_periodic_faces_ != nullptr) sw_periodic_faces_->publish_state(enable);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_physical_buttons(bool enable) {
  physical_buttons_enabled_ = enable;
#ifdef USE_SWITCH
  if (sw_physical_buttons_ != nullptr) sw_physical_buttons_->publish_state(enable);
#endif
  schedule_save_preferences_();
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

void HackPackNixieClock::set_link_brightness(bool enable) {
  link_brightness_ = enable;
  if (link_brightness_ && panel_light_ != nullptr && underglow_light_ != nullptr) {
    bool src_on = panel_light_->remote_values.is_on();
    float src_bri = panel_light_->remote_values.get_brightness();
    float target_bri = src_bri * 0.80f;

    bool tgt_on = underglow_light_->remote_values.is_on();
    float tgt_bri = underglow_light_->remote_values.get_brightness();

    if (tgt_on != src_on || std::abs(tgt_bri - target_bri) >= 0.005f) {
      syncing_light_ = true;
      auto call = underglow_light_->make_call();
      call.set_state(src_on);
      if (src_on) {
        call.set_brightness(target_bri);
      }
      call.set_transition_length(0);
      call.perform();
      syncing_light_ = false;
    }
  }
#ifdef USE_SWITCH
  if (sw_link_brightness_ != nullptr) sw_link_brightness_->publish_state(enable);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::set_panel_brightness(uint8_t brightness) {
  panel_brightness_ = brightness;
}

void HackPackNixieClock::set_underglow_brightness(uint8_t brightness) {
  underglow_brightness_ = brightness;
}

void HackPackNixieClock::set_ready_for_ota() {
  display_mode_ = MODE_OFF;
  link_brightness_ = false; // Decouple so underglow stays lit as a low-power OTA status glow
  panel_brightness_ = 0;
  underglow_brightness_ = 64; // ~25% brightness for low power

  // Ensure underglow LEDs have visible warm amber color if they were previously off/black
  bool has_ug_color = false;
  for (int i = 0; i < 13; i++) {
    if (underglow_rgb_[i][0] > 0 || underglow_rgb_[i][1] > 0 || underglow_rgb_[i][2] > 0) {
      has_ug_color = true;
      break;
    }
  }
  if (!has_ug_color) {
    for (int i = 0; i < 13; i++) {
      underglow_rgb_[i][0] = 255;
      underglow_rgb_[i][1] = 140;
      underglow_rgb_[i][2] = 0;
    }
  }

  render_hardware_leds_();
}

void HackPackNixieClock::set_panel_color_pos(uint8_t pos) {
  panel_color_pos_ = pos;
  anim_mode_changed_ = true;
  use_panel_custom_rgb_ = false;
  schedule_save_preferences_();
}

void HackPackNixieClock::set_underglow_color_pos(uint8_t pos) {
  underglow_color_pos_ = pos;
  use_ug_custom_rgb_ = false;
  schedule_save_preferences_();
}

void HackPackNixieClock::set_panel_rgb(uint8_t r, uint8_t g, uint8_t b) {
  custom_r_ = r; custom_g_ = g; custom_b_ = b;
  use_panel_custom_rgb_ = true;
  color_mode_ = COLOR_SOLID;
  anim_mode_changed_ = true;
  schedule_save_preferences_();
}

void HackPackNixieClock::set_underglow_rgb(uint8_t r, uint8_t g, uint8_t b) {
  ug_custom_r_ = r; ug_custom_g_ = g; ug_custom_b_ = b;
  use_ug_custom_rgb_ = true;
  schedule_save_preferences_();
}

void HackPackNixieClock::set_alarm(uint8_t hour, uint8_t minute) {
  alarm_hour_ = hour % 24;
  alarm_minute_ = minute % 60;
  alarm_enabled_ = true;
  alarm_ringing_ = false;
#ifdef USE_SWITCH
  if (sw_alarm_enabled_ != nullptr) sw_alarm_enabled_->publish_state(true);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::arm_alarm() {
  alarm_enabled_ = true;
#ifdef USE_SWITCH
  if (sw_alarm_enabled_ != nullptr) sw_alarm_enabled_->publish_state(true);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::disarm_alarm() {
  alarm_enabled_ = false;
  alarm_ringing_ = false;
#ifdef USE_SWITCH
  if (sw_alarm_enabled_ != nullptr) sw_alarm_enabled_->publish_state(false);
#endif
#ifdef USE_BINARY_SENSOR
  if (bs_alarm_ringing_ != nullptr) bs_alarm_ringing_->publish_state(false);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::stop_alarm() {
  alarm_ringing_ = false;
#ifdef USE_BINARY_SENSOR
  if (bs_alarm_ringing_ != nullptr) bs_alarm_ringing_->publish_state(false);
#endif
}

void HackPackNixieClock::set_timer_duration(uint32_t seconds) {
  timer_duration_sec_ = (seconds > 0) ? seconds : 300;
#ifdef USE_NUMBER
  if (num_timer_duration_minutes_ != nullptr) num_timer_duration_minutes_->publish_state((float)(timer_duration_sec_ / 60));
#endif
#ifdef USE_TEXT
  if (txt_timer_duration_ != nullptr) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%um", (unsigned int)(timer_duration_sec_ / 60));
    txt_timer_duration_->publish_state(buf);
  }
#endif
  schedule_save_preferences_();
}

uint32_t HackPackNixieClock::set_timer_duration_string(const std::string &str) {
  uint32_t s = parse_duration_string(str);
  if (s > 0) {
    timer_duration_sec_ = s;
#ifdef USE_NUMBER
    if (num_timer_duration_minutes_ != nullptr) num_timer_duration_minutes_->publish_state((float)(s / 60));
#endif
#ifdef USE_TEXT
    if (txt_timer_duration_ != nullptr) txt_timer_duration_->publish_state(str);
#endif
    schedule_save_preferences_();
  }
  return s;
}

uint32_t HackPackNixieClock::parse_duration_string(const std::string &input) {
  std::string s;
  for (char c : input) {
    if (c != ' ' && c != '\t') s += (char)std::tolower((unsigned char)c);
  }
  if (s.empty()) return 0;

  // Check for colon format: HH:MM:SS or MM:SS
  size_t first_colon = s.find(':');
  if (first_colon != std::string::npos) {
    size_t second_colon = s.find(':', first_colon + 1);
    if (second_colon == std::string::npos) {
      // MM:SS
      uint32_t m = std::strtoul(s.substr(0, first_colon).c_str(), nullptr, 10);
      uint32_t sec = std::strtoul(s.substr(first_colon + 1).c_str(), nullptr, 10);
      return m * 60 + sec;
    } else {
      // HH:MM:SS
      uint32_t h = std::strtoul(s.substr(0, first_colon).c_str(), nullptr, 10);
      uint32_t m = std::strtoul(s.substr(first_colon + 1, second_colon - first_colon - 1).c_str(), nullptr, 10);
      uint32_t sec = std::strtoul(s.substr(second_colon + 1).c_str(), nullptr, 10);
      return h * 3600 + m * 60 + sec;
    }
  }

  // Check for unit format (e.g., 1h30m, 15m30s, 45s)
  bool has_units = false;
  uint32_t total_sec = 0;
  uint32_t cur_val = 0;
  bool in_num = false;

  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (std::isdigit((unsigned char)c)) {
      cur_val = cur_val * 10 + (c - '0');
      in_num = true;
    } else if (c == 'h') {
      total_sec += cur_val * 3600;
      cur_val = 0;
      in_num = false;
      has_units = true;
      if (i + 1 < s.length() && s[i + 1] == 'r') i++; // handle "hr"
    } else if (c == 'm') {
      total_sec += cur_val * 60;
      cur_val = 0;
      in_num = false;
      has_units = true;
      if (i + 2 < s.length() && s[i + 1] == 'i' && s[i + 2] == 'n') i += 2; // handle "min"
    } else if (c == 's') {
      total_sec += cur_val;
      cur_val = 0;
      in_num = false;
      has_units = true;
      if (i + 2 < s.length() && s[i + 1] == 'e' && s[i + 2] == 'c') i += 2; // handle "sec"
    }
  }

  if (has_units) {
    total_sec += cur_val;
    return total_sec;
  }

  // Pure numeric value without units or colons
  if (in_num && cur_val > 0) {
    if (cur_val <= 60) return cur_val * 60;
    return cur_val;
  }

  return 0;
}

void HackPackNixieClock::start_timer(uint32_t hours, uint32_t minutes, uint32_t seconds) {
  uint32_t tot = hours * 3600 + minutes * 60 + seconds;
  start_timer(tot);
}

void HackPackNixieClock::start_timer(uint32_t duration_sec) {
  if (duration_sec > 0) {
    timer_duration_sec_ = duration_sec;
    schedule_save_preferences_();
  }
  uint32_t tot = (timer_duration_sec_ > 0) ? timer_duration_sec_ : 300;
  timer_end_millis_ = millis() + (tot * 1000);
  timer_running_ = true;
  timer_ringing_ = false;
#ifdef USE_BINARY_SENSOR
  if (bs_timer_running_ != nullptr) bs_timer_running_->publish_state(true);
  if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(false);
#endif
  set_display_mode(MODE_TIMER);
}

void HackPackNixieClock::stop_timer() {
  timer_running_ = false;
  timer_ringing_ = false;
#ifdef USE_BINARY_SENSOR
  if (bs_timer_running_ != nullptr) bs_timer_running_->publish_state(false);
  if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(false);
#endif
  if (display_mode_ == MODE_TIMER) set_display_mode(MODE_TIME);
}

void HackPackNixieClock::play_beep(uint32_t duration_ms) {
  beep_end_ = millis() + duration_ms;
#if defined(USE_ESP_IDF)
  gpio_set_level((gpio_num_t)play_pin_, 1);
#else
  digitalWrite(play_pin_, HIGH);
#endif
}

void HackPackNixieClock::set_record_sound(bool active) {
#if defined(USE_ESP_IDF)
  gpio_set_level((gpio_num_t)rec_pin_, active ? 1 : 0);
#else
  digitalWrite(rec_pin_, active ? HIGH : LOW);
#endif
}

void HackPackNixieClock::show_scrolling_text(const std::string &message, uint32_t scroll_speed_ms) {
  if (display_mode_ != MODE_CUSTOM_TEXT) return_mode_ = display_mode_;
  display_mode_ = MODE_CUSTOM_TEXT;
  text_message_ = message;
  scroll_speed_ms_ = (scroll_speed_ms > 0) ? scroll_speed_ms : 350;
  text_phase_ = TEXT_PHASE_BLANK_START;
  text_phase_end_ = millis() + 1000;
  show_string_("      ");
}

void HackPackNixieClock::show_custom_text(const std::string &text, uint32_t duration_seconds) {
  show_scrolling_text(text, 350);
}

void HackPackNixieClock::trigger_face_animation() {
  if (display_mode_ != MODE_FACE) return_mode_ = display_mode_;
  display_mode_ = MODE_FACE;
  face_step_ = 0;
  show_string_("O __O ");
  face_step_end_ = millis() + 1000;
}

void HackPackNixieClock::trigger_slot_machine(uint32_t duration_ms) {
  if (display_mode_ != MODE_SLOT_MACHINE) return_mode_ = display_mode_;
  display_mode_ = MODE_SLOT_MACHINE;
  slot_machine_end_ = millis() + duration_ms;
  last_slot_step_ = 0;
  slot_digit_ = 0;
}

// =============================================================================
// Display & Sequence State Updates
// =============================================================================
void HackPackNixieClock::update_display_state_() {
  uint32_t now_ms = millis();
  ESPTime now_time = time_source_ ? time_source_->now() : ESPTime{};

  // 1. Custom Text / Scrolling Sequence
  if (display_mode_ == MODE_CUSTOM_TEXT) {
    switch (text_phase_) {
      case TEXT_PHASE_BLANK_START:
        show_string_("      ");
        if (now_ms >= text_phase_end_) {
          text_phase_ = TEXT_PHASE_FLASH_ALERT;
          text_phase_end_ = now_ms + 2000;
          flash_toggle_time_ = now_ms;
          flash_state_ = true;
          show_string_("!!!!!!");
        }
        break;

      case TEXT_PHASE_FLASH_ALERT:
        if (now_ms - flash_toggle_time_ >= 250) {
          flash_toggle_time_ = now_ms;
          flash_state_ = !flash_state_;
          show_string_(flash_state_ ? "!!!!!!" : "      ");
        }
        if (now_ms >= text_phase_end_) {
          text_phase_ = TEXT_PHASE_SCROLL;
          scroll_pos_ = 0;
          scroll_next_step_ = now_ms;
        }
        break;

      case TEXT_PHASE_SCROLL: {
        if (now_ms >= scroll_next_step_) {
          std::string padded = "      " + text_message_ + "      ";
          if (scroll_pos_ + 6 <= padded.length()) {
            std::string window = padded.substr(scroll_pos_, 6);
            show_string_(window.c_str());
            scroll_pos_++;
            scroll_next_step_ = now_ms + scroll_speed_ms_;
          } else {
            text_phase_ = TEXT_PHASE_BLANK_END;
            text_phase_end_ = now_ms + 1000;
            show_string_("      ");
          }
        }
        break;
      }

      case TEXT_PHASE_BLANK_END:
        show_string_("      ");
        if (now_ms >= text_phase_end_) {
          text_phase_ = TEXT_PHASE_IDLE;
          display_mode_ = return_mode_;
        }
        break;

      default:
        display_mode_ = return_mode_;
        break;
    }
    return;
  }

  // 2. Slot Machine Sequence
  if (display_mode_ == MODE_SLOT_MACHINE) {
    if (now_ms >= slot_machine_end_) {
      display_mode_ = return_mode_;
    } else {
      if (now_ms - last_slot_step_ > 50) {
        last_slot_step_ = now_ms;
        slot_digit_ = (slot_digit_ + 1) % 10;
        char d = '0' + slot_digit_;
        set_display_chars_(d, d, d, d, d, d);
      }
      return;
    }
  }

  // 3. Face Animation Sequence
  if (display_mode_ == MODE_FACE) {
    if (now_ms >= face_step_end_) {
      face_step_++;
      switch (face_step_) {
        case 1: show_string_(" O__ O"); face_step_end_ = now_ms + 1000; break;
        case 2: show_string_(" O<>O "); face_step_end_ = now_ms + 500;  break;
        case 3: show_string_(" -<>- "); face_step_end_ = now_ms + 250;  break;
        case 4: show_string_(" O<>O "); face_step_end_ = now_ms + 1500; break;
        case 5: show_string_(" ^<>^ "); face_step_end_ = now_ms + 1500; break;
        default:
          display_mode_ = return_mode_;
          next_periodic_face_ = now_ms + 30000;
          break;
      }
    }
    return;
  }

  // Check Periodic Face Animation
  if (display_mode_ == MODE_TIME && face_anim_enabled_ && now_ms > next_periodic_face_) {
    trigger_face_animation();
    return;
  }

  // 4. Render Active Display Mode
  switch (display_mode_) {
    case MODE_TIME: {
      static bool has_synced_time = false;
      if (now_time.is_valid()) {
        has_synced_time = true;
        int h = now_time.hour;
        int m = now_time.minute;
        int s = now_time.second;

        if (!hr24_mode_) {
          if (h == 0) h = 12;
          else if (h > 12) h -= 12;
        }

        char c0 = (show_lead_zero_ || h >= 10) ? ('0' + (h / 10)) : ' ';
        char c1 = '0' + (h % 10);
        char c2 = '0' + (m / 10);
        char c3 = '0' + (m % 10);
        char c4 = '0' + (s / 10);
        char c5 = '0' + (s % 10);

        set_display_chars_(c0, c1, c2, c3, c4, c5);
      } else if (!has_synced_time) {
        show_string_("-- -- ");
      }
      break;
    }

    case MODE_TIMER: {
      if (timer_running_) {
        if (now_ms >= timer_end_millis_) {
          timer_running_ = false;
          timer_ringing_ = true;
          timer_ring_start_ = now_ms;
          timer_remaining_sec_ = 0;
#ifdef USE_BINARY_SENSOR
          if (bs_timer_running_ != nullptr) bs_timer_running_->publish_state(false);
          if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(true);
#endif
#ifdef USE_SENSOR
          if (sensor_timer_remaining_sec_ != nullptr) sensor_timer_remaining_sec_->publish_state(0);
#endif
#ifdef USE_TEXT_SENSOR
          if (text_sensor_timer_remaining_ != nullptr) text_sensor_timer_remaining_->publish_state("00:00:00");
#endif
          set_display_chars_('0', '0', '0', '0', '0', '0');
        } else {
          uint32_t left_ms = timer_end_millis_ - now_ms;
          uint32_t prev_sec = timer_remaining_sec_;
          timer_remaining_sec_ = (left_ms + 999) / 1000;
          uint32_t tot_sec = left_ms / 1000;
          uint32_t th = tot_sec / 3600;
          uint32_t tm = (tot_sec % 3600) / 60;
          uint32_t ts = tot_sec % 60;

          if (timer_remaining_sec_ != prev_sec) {
#ifdef USE_SENSOR
            if (sensor_timer_remaining_sec_ != nullptr) sensor_timer_remaining_sec_->publish_state(timer_remaining_sec_);
#endif
#ifdef USE_TEXT_SENSOR
            if (text_sensor_timer_remaining_ != nullptr) {
              char buf[16];
              snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)th, (unsigned long)tm, (unsigned long)ts);
              text_sensor_timer_remaining_->publish_state(buf);
            }
#endif
          }

          set_display_chars_(
            '0' + ((th / 10) % 10), '0' + (th % 10),
            '0' + ((tm / 10) % 10), '0' + (tm % 10),
            '0' + ((ts / 10) % 10), '0' + (ts % 10)
          );
        }
      } else if (timer_ringing_) {
        set_display_chars_('0', '0', '0', '0', '0', '0');
      } else {
        uint32_t th = timer_duration_sec_ / 3600;
        uint32_t tm = (timer_duration_sec_ % 3600) / 60;
        uint32_t ts = timer_duration_sec_ % 60;
        set_display_chars_(
          '0' + ((th / 10) % 10), '0' + (th % 10),
          '0' + ((tm / 10) % 10), '0' + (tm % 10),
          '0' + ((ts / 10) % 10), '0' + (ts % 10)
        );
      }
      break;
    }

    case MODE_ALARM: {
      int h = alarm_hour_;
      if (!hr24_mode_) {
        if (h == 0) h = 12;
        else if (h > 12) h -= 12;
      }
      set_display_chars_(
        '0' + (h / 10), '0' + (h % 10),
        '0' + (alarm_minute_ / 10), '0' + (alarm_minute_ % 10),
        '0', '0'
      );
      break;
    }

    case MODE_CUSTOM_TEXT:
    case MODE_FACE:
    case MODE_SLOT_MACHINE:
      break;

    case MODE_OFF:
      show_string_("      ");
      break;
  }

  // 5. Alarm Trigger Check
  if (alarm_enabled_ && now_time.is_valid()) {
    if (now_time.hour == alarm_hour_ && now_time.minute == alarm_minute_ && now_time.second == 0 && !alarm_ringing_) {
      alarm_ringing_ = true;
      alarm_ring_start_ = now_ms;
#ifdef USE_BINARY_SENSOR
      if (bs_alarm_ringing_ != nullptr) bs_alarm_ringing_->publish_state(true);
#endif
    }
  }
}

// =============================================================================
// Underglow & Indicators Update
// =============================================================================
void HackPackNixieClock::update_underglow_() {
  ESPTime now_time = time_source_ ? time_source_->now() : ESPTime{};
  uint8_t sec = now_time.is_valid() ? now_time.second : 0;
  if (sec != last_sec_) {
    last_sec_ = sec;
    colon_blink_state_ = !colon_blink_state_;
  }

  uint32_t base_ug = use_ug_custom_rgb_ ? make_rgb_(ug_custom_r_, ug_custom_g_, ug_custom_b_) : wheel_(underglow_color_pos_);

  // 1. Colons (LED 0 and 1)
  bool colons_lit = !colon_blinking_ || colon_blink_state_;
  if (colons_lit) {
    uint32_t col0 = base_ug, col1 = base_ug;
    if (colon_mode_ == COLON_AUTO_BLEND && display_mode_ == MODE_TIME) {
      col0 = blend_two_panels_(1, 2);
      col1 = blend_two_panels_(3, 4);
    }
    underglow_rgb_[0][0] = red_(col0);
    underglow_rgb_[0][1] = green_(col0);
    underglow_rgb_[0][2] = blue_(col0);
    underglow_rgb_[1][0] = red_(col1);
    underglow_rgb_[1][1] = green_(col1);
    underglow_rgb_[1][2] = blue_(col1);
  } else {
    underglow_rgb_[0][0] = 0; underglow_rgb_[0][1] = 0; underglow_rgb_[0][2] = 0;
    underglow_rgb_[1][0] = 0; underglow_rgb_[1][1] = 0; underglow_rgb_[1][2] = 0;
  }

  // 2. Badge Glow (LEDs 2..9)
  for (int i = 2; i < 10; i++) {
    underglow_rgb_[i][0] = red_(base_ug);
    underglow_rgb_[i][1] = green_(base_ug);
    underglow_rgb_[i][2] = blue_(base_ug);
  }

  // 3. Alarm Indicator (LED 10)
  if (alarm_enabled_ || alarm_ringing_) {
    underglow_rgb_[10][0] = 255;
    underglow_rgb_[10][1] = 0;
    underglow_rgb_[10][2] = 0;
  } else {
    underglow_rgb_[10][0] = 0;
    underglow_rgb_[10][1] = 0;
    underglow_rgb_[10][2] = 0;
  }

  // 4. AM/PM Indicators (LED 11 = AM Amber, LED 12 = PM Purple)
  if (am_pm_enabled_ && now_time.is_valid() && display_mode_ == MODE_TIME && !timer_running_) {
    bool is_pm = (now_time.hour >= 12);
    if (is_pm) {
      underglow_rgb_[11][0] = 0;  underglow_rgb_[11][1] = 0;  underglow_rgb_[11][2] = 0;
      underglow_rgb_[12][0] = 80; underglow_rgb_[12][1] = 20; underglow_rgb_[12][2] = 255;
    } else {
      underglow_rgb_[11][0] = 255; underglow_rgb_[11][1] = 140; underglow_rgb_[11][2] = 20;
      underglow_rgb_[12][0] = 0;   underglow_rgb_[12][1] = 0;   underglow_rgb_[12][2] = 0;
    }
  } else {
    underglow_rgb_[11][0] = 0; underglow_rgb_[11][1] = 0; underglow_rgb_[11][2] = 0;
    underglow_rgb_[12][0] = 0; underglow_rgb_[12][1] = 0; underglow_rgb_[12][2] = 0;
  }
}

// =============================================================================
// Hardware WS2812 Cycle-Accurate IRAM Bit Transmitter
// =============================================================================
#if defined(USE_ESP_IDF)
static void IRAM_ATTR ws2812_transmit(uint8_t pin, const uint8_t *data, size_t len) {
  if (!data || len == 0) return;

  uint32_t mask = (1UL << pin);
  uint32_t cpu_mhz = 160;
#if defined(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
  cpu_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
#elif defined(CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ)
  cpu_mhz = CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ;
#endif

  // WS2812 (800kHz): T0H=350ns, T0L=900ns, T1H=900ns, T1L=350ns
  uint32_t c_0h = (350 * cpu_mhz) / 1000;
  uint32_t c_0l = (900 * cpu_mhz) / 1000;
  uint32_t c_1h = (900 * cpu_mhz) / 1000;
  uint32_t c_1l = (350 * cpu_mhz) / 1000;

  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&mux);

  for (size_t i = 0; i < len; i++) {
    uint8_t byte = data[i];
    for (int bit = 7; bit >= 0; bit--) {
      if ((byte >> bit) & 1) {
        REG_WRITE(GPIO_OUT_W1TS_REG, mask);
        uint32_t start = esp_cpu_get_cycle_count();
        while ((esp_cpu_get_cycle_count() - start) < c_1h) {}

        REG_WRITE(GPIO_OUT_W1TC_REG, mask);
        start = esp_cpu_get_cycle_count();
        while ((esp_cpu_get_cycle_count() - start) < c_1l) {}
      } else {
        REG_WRITE(GPIO_OUT_W1TS_REG, mask);
        uint32_t start = esp_cpu_get_cycle_count();
        while ((esp_cpu_get_cycle_count() - start) < c_0h) {}

        REG_WRITE(GPIO_OUT_W1TC_REG, mask);
        start = esp_cpu_get_cycle_count();
        while ((esp_cpu_get_cycle_count() - start) < c_0l) {}
      }
    }
  }

  // WS2812 Reset Latch: >300us LOW
  REG_WRITE(GPIO_OUT_W1TC_REG, mask);
  uint32_t reset_cycles = 300 * cpu_mhz;
  uint32_t start = esp_cpu_get_cycle_count();
  while ((esp_cpu_get_cycle_count() - start) < reset_cycles) {}

  portEXIT_CRITICAL(&mux);
}
#endif

// 256-entry Perceptual Gamma 2.4 curve table with non-zero floor for x > 0
static const uint8_t GAMMA8_TABLE[256] = {
    0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
    2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,
    5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,
    9,  10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  14,  14,  14,  15,  15,
   16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  22,  22,  23,  23,  24,
   24,  25,  26,  26,  27,  28,  28,  29,  30,  30,  31,  32,  32,  33,  34,  35,
   35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  43,  44,  45,  46,  47,  48,
   49,  50,  51,  52,  53,  53,  54,  55,  56,  57,  58,  59,  60,  62,  63,  64,
   65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  80,  81,  82,
   83,  85,  86,  87,  88,  90,  91,  92,  94,  95,  96,  98,  99, 100, 102, 103,
  105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 124, 126, 127,
  129, 131, 132, 134, 136, 137, 139, 141, 142, 144, 146, 148, 149, 151, 153, 155,
  156, 158, 160, 162, 164, 166, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185,
  187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 210, 212, 214, 216, 218,
  220, 223, 225, 227, 229, 232, 234, 236, 239, 241, 243, 246, 248, 250, 253, 255
};

void HackPackNixieClock::render_hardware_leds_() {
#if defined(USE_ESP_IDF)
  uint8_t pb = GAMMA8_TABLE[panel_brightness_];
  // When linked, scale underglow to 80% (204/255) of panel brightness to match perceived lumens behind the panel film
  uint8_t effective_ug_bri = link_brightness_ ? (uint8_t)((uint16_t)panel_brightness_ * 204 / 255) : underglow_brightness_;
  uint8_t ub = GAMMA8_TABLE[effective_ug_bri];

  // Convert 42 Panel LEDs (GRB) to byte buffer
  uint8_t panel_bytes[42 * 3];
  int idx = 0;
  for (int p = 0; p < 6; p++) {
    for (int s = 0; s < 7; s++) {
      uint8_t r = 0, g = 0, b = 0;
      if (panel_segments_[p][s] && display_mode_ != MODE_OFF) {
        r = (uint8_t)((uint16_t)panel_rgb_[p][s][0] * pb / 255);
        g = (uint8_t)((uint16_t)panel_rgb_[p][s][1] * pb / 255);
        b = (uint8_t)((uint16_t)panel_rgb_[p][s][2] * pb / 255);
      }
      panel_bytes[idx++] = g; // GRB order
      panel_bytes[idx++] = r;
      panel_bytes[idx++] = b;
    }
  }
  ws2812_transmit(panel_pin_, panel_bytes, sizeof(panel_bytes));

  // Convert 13 Underglow LEDs (GRB) to byte buffer
  uint8_t ug_bytes[13 * 3];
  idx = 0;
  for (int i = 0; i < 13; i++) {
    uint8_t r = (uint8_t)((uint16_t)underglow_rgb_[i][0] * ub / 255);
    uint8_t g = (uint8_t)((uint16_t)underglow_rgb_[i][1] * ub / 255);
    uint8_t b = (uint8_t)((uint16_t)underglow_rgb_[i][2] * ub / 255);
    ug_bytes[idx++] = g; // GRB order
    ug_bytes[idx++] = r;
    ug_bytes[idx++] = b;
  }
  ws2812_transmit(underglow_pin_, ug_bytes, sizeof(ug_bytes));
#endif
}

// =============================================================================
// Helper Functions: Animation, Colors, Glyphs
// =============================================================================
void HackPackNixieClock::map_char_to_segments_(char val, bool out7[7]) const {
  char lower = (val >= 'A' && val <= 'Z') ? (val + 32) : val;
  if (val == 'O') lower = 'O';

  for (size_t i = 0; i < sizeof(NIXIE_CHARSET_MAP); ++i) {
    if (NIXIE_CHARSET_MAP[i] == lower || NIXIE_CHARSET_MAP[i] == val) {
      for (int s = 0; s < 7; s++) out7[s] = NIXIE_SEGMENT_MAP[i][s];
      return;
    }
  }
  for (int s = 0; s < 7; s++) out7[s] = false;
}

void HackPackNixieClock::set_display_chars_(char c0, char c1, char c2, char c3, char c4, char c5) {
  bool changed = (current_display_chars_[0] != c0 ||
                  current_display_chars_[1] != c1 ||
                  current_display_chars_[2] != c2 ||
                  current_display_chars_[3] != c3 ||
                  current_display_chars_[4] != c4 ||
                  current_display_chars_[5] != c5);

  current_display_chars_[0] = c0;
  current_display_chars_[1] = c1;
  current_display_chars_[2] = c2;
  current_display_chars_[3] = c3;
  current_display_chars_[4] = c4;
  current_display_chars_[5] = c5;
  current_display_chars_[6] = '\0';

#ifdef USE_TEXT_SENSOR
  if (changed && text_sensor_active_display_chars_ != nullptr) {
    ESP_LOGVV(TAG, "Active Display Characters: %s", current_display_chars_);
    text_sensor_active_display_chars_->publish_state(std::string(current_display_chars_));
  }
#endif

  for (int p = 0; p < 6; p++) {
    bool segs[7];
    map_char_to_segments_(current_display_chars_[p], segs);
    for (int s = 0; s < 7; s++) {
      panel_segments_[p][s] = segs[s];
    }
  }
}

void HackPackNixieClock::show_string_(const char *s) {
  char buf[6] = {' ', ' ', ' ', ' ', ' ', ' '};
  if (s) {
    for (int i = 0; i < 6 && s[i] != '\0'; i++) {
      buf[i] = s[i];
    }
  }
  set_display_chars_(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
}

uint32_t HackPackNixieClock::average_panel_color_(int panelIndex) const {
  if (panelIndex < 0 || panelIndex > 5) return 0;
  uint32_t sumR = 0, sumG = 0, sumB = 0;
  for (int i = 0; i < 7; i++) {
    sumR += panel_rgb_[panelIndex][i][0];
    sumG += panel_rgb_[panelIndex][i][1];
    sumB += panel_rgb_[panelIndex][i][2];
  }
  return make_rgb_(sumR / 7, sumG / 7, sumB / 7);
}

uint32_t HackPackNixieClock::blend_two_panels_(int p1, int p2) const {
  uint32_t c1 = average_panel_color_(p1);
  uint32_t c2 = average_panel_color_(p2);
  uint8_t r = ((uint16_t)red_(c1) + red_(c2)) / 2;
  uint8_t g = ((uint16_t)green_(c1) + green_(c2)) / 2;
  uint8_t b = ((uint16_t)blue_(c1) + blue_(c2)) / 2;
  return make_rgb_(r, g, b);
}

uint32_t HackPackNixieClock::wheel_(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)0 << 8) | (pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return ((uint32_t)0 << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  }
  pos -= 170;
  return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | 0;
}

uint32_t HackPackNixieClock::color_fade_(uint32_t c1, uint32_t c2, int step, int maxSteps) {
  if (maxSteps <= 0) return c1;
  uint8_t r1 = red_(c1), g1 = green_(c1), b1 = blue_(c1);
  uint8_t r2 = red_(c2), g2 = green_(c2), b2 = blue_(c2);
  uint8_t r = (r1 * (maxSteps - step) + r2 * step) / maxSteps;
  uint8_t g = (g1 * (maxSteps - step) + g2 * step) / maxSteps;
  uint8_t b = (b1 * (maxSteps - step) + b2 * step) / maxSteps;
  return make_rgb_(r, g, b);
}

void HackPackNixieClock::setup_animation_() {
  anim_step_ = 0;
  switch (color_mode_) {
    case COLOR_RAINBOW: anim_total_steps_ = 255; anim_frame_ms_ = 40; break;
    case COLOR_SOLID: anim_total_steps_ = 1; anim_frame_ms_ = 100; break;
    case COLOR_GRADIENT:
      anim_total_steps_ = 50; anim_frame_ms_ = 50;
      strt_col1_ = wheel_(panel_color_pos_ + (rand() % 61 - 30));
      strt_col2_ = wheel_(panel_color_pos_ + (rand() % 61 - 30));
      end_col1_  = wheel_(panel_color_pos_ + (rand() % 61 - 30));
      end_col2_  = wheel_(panel_color_pos_ + (rand() % 61 - 30));
      now_col1_  = strt_col1_; now_col2_  = strt_col2_;
      break;
    case COLOR_FLOW:
      anim_total_steps_ = 50; anim_frame_ms_ = 25;
      strt_col1_ = wheel_(rand() % 256); strt_col2_ = wheel_(rand() % 256);
      end_col1_  = wheel_(rand() % 256); end_col2_  = wheel_(rand() % 256);
      now_col1_  = strt_col1_; now_col2_  = strt_col2_;
      break;
    case COLOR_WIPE: {
      anim_total_steps_ = 30; anim_frame_ms_ = 20; wipe_index_ = 0; wipe_col_ = true;
      uint8_t col = rand() % 256;
      while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
      strt_col1_ = wheel_(col);
      break;
    }
    case COLOR_PULSE: {
      anim_total_steps_ = 40; anim_frame_ms_ = 15; pulse_index_ = 0; pulse_dir_ = true;
      uint8_t col = rand() % 256;
      while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
      strt_col1_ = wheel_(col);
      break;
    }
    case COLOR_BOUNCE: {
      anim_total_steps_ = 40; anim_frame_ms_ = 15; bounce_index_ = 0; bounce_dir_ = true;
      uint8_t col = rand() % 256;
      while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
      strt_col1_ = wheel_(panel_color_pos_); strt_col2_ = wheel_(col);
      break;
    }
  }
}

void HackPackNixieClock::on_animation_complete_() {
  switch (color_mode_) {
    case COLOR_GRADIENT:
      strt_col1_ = end_col1_; strt_col2_ = end_col2_;
      now_col1_  = strt_col1_; now_col2_  = strt_col2_;
      end_col1_  = wheel_(panel_color_pos_ + (rand() % 61 - 30));
      end_col2_  = wheel_(panel_color_pos_ + (rand() % 61 - 30));
      break;
    case COLOR_FLOW:
      strt_col1_ = end_col1_; strt_col2_ = end_col2_;
      now_col1_  = strt_col1_; now_col2_  = strt_col2_;
      end_col1_  = wheel_(rand() % 256); end_col2_  = wheel_(rand() % 256);
      break;
    case COLOR_WIPE:
      wipe_index_++;
      if (wipe_index_ > 5) {
        wipe_col_ = !wipe_col_;
        wipe_index_ = 0;
        if (wipe_col_) {
          uint8_t col = rand() % 256;
          while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
          strt_col1_ = wheel_(col);
        }
      }
      break;
    case COLOR_PULSE:
      if (pulse_dir_) pulse_index_++;
      else pulse_index_--;
      if (pulse_index_ == 0 && !pulse_dir_) {
        uint8_t col = rand() % 256;
        while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
        strt_col1_ = wheel_(col);
      }
      if (pulse_index_ > 3) { pulse_dir_ = !pulse_dir_; pulse_index_ = 3; }
      if (pulse_index_ < 0) { pulse_dir_ = !pulse_dir_; pulse_index_ = 1; }
      break;
    case COLOR_BOUNCE:
      if (bounce_dir_) bounce_index_++;
      else bounce_index_--;
      if (bounce_index_ > 5) {
        uint8_t col = rand() % 256;
        while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
        strt_col1_ = strt_col2_; strt_col2_ = wheel_(col);
        bounce_dir_ = !bounce_dir_; bounce_index_ = 5;
      }
      if (bounce_index_ < 0) {
        uint8_t col = rand() % 256;
        while (abs((int)col - (int)panel_color_pos_) < 15) col = rand() % 256;
        strt_col1_ = strt_col2_; strt_col2_ = wheel_(col);
        bounce_dir_ = !bounce_dir_; bounce_index_ = 0;
      }
      break;
    default: break;
  }
}

void HackPackNixieClock::step_animation_() {
  uint32_t now = millis();
  if (now - last_anim_update_ < (uint32_t)anim_frame_ms_) return;
  last_anim_update_ = now;

  if (anim_mode_changed_) {
    setup_animation_();
    anim_mode_changed_ = false;
  }

  uint32_t base_col = use_panel_custom_rgb_ ? make_rgb_(custom_r_, custom_g_, custom_b_) : wheel_(panel_color_pos_);

  switch (color_mode_) {
    case COLOR_RAINBOW:
      for (int p = 0; p < 6; p++) {
        for (int s = 0; s < 7; s++) {
          uint32_t c = wheel_(25 * p + 5 * s + anim_step_);
          panel_rgb_[p][s][0] = red_(c);
          panel_rgb_[p][s][1] = green_(c);
          panel_rgb_[p][s][2] = blue_(c);
        }
      }
      break;

    case COLOR_SOLID:
      for (int p = 0; p < 6; p++) {
        for (int s = 0; s < 7; s++) {
          panel_rgb_[p][s][0] = red_(base_col);
          panel_rgb_[p][s][1] = green_(base_col);
          panel_rgb_[p][s][2] = blue_(base_col);
        }
      }
      break;

    case COLOR_GRADIENT:
    case COLOR_FLOW:
      for (int p = 0; p < 6; p++) {
        uint32_t panelColor = color_fade_(now_col1_, now_col2_, p, 5);
        for (int s = 0; s < 7; s++) {
          panel_rgb_[p][s][0] = red_(panelColor);
          panel_rgb_[p][s][1] = green_(panelColor);
          panel_rgb_[p][s][2] = blue_(panelColor);
        }
      }
      now_col1_ = color_fade_(strt_col1_, end_col1_, anim_step_, anim_total_steps_);
      now_col2_ = color_fade_(strt_col2_, end_col2_, anim_step_, anim_total_steps_);
      break;

    case COLOR_WIPE: {
      uint32_t fadeCol = wipe_col_ ? 
        color_fade_(base_col, strt_col1_, anim_step_, anim_total_steps_) :
        color_fade_(strt_col1_, base_col, anim_step_, anim_total_steps_);

      for (int p = 0; p < 6; p++) {
        uint32_t c = (p == wipe_index_) ? fadeCol : (wipe_col_ ? (p < wipe_index_ ? strt_col1_ : base_col) : (p > wipe_index_ ? strt_col1_ : base_col));
        for (int s = 0; s < 7; s++) {
          panel_rgb_[p][s][0] = red_(c);
          panel_rgb_[p][s][1] = green_(c);
          panel_rgb_[p][s][2] = blue_(c);
        }
      }
      break;
    }

    case COLOR_PULSE:
      for (int p = 0; p < 6; p++) {
        uint32_t c;
        if (p < 3 - pulse_index_ || p > 2 + pulse_index_) {
          c = base_col;
        } else if (p == 3 - pulse_index_ || p == 2 + pulse_index_) {
          c = pulse_dir_ ? color_fade_(base_col, strt_col1_, anim_step_, anim_total_steps_) : color_fade_(strt_col1_, base_col, anim_step_, anim_total_steps_);
        } else {
          c = strt_col1_;
        }
        for (int s = 0; s < 7; s++) {
          panel_rgb_[p][s][0] = red_(c);
          panel_rgb_[p][s][1] = green_(c);
          panel_rgb_[p][s][2] = blue_(c);
        }
      }
      break;

    case COLOR_BOUNCE:
      for (int p = 0; p < 6; p++) {
        uint32_t c = base_col;
        if (p == bounce_index_) {
          c = color_fade_(base_col, strt_col2_, anim_step_, anim_total_steps_);
        } else if (bounce_dir_ && p == bounce_index_ - 1) {
          c = color_fade_(strt_col2_, base_col, anim_step_, anim_total_steps_);
        } else if (!bounce_dir_ && p == bounce_index_ + 1) {
          c = color_fade_(strt_col2_, base_col, anim_step_, anim_total_steps_);
        }
        for (int s = 0; s < 7; s++) {
          panel_rgb_[p][s][0] = red_(c);
          panel_rgb_[p][s][1] = green_(c);
          panel_rgb_[p][s][2] = blue_(c);
        }
      }
      break;
  }

  anim_step_++;
  if (anim_step_ > anim_total_steps_) {
    anim_step_ = 0;
    on_animation_complete_();
  }
}

// =============================================================================
// HackPackNixieLightOutput Implementation
// =============================================================================
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

// =============================================================================
// Entity Registration Methods
// =============================================================================
#ifdef USE_SWITCH
void HackPackNixieClock::register_switch(SwitchType type, switch_::Switch *sw) {
  switch (type) {
    case SwitchType::FORMAT_24HR: sw_format_24hr_ = sw; break;
    case SwitchType::LEADING_ZERO: sw_leading_zero_ = sw; break;
    case SwitchType::COLON_BLINKING: sw_colon_blinking_ = sw; break;
    case SwitchType::AM_PM_INDICATORS: sw_am_pm_indicators_ = sw; break;
    case SwitchType::PERIODIC_FACES: sw_periodic_faces_ = sw; break;
    case SwitchType::PHYSICAL_BUTTONS: sw_physical_buttons_ = sw; break;
    case SwitchType::LINK_BRIGHTNESS: sw_link_brightness_ = sw; break;
    case SwitchType::ALARM_ENABLED: sw_alarm_enabled_ = sw; break;
    case SwitchType::RECORD_SOUND: sw_record_sound_ = sw; break;
  }
}
#endif

#ifdef USE_SELECT
void HackPackNixieClock::register_select(SelectType type, select::Select *sel) {
  switch (type) {
    case SelectType::DISPLAY_MODE: sel_display_mode_ = sel; break;
    case SelectType::COLON_COLOR_MODE: sel_colon_color_mode_ = sel; break;
  }
}
#endif

#ifdef USE_NUMBER
void HackPackNixieClock::register_number(NumberType type, number::Number *num) {
  if (type == NumberType::TIMER_DURATION_MINUTES) {
    num_timer_duration_minutes_ = num;
  }
}
#endif

#ifdef USE_TEXT
void HackPackNixieClock::register_text(TextType type, text::Text *txt) {
  if (type == TextType::TIMER_DURATION) {
    txt_timer_duration_ = txt;
  }
}
#endif

#ifdef USE_DATETIME_TIME
void HackPackNixieClock::register_time(TimeType type, datetime::TimeEntity *tm) {
  if (type == TimeType::ALARM_TIME) {
    tm_alarm_time_ = tm;
  }
}
#endif

void HackPackNixieClock::register_light(LightType type, light::LightState *st) {
  if (type == LightType::PANEL) {
    panel_light_ = st;
    if (st != nullptr) st->add_remote_values_listener(&panel_listener_);
  } else if (type == LightType::UNDERGLOW) {
    underglow_light_ = st;
    if (st != nullptr) st->add_remote_values_listener(&underglow_listener_);
  }
}

// =============================================================================
// Platform Entities Implementation
// =============================================================================
#ifdef USE_SWITCH
void HackPackNixieSwitch::setup() {
  if (!parent_) return;
  bool initial_state = false;
  switch (type_) {
    case SwitchType::FORMAT_24HR: initial_state = parent_->get_24hr_mode(); break;
    case SwitchType::LEADING_ZERO: initial_state = parent_->get_leading_zero(); break;
    case SwitchType::COLON_BLINKING: initial_state = parent_->get_colon_blinking(); break;
    case SwitchType::AM_PM_INDICATORS: initial_state = parent_->get_am_pm_indicators(); break;
    case SwitchType::PERIODIC_FACES: initial_state = parent_->get_face_animations(); break;
    case SwitchType::PHYSICAL_BUTTONS: initial_state = parent_->get_physical_buttons(); break;
    case SwitchType::LINK_BRIGHTNESS: initial_state = parent_->get_link_brightness(); break;
    case SwitchType::ALARM_ENABLED: initial_state = parent_->is_alarm_armed(); break;
    case SwitchType::RECORD_SOUND: initial_state = false; break;
  }
  publish_state(initial_state);
}

void HackPackNixieSwitch::write_state(bool state) {
  if (!parent_) return;
  switch (type_) {
    case SwitchType::FORMAT_24HR: parent_->set_24hr_mode(state); break;
    case SwitchType::LEADING_ZERO: parent_->set_leading_zero(state); break;
    case SwitchType::COLON_BLINKING: parent_->set_colon_blinking(state); break;
    case SwitchType::AM_PM_INDICATORS: parent_->set_am_pm_indicators(state); break;
    case SwitchType::PERIODIC_FACES: parent_->set_face_animations(state); break;
    case SwitchType::PHYSICAL_BUTTONS: parent_->set_physical_buttons(state); break;
    case SwitchType::LINK_BRIGHTNESS: parent_->set_link_brightness(state); break;
    case SwitchType::ALARM_ENABLED:
      if (state) parent_->arm_alarm();
      else parent_->disarm_alarm();
      break;
    case SwitchType::RECORD_SOUND: parent_->set_record_sound(state); break;
  }
  publish_state(state);
}
#endif

#ifdef USE_BUTTON
void HackPackNixieButton::press_action() {
  if (!parent_) return;
  switch (type_) {
    case ButtonType::START_TIMER: parent_->start_timer(); break;
    case ButtonType::STOP_TIMER: parent_->stop_timer(); break;
    case ButtonType::STOP_ALARM: parent_->stop_alarm(); break;
    case ButtonType::TRIGGER_FACE: parent_->trigger_face_animation(); break;
    case ButtonType::TRIGGER_SLOT_MACHINE: parent_->trigger_slot_machine(2500); break;
    case ButtonType::PLAY_SOUND: parent_->play_beep(150); break;
  }
}
#endif

#ifdef USE_SELECT
void HackPackNixieSelect::setup() {
  if (!parent_) return;
  if (type_ == SelectType::DISPLAY_MODE) {
    publish_state("Time");
  } else if (type_ == SelectType::COLON_COLOR_MODE) {
    ColonMode cm = parent_->get_colon_mode();
    if (cm == COLON_AUTO_BLEND) publish_state("Auto Blend");
    else if (cm == COLON_MATCH_UNDERGLOW) publish_state("Match Underglow");
    else publish_state("Fixed");
  }
}

void HackPackNixieSelect::control(const std::string &value) {
  if (!parent_) return;
  if (type_ == SelectType::DISPLAY_MODE) {
    if (value == "Time") parent_->set_display_mode(MODE_TIME);
    else if (value == "Timer") parent_->set_display_mode(MODE_TIMER);
    else if (value == "Alarm View") parent_->set_display_mode(MODE_ALARM);
    else if (value == "Slot Machine") parent_->trigger_slot_machine(2500);
    else if (value == "Faces") parent_->trigger_face_animation();
    else if (value == "Custom Text") parent_->set_display_mode(MODE_CUSTOM_TEXT);
    else if (value == "Off") parent_->set_display_mode(MODE_OFF);
  } else if (type_ == SelectType::COLON_COLOR_MODE) {
    if (value == "Auto Blend") parent_->set_colon_mode(COLON_AUTO_BLEND);
    else if (value == "Match Underglow") parent_->set_colon_mode(COLON_MATCH_UNDERGLOW);
    else if (value == "Fixed") parent_->set_colon_mode(COLON_FIXED);
  }
  publish_state(value);
}
#endif

#ifdef USE_NUMBER
void HackPackNixieNumber::setup() {
  if (!parent_) return;
  publish_state((float)(parent_->get_timer_duration() / 60));
}

void HackPackNixieNumber::control(float value) {
  if (!parent_) return;
  uint32_t sec = (uint32_t)value * 60;
  parent_->set_timer_duration(sec);
  publish_state(value);
}
#endif

#ifdef USE_TEXT
void HackPackNixieText::setup() {
  if (!parent_) return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%um", (unsigned int)(parent_->get_timer_duration() / 60));
  publish_state(buf);
}

void HackPackNixieText::control(const std::string &value) {
  if (!parent_) return;
  parent_->set_timer_duration_string(value);
  publish_state(value);
}
#endif

#ifdef USE_DATETIME_TIME
void HackPackNixieTime::setup() {
  if (!parent_) return;
  this->hour_ = parent_->get_alarm_hour();
  this->minute_ = parent_->get_alarm_minute();
  this->second_ = 0;
  this->publish_state();
}

void HackPackNixieTime::control(const datetime::TimeCall &call) {
  if (!parent_) return;
  if (call.get_hour().has_value() && call.get_minute().has_value()) {
    parent_->set_alarm(*call.get_hour(), *call.get_minute());
  }
}
#endif

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
