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
  'w','x','y','z','-','_','^','@','#','<','>','O'
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
  {0, 1, 1, 0, 1, 0, 1}  // 'O'
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
  setup_animation_();
  next_periodic_face_ = millis() + 30000;
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

  // 1. Poll Physical Buttons
  poll_buttons_();

  // 2. Step Animation Engine
  step_animation_();

  // 3. Update Display & Sequence State
  update_display_state_();

  // 4. Update Underglow & Colons
  update_underglow_();

  // 5. Beep Pulse Management
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

  // 6. Alarm/Timer Repeating Beeper
  if (alarm_ringing_ || timer_ringing_) {
    uint32_t start_ref = alarm_ringing_ ? alarm_ring_start_ : timer_ring_start_;
    if ((now_ms - start_ref) % 10000 < 100) {
      if (beep_end_ == 0) play_beep(100);
    }
  }

  // 7. Render Hardware LEDs at ~50Hz (20ms intervals)
  static uint32_t last_render = 0;
  if (now_ms - last_render >= 20) {
    last_render = now_ms;
    render_hardware_leds_();
  }
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
// Sequences & Actions
// =============================================================================
void HackPackNixieClock::set_display_mode(DisplayMode mode) {
  display_mode_ = mode;
}

void HackPackNixieClock::set_color_mode(ColorMode mode) {
  color_mode_ = mode;
  anim_mode_changed_ = true;
}

void HackPackNixieClock::set_panel_rgb(uint8_t r, uint8_t g, uint8_t b) {
  custom_r_ = r; custom_g_ = g; custom_b_ = b;
  use_panel_custom_rgb_ = true;
}

void HackPackNixieClock::set_underglow_rgb(uint8_t r, uint8_t g, uint8_t b) {
  ug_custom_r_ = r; ug_custom_g_ = g; ug_custom_b_ = b;
  use_ug_custom_rgb_ = true;
}

void HackPackNixieClock::set_alarm(uint8_t hour, uint8_t minute) {
  alarm_hour_ = hour % 24;
  alarm_minute_ = minute % 60;
  alarm_enabled_ = true;
  alarm_ringing_ = false;
}

void HackPackNixieClock::disarm_alarm() {
  alarm_enabled_ = false;
  alarm_ringing_ = false;
}

void HackPackNixieClock::stop_alarm() {
  alarm_ringing_ = false;
}

void HackPackNixieClock::start_timer(uint32_t hours, uint32_t minutes, uint32_t seconds) {
  uint32_t tot = hours * 3600 + minutes * 60 + seconds;
  if (tot == 0) tot = 300;
  timer_duration_sec_ = tot;
  timer_end_millis_ = millis() + (timer_duration_sec_ * 1000);
  timer_running_ = true;
  timer_ringing_ = false;
  set_display_mode(MODE_TIMER);
}

void HackPackNixieClock::stop_timer() {
  timer_running_ = false;
  timer_ringing_ = false;
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

void HackPackNixieClock::show_custom_text(const std::string &text, uint32_t duration_seconds) {
  if (display_mode_ != MODE_CUSTOM_TEXT) return_mode_ = display_mode_;
  display_mode_ = MODE_CUSTOM_TEXT;
  show_string_(text.c_str());
  custom_text_expiry_ = (duration_seconds > 0) ? (millis() + duration_seconds * 1000) : 0;
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

  // 1. Custom Text Expiry
  if (display_mode_ == MODE_CUSTOM_TEXT && custom_text_expiry_ > 0 && now_ms >= custom_text_expiry_) {
    display_mode_ = return_mode_;
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
          set_display_chars_('0', '0', '0', '0', '0', '0');
        } else {
          uint32_t left_ms = timer_end_millis_ - now_ms;
          timer_remaining_sec_ = (left_ms + 999) / 1000;
          uint32_t tot_sec = left_ms / 1000;
          uint32_t th = tot_sec / 3600;
          uint32_t tm = (tot_sec % 3600) / 60;
          uint32_t ts = tot_sec % 60;

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
      underglow_rgb_[12][0] = 0;   underglow_rgb_[12][0] = 0;   underglow_rgb_[12][2] = 0;
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

void HackPackNixieClock::render_hardware_leds_() {
#if defined(USE_ESP_IDF)
  // Convert 42 Panel LEDs (GRB) to byte buffer
  uint8_t panel_bytes[42 * 3];
  int idx = 0;
  for (int p = 0; p < 6; p++) {
    for (int s = 0; s < 7; s++) {
      uint8_t r = 0, g = 0, b = 0;
      if (panel_segments_[p][s] && display_mode_ != MODE_OFF) {
        r = (uint8_t)((uint16_t)panel_rgb_[p][s][0] * panel_brightness_ / 255);
        g = (uint8_t)((uint16_t)panel_rgb_[p][s][1] * panel_brightness_ / 255);
        b = (uint8_t)((uint16_t)panel_rgb_[p][s][2] * panel_brightness_ / 255);
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
    uint8_t r = (uint8_t)((uint16_t)underglow_rgb_[i][0] * underglow_brightness_ / 255);
    uint8_t g = (uint8_t)((uint16_t)underglow_rgb_[i][1] * underglow_brightness_ / 255);
    uint8_t b = (uint8_t)((uint16_t)underglow_rgb_[i][2] * underglow_brightness_ / 255);
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
  current_display_chars_[0] = c0;
  current_display_chars_[1] = c1;
  current_display_chars_[2] = c2;
  current_display_chars_[3] = c3;
  current_display_chars_[4] = c4;
  current_display_chars_[5] = c5;
  current_display_chars_[6] = '\0';

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

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
