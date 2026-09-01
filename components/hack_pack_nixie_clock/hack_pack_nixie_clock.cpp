#include "hack_pack_nixie_clock.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#if defined(USE_ESP_IDF)
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#endif

namespace esphome {
namespace hack_pack_nixie_clock {

static const char *const TAG = "hack_pack_nixie_clock";

HackPackNixieClock::HackPackNixieClock() {
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
  std::memset(current_display_chars_, ' ', 6);
  current_display_chars_[6] = '\0';
}

void HackPackNixieClock::setup() {
  ESP_LOGI(TAG, "Initializing Hack Pack Nixie Clock Component...");
  init_hardware_();
  load_preferences_();
  current_animation_ = create_color_animation(color_mode_);
  if (current_animation_) {
    current_animation_->setup(this);
  }
  ESP_LOGI(TAG, "Hack Pack Nixie Clock initialized successfully");
}

void HackPackNixieClock::dump_config() {
  ESP_LOGCONFIG(TAG, "Hack Pack Nixie Clock:");
  ESP_LOGCONFIG(TAG, "  Panel LED Pin (GPIO0): %u", panel_pin_);
  ESP_LOGCONFIG(TAG, "  Underglow LED Pin (GPIO1): %u", underglow_pin_);
  ESP_LOGCONFIG(TAG, "  Audio Play Pin (GPIO5): %u", play_pin_);
  ESP_LOGCONFIG(TAG, "  Audio Rec Pin (GPIO4): %u", rec_pin_);
  ESP_LOGCONFIG(TAG, "  Buttons (Top/Center/Up/Down/Left/Right): %u / %u / %u / %u / %u / %u",
                btn_top_.pin, btn_center_.pin, btn_up_.pin, btn_down_.pin, btn_left_.pin, btn_right_.pin);
  ESP_LOGCONFIG(TAG, "  24-Hour Format: %s", hr24_mode_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Leading Zero: %s", show_lead_zero_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Colon Blinking: %s", colon_blinking_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  AM/PM Indicators: %s", am_pm_enabled_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Link Brightness (80%% Ratio): %s", link_brightness_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Physical Buttons: %s", physical_buttons_enabled_ ? "ENABLED" : "DISABLED");
  ESP_LOGCONFIG(TAG, "  Alarm Configured: %02u:%02u (%s)", alarm_hour_, alarm_minute_, alarm_enabled_ ? "ARMED" : "DISARMED");
  ESP_LOGCONFIG(TAG, "  Timer Default Duration: %u seconds", (unsigned int)timer_duration_sec_);
  if (time_source_ == nullptr) {
    ESP_LOGW(TAG, "  Time Source: NONE (Sync via Home Assistant or SNTP required)");
  }
}

void HackPackNixieClock::loop() {
  uint32_t now_ms = millis();

  // 1. Debounced Preference Flash Save
  if (pref_save_timeout_ > 0 && now_ms >= pref_save_timeout_) {
    save_preferences_now_();
  }

  // 2. Poll Physical Navigation Buttons
  poll_buttons_();

  // 3. Step Polymorphic LED Animation Strategy
  step_animation_();

  // 4. Update Display Mode State Machine
  update_display_state_();

  // 5. Update Underglow, Colons & Indicators
  update_underglow_();

  // 6. Audio Beep Pulse Management
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

  // 7. Repeating Alarm / Timer Beeper
  if (alarm_ringing_ || timer_ringing_) {
    uint32_t start_ref = alarm_ringing_ ? alarm_ring_start_ : timer_ring_start_;
    if ((now_ms - start_ref) % 1000 < 100) {
      if (beep_end_ == 0) play_beep(100);
    }
  }

  // 8. Render Hardware LEDs at ~50Hz (20ms intervals) after a 1200ms boot power-stabilization grace period
  static uint32_t last_render = 0;
  if (now_ms >= 1200 && now_ms - last_render >= 20) {
    last_render = now_ms;
    render_hardware_leds_();
  }
}

// =============================================================================
// NVS Flash Persistence
// =============================================================================
void HackPackNixieClock::load_preferences_() {
  pref_ = global_preferences->make_preference<NixieClockStorage>(fnv1_hash("hack_pack_nixie_clock_v2"));
  NixieClockStorage storage;
  if (pref_.load(&storage)) {
    // Validate storage magic header and version using defined constants
    if (storage.magic == NIXIE_STORAGE_MAGIC && storage.version == NIXIE_STORAGE_VERSION) {
      ESP_LOGI(TAG, "Restored preferences from flash storage (v%u)", storage.version);
      hr24_mode_ = storage.hr24_mode;
      show_lead_zero_ = storage.show_lead_zero;
      colon_blinking_ = storage.colon_blinking;
      am_pm_enabled_ = storage.am_pm_enabled;
      face_anim_enabled_ = storage.face_anim_enabled;
      physical_buttons_enabled_ = storage.physical_buttons_enabled;
      link_brightness_ = storage.link_brightness;
      alarm_enabled_ = storage.alarm_enabled;
      alarm_hour_ = storage.alarm_hour % 24;
      alarm_minute_ = storage.alarm_minute % 60;
      colon_mode_ = (ColonMode)(storage.colon_mode % 3);
      if (storage.timer_duration_sec > 0 && storage.timer_duration_sec <= 86400) {
        timer_duration_sec_ = storage.timer_duration_sec;
      }
      return;
    }
  }
  ESP_LOGI(TAG, "Initialized default preferences (first boot or version upgrade)");
}

void HackPackNixieClock::schedule_save_preferences_() {
  uint32_t now = millis();
  if (pref_save_start_ == 0) {
    pref_save_start_ = now;
  }
  // Enforce a hard maximum ceiling of 15 seconds even if adjustments continue
  if (now - pref_save_start_ > 15000) {
    save_preferences_now_();
  } else {
    pref_save_timeout_ = now + 5000;
  }
}

void HackPackNixieClock::save_preferences_now_() {
  pref_save_timeout_ = 0;
  pref_save_start_ = 0;
  NixieClockStorage storage;
  storage.magic = NIXIE_STORAGE_MAGIC;
  storage.version = NIXIE_STORAGE_VERSION;
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
  ESP_LOGD(TAG, "Saved preferences to flash storage");
}

// =============================================================================
// Hardware Initialization & GPIO Configuration
// =============================================================================
void HackPackNixieClock::init_hardware_() {
#if defined(USE_ESP_IDF)
  gpio_config_t out_conf{};
  out_conf.intr_type = GPIO_INTR_DISABLE;
  out_conf.mode = GPIO_MODE_OUTPUT;
  out_conf.pin_bit_mask = (1ULL << panel_pin_) | (1ULL << underglow_pin_) |
                          (1ULL << play_pin_) | (1ULL << rec_pin_);
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
// Physical Buttons Debouncing & Navigation
// =============================================================================
void HackPackNixieClock::handle_button_(ButtonState &btn, const char *name, void (HackPackNixieClock::*on_click)(), void (HackPackNixieClock::*on_long_press)(), void (HackPackNixieClock::*on_repeat)()) {
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
      btn.last_repeat_time = 0;
    }

    if (btn.is_pressed) {
      // Long press threshold: 1000ms
      if (!btn.long_press_handled && (now - btn.press_start >= 1000)) {
        btn.long_press_handled = true;
        btn.last_repeat_time = now;
        ESP_LOGD(TAG, "Button long press: %s", name);
        if (physical_buttons_enabled_ && on_long_press) {
          (this->*on_long_press)();
        }
      }

      // Auto-repeat: if held > 1000ms and on_repeat is configured, trigger every 100ms
      if (btn.long_press_handled && on_repeat) {
        if (now - btn.last_repeat_time >= 100) {
          btn.last_repeat_time = now;
          if (physical_buttons_enabled_) {
            (this->*on_repeat)();
          }
        }
      }
    }
  } else {
    if (btn.is_pressed) {
      if (!btn.long_press_handled && (now - btn.press_start < 1000)) {
        ESP_LOGD(TAG, "Button click: %s", name);
        if (physical_buttons_enabled_ && on_click) {
          (this->*on_click)();
        }
      }
      btn.is_pressed = false;
      btn.last_repeat_time = 0;
    }
  }
}

void HackPackNixieClock::poll_buttons_() {
  handle_button_(btn_top_, "Top", &HackPackNixieClock::btn_top_click_, &HackPackNixieClock::btn_top_long_, nullptr);
  handle_button_(btn_center_, "Center", &HackPackNixieClock::btn_center_click_, &HackPackNixieClock::btn_center_long_, nullptr);
  handle_button_(btn_up_, "Up", &HackPackNixieClock::btn_up_click_, nullptr, &HackPackNixieClock::btn_up_click_);
  handle_button_(btn_down_, "Down", &HackPackNixieClock::btn_down_click_, nullptr, &HackPackNixieClock::btn_down_click_);
  handle_button_(btn_left_, "Left", &HackPackNixieClock::btn_left_click_, nullptr, &HackPackNixieClock::btn_left_click_);
  handle_button_(btn_right_, "Right", &HackPackNixieClock::btn_right_click_, nullptr, &HackPackNixieClock::btn_right_click_);
}

void HackPackNixieClock::btn_top_click_() {
  if (alarm_ringing_) {
    stop_alarm();
    return;
  }
  if (timer_ringing_) {
    stop_timer();
    return;
  }
  switch (display_mode_) {
    case MODE_TIMER:
      toggle_timer();
      break;
    case MODE_ALARM:
      alarm_enabled_ = !alarm_enabled_;
      ESP_LOGI(TAG, "Alarm %s via top button", alarm_enabled_ ? "armed" : "disarmed");
      play_beep(100);
#ifdef USE_SWITCH
      if (sw_alarm_enabled_ != nullptr) sw_alarm_enabled_->publish_state(alarm_enabled_);
#endif
      schedule_save_preferences_();
      break;
    case MODE_TIME:
    default:
      break;
  }
}

void HackPackNixieClock::btn_top_long_() {
  if (display_mode_ == MODE_TIMER) {
    reset_timer();
  } else {
    play_beep(150);
  }
}

void HackPackNixieClock::btn_center_click_() {
  switch (display_mode_) {
    case MODE_TIME:
      set_display_mode(MODE_TIMER);
      break;
    case MODE_TIMER:
      set_display_mode(MODE_ALARM);
      break;
    case MODE_ALARM:
    case MODE_CUSTOM_TEXT:
    case MODE_FACE:
    case MODE_SLOT_MACHINE:
    case MODE_OFF:
    default:
      set_display_mode(MODE_TIME);
      break;
  }
}

void HackPackNixieClock::btn_center_long_() {
  trigger_slot_machine(2500);
}

void HackPackNixieClock::btn_up_click_() {
  switch (display_mode_) {
    case MODE_TIMER:
      adjust_timer_duration(60);
      break;
    case MODE_ALARM:
      adjust_alarm_time(1, 0);
      break;
    case MODE_TIME:
    default:
      set_panel_color_pos((panel_color_pos_ + 16) % 256);
      break;
  }
}

void HackPackNixieClock::btn_down_click_() {
  switch (display_mode_) {
    case MODE_TIMER:
      adjust_timer_duration(-60);
      break;
    case MODE_ALARM:
      adjust_alarm_time(-1, 0);
      break;
    case MODE_TIME:
    default:
      set_panel_color_pos((panel_color_pos_ + 240) % 256);
      break;
  }
}

void HackPackNixieClock::btn_left_click_() {
  switch (display_mode_) {
    case MODE_TIMER:
      adjust_timer_duration(-1);
      break;
    case MODE_ALARM:
      adjust_alarm_time(0, -1);
      break;
    case MODE_TIME:
    default: {
      int m = (int)color_mode_ - 1;
      if (m < 0) m = 6;
      set_color_mode((ColorMode)m);
      break;
    }
  }
}

void HackPackNixieClock::btn_right_click_() {
  switch (display_mode_) {
    case MODE_TIMER:
      adjust_timer_duration(1);
      break;
    case MODE_ALARM:
      adjust_alarm_time(0, 1);
      break;
    case MODE_TIME:
    default: {
      int m = ((int)color_mode_ + 1) % 7;
      set_color_mode((ColorMode)m);
      break;
    }
  }
}

// =============================================================================
// Public Controls & Actions
// =============================================================================
void HackPackNixieClock::set_display_mode(DisplayMode mode) {
  if (display_mode_ == mode) return;
  ESP_LOGI(TAG, "Display mode changed: %u -> %u", (unsigned int)display_mode_, (unsigned int)mode);
  display_mode_ = mode;
}

void HackPackNixieClock::set_color_mode(ColorMode mode) {
  if (color_mode_ == mode && current_animation_ != nullptr) return;
  ESP_LOGI(TAG, "Color effect changed: %u -> %u", (unsigned int)color_mode_, (unsigned int)mode);
  color_mode_ = mode;
  if (mode != COLOR_SOLID) {
    use_panel_custom_rgb_ = false;
  }
  current_animation_ = create_color_animation(color_mode_);
  if (current_animation_) {
    current_animation_->setup(this);
  }
  anim_step_ = 0;
  schedule_save_preferences_();
}

void HackPackNixieClock::set_colon_mode(ColonMode mode) {
  colon_mode_ = mode;
#ifdef USE_SELECT
  if (sel_colon_color_mode_ != nullptr) {
    switch (mode) {
      case COLON_AUTO_BLEND:
        sel_colon_color_mode_->publish_state("Auto Blend");
        break;
      case COLON_MATCH_UNDERGLOW:
        sel_colon_color_mode_->publish_state("Match Underglow");
        break;
      case COLON_FIXED:
      default:
        sel_colon_color_mode_->publish_state("Fixed");
        break;
    }
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
  link_brightness_ = false;
  panel_brightness_ = 0;
  underglow_brightness_ = 0;

  // Zero out all 42 panel LEDs and 13 underglow LEDs to eliminate power consumption during OTA flashing
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

  render_hardware_leds_();
  ESP_LOGI(TAG, "Hardware pre-flight complete: All LEDs darkened for OTA firmware write");
}

void HackPackNixieClock::set_panel_color_pos(uint8_t pos) {
  panel_color_pos_ = pos;
  use_panel_custom_rgb_ = false;
  if (current_animation_) {
    current_animation_->setup(this);
  }
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
  set_color_mode(COLOR_SOLID);
  schedule_save_preferences_();
}

void HackPackNixieClock::set_underglow_rgb(uint8_t r, uint8_t g, uint8_t b) {
  ug_custom_r_ = r; ug_custom_g_ = g; ug_custom_b_ = b;
  use_ug_custom_rgb_ = true;
  schedule_save_preferences_();
}

// =============================================================================
// Alarm & Timer Controls
// =============================================================================
void HackPackNixieClock::set_alarm(uint8_t hour, uint8_t minute) {
  alarm_hour_ = hour % 24;
  alarm_minute_ = minute % 60;
  alarm_enabled_ = true;
  alarm_ringing_ = false;
  ESP_LOGI(TAG, "Alarm set and armed for %02u:%02u", alarm_hour_, alarm_minute_);
#ifdef USE_SWITCH
  if (sw_alarm_enabled_ != nullptr) sw_alarm_enabled_->publish_state(true);
#endif
#ifdef USE_DATETIME_TIME
  if (tm_alarm_time_ != nullptr) {
    static_cast<HackPackNixieTime *>(tm_alarm_time_)->update_time(alarm_hour_, alarm_minute_);
  }
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::adjust_alarm_time(int8_t delta_hours, int8_t delta_minutes) {
  int h = (int)alarm_hour_ + delta_hours;
  while (h < 0) h += 24;
  alarm_hour_ = (uint8_t)(h % 24);

  int m = (int)alarm_minute_ + delta_minutes;
  while (m < 0) m += 60;
  alarm_minute_ = (uint8_t)(m % 60);

#ifdef USE_DATETIME_TIME
  if (tm_alarm_time_ != nullptr) {
    static_cast<HackPackNixieTime *>(tm_alarm_time_)->update_time(alarm_hour_, alarm_minute_);
  }
#endif

  if (display_mode_ == MODE_ALARM) {
    uint8_t disp_hr = alarm_hour_;
    if (!hr24_mode_) {
      disp_hr = disp_hr % 12;
      if (disp_hr == 0) disp_hr = 12;
    }
    char c0 = (disp_hr / 10) ? ('0' + (disp_hr / 10)) : (show_lead_zero_ ? '0' : ' ');
    char c1 = '0' + (disp_hr % 10);
    char c2 = '0' + (alarm_minute_ / 10);
    char c3 = '0' + (alarm_minute_ % 10);
    char c4 = ' ';
    char c5 = ' ';
    set_display_chars_(c0, c1, c2, c3, c4, c5);
  }

  schedule_save_preferences_();
}

void HackPackNixieClock::arm_alarm() {
  alarm_enabled_ = true;
  ESP_LOGI(TAG, "Alarm armed for %02u:%02u", alarm_hour_, alarm_minute_);
#ifdef USE_SWITCH
  if (sw_alarm_enabled_ != nullptr) sw_alarm_enabled_->publish_state(true);
#endif
  schedule_save_preferences_();
}

void HackPackNixieClock::disarm_alarm() {
  alarm_enabled_ = false;
  alarm_ringing_ = false;
  ESP_LOGI(TAG, "Alarm disarmed");
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
  ESP_LOGI(TAG, "Alarm stopped");
#ifdef USE_BINARY_SENSOR
  if (bs_alarm_ringing_ != nullptr) bs_alarm_ringing_->publish_state(false);
#endif
}

std::string HackPackNixieClock::get_formatted_timer_duration() const {
  uint32_t h = timer_duration_sec_ / 3600;
  uint32_t m = (timer_duration_sec_ % 3600) / 60;
  uint32_t s = timer_duration_sec_ % 60;
  char buf[32];
  if (h > 0) {
    if (m > 0 && s > 0) snprintf(buf, sizeof(buf), "%luh %lum %lus", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    else if (m > 0) snprintf(buf, sizeof(buf), "%luh %lum", (unsigned long)h, (unsigned long)m);
    else if (s > 0) snprintf(buf, sizeof(buf), "%luh %lus", (unsigned long)h, (unsigned long)s);
    else snprintf(buf, sizeof(buf), "%luh", (unsigned long)h);
  } else if (m > 0) {
    if (s > 0) snprintf(buf, sizeof(buf), "%lum %lus", (unsigned long)m, (unsigned long)s);
    else snprintf(buf, sizeof(buf), "%lum", (unsigned long)m);
  } else {
    snprintf(buf, sizeof(buf), "%lus", (unsigned long)s);
  }
  return std::string(buf);
}

void HackPackNixieClock::adjust_timer_duration(int32_t delta_sec) {
  if (timer_running_) return;

  int64_t new_dur = (int64_t)timer_duration_sec_ + delta_sec;
  if (new_dur < 10) new_dur = 10;
  if (new_dur > 86399) new_dur = 86399;

  timer_duration_sec_ = (uint32_t)new_dur;
  timer_remaining_sec_ = timer_duration_sec_;

#ifdef USE_NUMBER
  if (num_timer_duration_minutes_ != nullptr) {
    num_timer_duration_minutes_->publish_state((float)(timer_duration_sec_ / 60));
  }
#endif
#ifdef USE_TEXT
  if (txt_timer_duration_ != nullptr) {
    txt_timer_duration_->publish_state(get_formatted_timer_duration());
  }
#endif
#ifdef USE_DATETIME_TIME
  if (tm_timer_duration_ != nullptr) {
    uint8_t th = timer_duration_sec_ / 3600;
    uint8_t tm = (timer_duration_sec_ % 3600) / 60;
    uint8_t ts = timer_duration_sec_ % 60;
    static_cast<HackPackNixieTime *>(tm_timer_duration_)->update_time(th, tm, ts);
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (text_sensor_timer_remaining_ != nullptr) {
    uint32_t th = timer_remaining_sec_ / 3600;
    uint32_t tm = (timer_remaining_sec_ % 3600) / 60;
    uint32_t ts = timer_remaining_sec_ % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)th, (unsigned long)tm, (unsigned long)ts);
    text_sensor_timer_remaining_->publish_state(buf);
  }
#endif

  if (display_mode_ == MODE_TIMER) {
    uint32_t th = timer_remaining_sec_ / 3600;
    uint32_t tm = (timer_remaining_sec_ % 3600) / 60;
    uint32_t ts = timer_remaining_sec_ % 60;
    char c0 = (th / 10) ? ('0' + (th / 10)) : (show_lead_zero_ ? '0' : ' ');
    char c1 = '0' + (th % 10);
    char c2 = '0' + (tm / 10);
    char c3 = '0' + (tm % 10);
    char c4 = '0' + (ts / 10);
    char c5 = '0' + (ts % 10);
    set_display_chars_(c0, c1, c2, c3, c4, c5);
  }

  schedule_save_preferences_();
}

void HackPackNixieClock::set_timer_duration(uint32_t seconds) {
  timer_duration_sec_ = (seconds > 0) ? seconds : 300;
  if (!timer_running_) {
    timer_remaining_sec_ = timer_duration_sec_;
#ifdef USE_TEXT_SENSOR
    if (text_sensor_timer_remaining_ != nullptr) {
      uint32_t th = timer_remaining_sec_ / 3600;
      uint32_t tm = (timer_remaining_sec_ % 3600) / 60;
      uint32_t ts = timer_remaining_sec_ % 60;
      char buf[16];
      snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)th, (unsigned long)tm, (unsigned long)ts);
      text_sensor_timer_remaining_->publish_state(buf);
    }
#endif
  }
  ESP_LOGI(TAG, "Timer duration set to %u seconds (%s)", (unsigned int)timer_duration_sec_, get_formatted_timer_duration().c_str());
#ifdef USE_NUMBER
  if (num_timer_duration_minutes_ != nullptr) {
    num_timer_duration_minutes_->publish_state((float)(timer_duration_sec_ / 60));
  }
#endif
#ifdef USE_TEXT
  if (txt_timer_duration_ != nullptr) {
    txt_timer_duration_->publish_state(get_formatted_timer_duration());
  }
#endif
#ifdef USE_DATETIME_TIME
  if (tm_timer_duration_ != nullptr) {
    uint8_t th = timer_duration_sec_ / 3600;
    uint8_t tm = (timer_duration_sec_ % 3600) / 60;
    uint8_t ts = timer_duration_sec_ % 60;
    static_cast<HackPackNixieTime *>(tm_timer_duration_)->update_time(th, tm, ts);
  }
#endif
  schedule_save_preferences_();
}

uint32_t HackPackNixieClock::set_timer_duration_string(const std::string &str) {
  uint32_t s = parse_duration_string(str);
  if (s > 0) {
    set_timer_duration(s);
  }
  return timer_duration_sec_;
}

uint32_t HackPackNixieClock::parse_duration_string(const std::string &input) {
  std::string s = input;
  s.erase(0, s.find_first_not_of(" \t\r\n"));
  s.erase(s.find_last_not_of(" \t\r\n") + 1);
  if (s.empty()) return 0;

  // Check for colon format: HH:MM:SS or MM:SS
  size_t first_colon = s.find(':');
  if (first_colon != std::string::npos) {
    size_t second_colon = s.find(':', first_colon + 1);
    if (second_colon == std::string::npos) {
      uint32_t m = std::strtoul(s.substr(0, first_colon).c_str(), nullptr, 10);
      uint32_t sec = std::strtoul(s.substr(first_colon + 1).c_str(), nullptr, 10);
      return m * 60 + sec;
    } else {
      uint32_t h = std::strtoul(s.substr(0, first_colon).c_str(), nullptr, 10);
      uint32_t m = std::strtoul(s.substr(first_colon + 1, second_colon - first_colon - 1).c_str(), nullptr, 10);
      uint32_t sec = std::strtoul(s.substr(second_colon + 1).c_str(), nullptr, 10);
      return h * 3600 + m * 60 + sec;
    }
  }

  // Check for suffixed formats: e.g. "5m", "10s", "1h 30m"
  uint32_t total = 0;
  uint32_t current_num = 0;
  bool in_num = false;
  bool valid = false;

  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c >= '0' && c <= '9') {
      current_num = current_num * 10 + (c - '0');
      in_num = true;
      valid = true;
    } else if (c == 'h' || c == 'H') {
      total += current_num * 3600;
      current_num = 0;
      in_num = false;
    } else if (c == 'm' || c == 'M') {
      total += current_num * 60;
      current_num = 0;
      in_num = false;
    } else if (c == 's' || c == 'S') {
      total += current_num;
      current_num = 0;
      in_num = false;
    }
  }

  if (in_num && total == 0) {
    return current_num * 60;
  }
  if (!valid && total == 0) {
    ESP_LOGW(TAG, "Unrecognized duration string: '%s'", input.c_str());
  }
  return total;
}

void HackPackNixieClock::start_timer(uint32_t hours, uint32_t minutes, uint32_t seconds) {
  start_timer(hours * 3600 + minutes * 60 + seconds);
}

void HackPackNixieClock::start_timer(uint32_t duration_sec) {
  if (duration_sec > 0) {
    timer_duration_sec_ = duration_sec;
    timer_remaining_sec_ = duration_sec;
  } else if (timer_remaining_sec_ == 0) {
    timer_remaining_sec_ = (timer_duration_sec_ > 0) ? timer_duration_sec_ : 300;
  }

  timer_end_millis_ = millis() + (timer_remaining_sec_ * 1000);
  timer_running_ = true;
  timer_ringing_ = false;

  ESP_LOGI(TAG, "Countdown timer started/resumed (%u seconds remaining)", (unsigned int)timer_remaining_sec_);

#ifdef USE_SWITCH
  if (sw_timer_running_ != nullptr) sw_timer_running_->publish_state(true);
#endif
#ifdef USE_BINARY_SENSOR
  if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(false);
#endif
}

void HackPackNixieClock::stop_timer() {
  if (timer_ringing_) {
    timer_ringing_ = false;
    ESP_LOGI(TAG, "Timer alarm dismissed; returning to mode %u", (unsigned int)timer_return_mode_);
    set_display_mode(timer_return_mode_);
  } else if (timer_running_) {
    uint32_t now_ms = millis();
    uint32_t left_ms = (timer_end_millis_ > now_ms) ? (timer_end_millis_ - now_ms) : 0;
    timer_remaining_sec_ = (left_ms + 999) / 1000;
    ESP_LOGI(TAG, "Countdown timer paused (%u seconds remaining)", (unsigned int)timer_remaining_sec_);
  }
  timer_running_ = false;

#ifdef USE_SWITCH
  if (sw_timer_running_ != nullptr) sw_timer_running_->publish_state(false);
#endif
#ifdef USE_BINARY_SENSOR
  if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(false);
#endif
#ifdef USE_TEXT_SENSOR
  if (text_sensor_timer_remaining_ != nullptr) {
    uint32_t th = timer_remaining_sec_ / 3600;
    uint32_t tm = (timer_remaining_sec_ % 3600) / 60;
    uint32_t ts = timer_remaining_sec_ % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)th, (unsigned long)tm, (unsigned long)ts);
    text_sensor_timer_remaining_->publish_state(buf);
  }
#endif
}

void HackPackNixieClock::reset_timer() {
  timer_running_ = false;
  timer_ringing_ = false;
  timer_remaining_sec_ = (timer_duration_sec_ > 0) ? timer_duration_sec_ : 300;
  ESP_LOGI(TAG, "Countdown timer reset to %u seconds", (unsigned int)timer_remaining_sec_);
  play_beep(150);

#ifdef USE_SWITCH
  if (sw_timer_running_ != nullptr) sw_timer_running_->publish_state(false);
#endif
#ifdef USE_BINARY_SENSOR
  if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(false);
#endif
#ifdef USE_TEXT_SENSOR
  if (text_sensor_timer_remaining_ != nullptr) {
    uint32_t th = timer_remaining_sec_ / 3600;
    uint32_t tm = (timer_remaining_sec_ % 3600) / 60;
    uint32_t ts = timer_remaining_sec_ % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)th, (unsigned long)tm, (unsigned long)ts);
    text_sensor_timer_remaining_->publish_state(buf);
  }
#endif
  if (display_mode_ == MODE_TIMER) {
    uint32_t th = timer_remaining_sec_ / 3600;
    uint32_t tm = (timer_remaining_sec_ % 3600) / 60;
    uint32_t ts = timer_remaining_sec_ % 60;
    char c0 = (th / 10) ? ('0' + (th / 10)) : (show_lead_zero_ ? '0' : ' ');
    char c1 = '0' + (th % 10);
    char c2 = '0' + (tm / 10);
    char c3 = '0' + (tm % 10);
    char c4 = '0' + (ts / 10);
    char c5 = '0' + (ts % 10);
    set_display_chars_(c0, c1, c2, c3, c4, c5);
  }
}

void HackPackNixieClock::toggle_timer() {
  if (alarm_ringing_) {
    stop_alarm();
    return;
  }
  if (timer_ringing_) {
    stop_timer();
    return;
  }
  if (timer_running_) {
    stop_timer();
  } else {
    start_timer();
  }
}

// =============================================================================
// Sequences, Messages & Audio
// =============================================================================
void HackPackNixieClock::play_beep(uint32_t duration_ms) {
  beep_end_ = millis() + (duration_ms > 0 ? duration_ms : 150);
}

void HackPackNixieClock::set_record_sound(bool active) {
#if defined(USE_ESP_IDF)
  gpio_set_level((gpio_num_t)rec_pin_, active ? 1 : 0);
#else
  digitalWrite(rec_pin_, active ? HIGH : LOW);
#endif
#ifdef USE_SWITCH
  if (sw_record_sound_ != nullptr) sw_record_sound_->publish_state(active);
#endif
}

void HackPackNixieClock::show_scrolling_text(const std::string &message, uint32_t scroll_speed_ms) {
  // Capture return mode only if currently in a persistent base mode
  switch (display_mode_) {
    case MODE_TIME:
    case MODE_TIMER:
    case MODE_ALARM:
    case MODE_OFF:
      return_mode_ = display_mode_;
      break;
    default:
      break;
  }
  display_mode_ = MODE_CUSTOM_TEXT;
  text_message_ = message;
  scroll_speed_ms_ = (scroll_speed_ms > 0) ? scroll_speed_ms : 350;
  text_phase_ = TEXT_PHASE_BLANK_START;
  text_phase_end_ = millis() + 1000;
  show_string_("      ");
  std::string sanitized = sanitize_nixie_text(message);
  ESP_LOGI(TAG, "Displaying message: '%s' (sanitized: '%s', speed %ums)",
           message.c_str(), sanitized.c_str(), (unsigned int)scroll_speed_ms_);
}

void HackPackNixieClock::show_custom_text(const std::string &text, uint32_t duration_seconds) {
  show_scrolling_text(text, 350);
}

void HackPackNixieClock::trigger_face_animation() {
  switch (display_mode_) {
    case MODE_TIME:
    case MODE_TIMER:
    case MODE_ALARM:
    case MODE_OFF:
      return_mode_ = display_mode_;
      break;
    default:
      break;
  }
  display_mode_ = MODE_FACE;
  face_step_ = 0;
  show_string_("o __o ");
  face_step_end_ = millis() + 1000;
}

void HackPackNixieClock::trigger_slot_machine(uint32_t duration_ms) {
  switch (display_mode_) {
    case MODE_TIME:
    case MODE_TIMER:
    case MODE_ALARM:
    case MODE_OFF:
      return_mode_ = display_mode_;
      break;
    default:
      break;
  }
  display_mode_ = MODE_SLOT_MACHINE;
  slot_machine_end_ = millis() + duration_ms;
  last_slot_step_ = 0;
  slot_digit_ = 0;
  ESP_LOGI(TAG, "Cathode anti-poisoning slot machine triggered (%ums)", (unsigned int)duration_ms);
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
          show_string_("- - - ");
        }
        break;

      case TEXT_PHASE_FLASH_ALERT:
        if (now_ms - flash_toggle_time_ >= 250) {
          flash_toggle_time_ = now_ms;
          flash_state_ = !flash_state_;
          show_string_(flash_state_ ? "- - - " : "      ");
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
        case 1: show_string_(" o__ o"); face_step_end_ = now_ms + 1000; break;
        case 2: show_string_(" o  o "); face_step_end_ = now_ms + 500;  break;
        case 3: show_string_(" -  - "); face_step_end_ = now_ms + 250;  break;
        case 4: show_string_(" o  o "); face_step_end_ = now_ms + 1500; break;
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

  // 4. Background Countdown Timer Step (independent of display mode)
  if (timer_running_) {
    if (now_ms >= timer_end_millis_) {
      timer_running_ = false;
      timer_ringing_ = true;
      timer_ring_start_ = now_ms;
      timer_remaining_sec_ = 0;
      timer_return_mode_ = display_mode_;
      ESP_LOGI(TAG, "Timer reached zero - Alarm ringing! (capturing return mode: %u)", (unsigned int)timer_return_mode_);
      set_display_mode(MODE_TIMER);

#ifdef USE_SWITCH
      if (sw_timer_running_ != nullptr) sw_timer_running_->publish_state(false);
#endif
#ifdef USE_BINARY_SENSOR
      if (bs_timer_ringing_ != nullptr) bs_timer_ringing_->publish_state(true);
#endif
#ifdef USE_TEXT_SENSOR
      if (text_sensor_timer_remaining_ != nullptr) text_sensor_timer_remaining_->publish_state("00:00:00");
#endif
    } else {
      uint32_t left_ms = timer_end_millis_ - now_ms;
      uint32_t prev_sec = timer_remaining_sec_;
      timer_remaining_sec_ = (left_ms + 999) / 1000;
      uint32_t tot_sec = left_ms / 1000;
      uint32_t th = tot_sec / 3600;
      uint32_t tm = (tot_sec % 3600) / 60;
      uint32_t ts = tot_sec % 60;

      if (timer_remaining_sec_ != prev_sec) {
#ifdef USE_TEXT_SENSOR
        if (text_sensor_timer_remaining_ != nullptr) {
          char buf[16];
          snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)th, (unsigned long)tm, (unsigned long)ts);
          text_sensor_timer_remaining_->publish_state(buf);
        }
#endif
      }
    }
  }

  // 5. Render Active Display Mode
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
        if (alarm_enabled_ && (now_ms - last_sync_warn_time_ > 60000)) {
          last_sync_warn_time_ = now_ms;
          ESP_LOGW(TAG, "Alarm is armed, but real-time clock is uncalibrated/unreachable!");
        }
      }
      break;
    }

    case MODE_TIMER: {
      if (timer_ringing_) {
        if ((now_ms / 500) % 2 == 0) {
          set_display_chars_('0', '0', '0', '0', '0', '0');
        } else {
          show_string_("      ");
        }
      } else {
        uint32_t disp_sec = timer_remaining_sec_;
        uint32_t th = disp_sec / 3600;
        uint32_t tm = (disp_sec % 3600) / 60;
        uint32_t ts = disp_sec % 60;
        char c0 = (th / 10) ? ('0' + (th / 10)) : (show_lead_zero_ ? '0' : ' ');
        char c1 = '0' + (th % 10);
        char c2 = '0' + (tm / 10);
        char c3 = '0' + (tm % 10);
        char c4 = '0' + (ts / 10);
        char c5 = '0' + (ts % 10);
        set_display_chars_(c0, c1, c2, c3, c4, c5);
      }
      break;
    }

    case MODE_ALARM: {
      uint8_t disp_hr = alarm_hour_;
      if (!hr24_mode_) {
        disp_hr = disp_hr % 12;
        if (disp_hr == 0) disp_hr = 12;
      }
      char c0 = (disp_hr / 10) ? ('0' + (disp_hr / 10)) : (show_lead_zero_ ? '0' : ' ');
      char c1 = '0' + (disp_hr % 10);
      char c2 = '0' + (alarm_minute_ / 10);
      char c3 = '0' + (alarm_minute_ % 10);
      char c4 = ' ';
      char c5 = ' ';
      set_display_chars_(c0, c1, c2, c3, c4, c5);
      break;
    }

    case MODE_CUSTOM_TEXT:
    case MODE_FACE:
    case MODE_SLOT_MACHINE:
      break;

    case MODE_OFF:
    default:
      show_string_("      ");
      break;
  }

  // 5. Alarm Trigger Check
  if (alarm_enabled_ && now_time.is_valid()) {
    if (now_time.hour == alarm_hour_ && now_time.minute == alarm_minute_ && now_time.second == 0 && !alarm_ringing_) {
      alarm_ringing_ = true;
      alarm_ring_start_ = now_ms;
      ESP_LOGI(TAG, "Alarm triggered for %02u:%02u!", alarm_hour_, alarm_minute_);
#ifdef USE_BINARY_SENSOR
      if (bs_alarm_ringing_ != nullptr) bs_alarm_ringing_->publish_state(true);
#endif
    }
  }
}

// =============================================================================
// Underglow, Colons & Indicators
// =============================================================================
void HackPackNixieClock::update_colons_(uint32_t base_ug) {
  bool colons_active = false;

  switch (display_mode_) {
    case MODE_TIME:
    case MODE_TIMER:
    case MODE_ALARM:
      colons_active = (!colon_blinking_ || colon_blink_state_);
      break;

    case MODE_CUSTOM_TEXT:
    case MODE_FACE:
    case MODE_SLOT_MACHINE:
    case MODE_OFF:
    default:
      colons_active = false;
      break;
  }

  // Double safety: Keep colons off whenever a message sequence is active
  if (text_phase_ != TEXT_PHASE_IDLE) {
    colons_active = false;
  }

  if (colons_active) {
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
}

void HackPackNixieClock::update_ampm_indicators_(const ESPTime &now_time) {
  bool ampm_active = false;

  switch (display_mode_) {
    case MODE_TIME:
      ampm_active = am_pm_enabled_ && now_time.is_valid() && !timer_running_ && (text_phase_ == TEXT_PHASE_IDLE);
      break;

    case MODE_TIMER:
    case MODE_ALARM:
    case MODE_CUSTOM_TEXT:
    case MODE_FACE:
    case MODE_SLOT_MACHINE:
    case MODE_OFF:
    default:
      ampm_active = false;
      break;
  }

  if (ampm_active) {
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
    underglow_rgb_[12][0] = 0; underglow_rgb_[12][0] = 0; underglow_rgb_[12][2] = 0;
  }
}

void HackPackNixieClock::update_underglow_() {
  ESPTime now_time = time_source_ ? time_source_->now() : ESPTime{};
  uint8_t sec = now_time.is_valid() ? now_time.second : 0;
  if (sec != last_sec_) {
    last_sec_ = sec;
    colon_blink_state_ = !colon_blink_state_;
  }

  uint32_t base_ug = use_ug_custom_rgb_ ? make_rgb_(ug_custom_r_, ug_custom_g_, ug_custom_b_) : wheel_(underglow_color_pos_);

  // 1. Colon LEDs (0 and 1)
  update_colons_(base_ug);

  // 2. Badge Glow LEDs (2..9)
  for (int i = 2; i < 10; i++) {
    underglow_rgb_[i][0] = red_(base_ug);
    underglow_rgb_[i][1] = green_(base_ug);
    underglow_rgb_[i][2] = blue_(base_ug);
  }

  // 3. Alarm Indicator LED (10)
  if (alarm_enabled_ || alarm_ringing_) {
    underglow_rgb_[10][0] = 255;
    underglow_rgb_[10][1] = 0;
    underglow_rgb_[10][2] = 0;
  } else {
    underglow_rgb_[10][0] = 0;
    underglow_rgb_[10][1] = 0;
    underglow_rgb_[10][2] = 0;
  }

  // 4. AM/PM Indicator LEDs (11 and 12)
  update_ampm_indicators_(now_time);
}

// =============================================================================
// Hardware WS2812 Rendering Pipeline
// =============================================================================
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

  // 1. Transmit Panel LEDs (GPIO0)
  ws2812_transmit(panel_pin_, panel_bytes, sizeof(panel_bytes));

  // Brief yield allowing any pending hardware interrupts (e.g. WiFi/BLE) to service
  esp_rom_delay_us(15);

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

  // 2. Transmit Underglow LEDs (GPIO1)
  ws2812_transmit(underglow_pin_, ug_bytes, sizeof(ug_bytes));
#endif
}

// =============================================================================
// Helper Functions: Animation, Colors, Glyphs
// =============================================================================
void HackPackNixieClock::set_display_chars_(char c0, char c1, char c2, char c3, char c4, char c5) {
  current_display_chars_[0] = c0;
  current_display_chars_[1] = c1;
  current_display_chars_[2] = c2;
  current_display_chars_[3] = c3;
  current_display_chars_[4] = c4;
  current_display_chars_[5] = c5;
  current_display_chars_[6] = '\0';

  for (int p = 0; p < 6; p++) {
    bool segs[7];
    map_char_to_segments(current_display_chars_[p], segs);
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

uint32_t HackPackNixieClock::average_panel_color_(int panelIndex) const {
  if (panelIndex < 0 || panelIndex >= 6) return 0;
  uint16_t sumR = 0, sumG = 0, sumB = 0;
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

void HackPackNixieClock::step_animation_() {
  if (!current_animation_) {
    current_animation_ = create_color_animation(color_mode_);
    if (current_animation_) {
      current_animation_->setup(this);
    }
    anim_step_ = 0;
  }

  if (!current_animation_) return;

  uint32_t now = millis();
  if (now - last_anim_update_ < current_animation_->get_frame_ms()) return;
  last_anim_update_ = now;

  current_animation_->step(this, anim_step_, current_animation_->get_total_steps());

  anim_step_++;
  if (anim_step_ >= current_animation_->get_total_steps()) {
    anim_step_ = 0;
    current_animation_->on_complete(this);
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
    case SwitchType::TIMER_RUNNING: sw_timer_running_ = sw; break;
  }
}
#endif

#ifdef USE_SELECT
void HackPackNixieClock::register_select(SelectType type, select::Select *sel) {
  switch (type) {
    case SelectType::COLON_COLOR_MODE: sel_colon_color_mode_ = sel; break;
  }
}
#endif

#ifdef USE_NUMBER
void HackPackNixieClock::register_number(NumberType type, number::Number *num) {
  switch (type) {
    case NumberType::TIMER_DURATION_MINUTES:
      num_timer_duration_minutes_ = num;
      break;
  }
}
#endif

#ifdef USE_TEXT
void HackPackNixieClock::register_text(TextType type, text::Text *txt) {
  switch (type) {
    case TextType::TIMER_DURATION:
      txt_timer_duration_ = txt;
      break;
  }
}
#endif

#ifdef USE_DATETIME_TIME
void HackPackNixieClock::register_time(TimeType type, datetime::TimeEntity *tm) {
  switch (type) {
    case TimeType::ALARM_TIME:
      tm_alarm_time_ = tm;
      break;
    case TimeType::TIMER_DURATION:
      tm_timer_duration_ = tm;
      break;
  }
}
#endif

void HackPackNixieClock::register_light(LightType type, light::LightState *st) {
  switch (type) {
    case LightType::PANEL:
      panel_light_ = st;
      if (st != nullptr) st->add_remote_values_listener(&panel_listener_);
      break;
    case LightType::UNDERGLOW:
      underglow_light_ = st;
      if (st != nullptr) st->add_remote_values_listener(&underglow_listener_);
      break;
  }
}

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
