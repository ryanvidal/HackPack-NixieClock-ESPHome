#include "entities.h"
#include "hack_pack_nixie_clock.h"
#include <cstdio>

namespace esphome {
namespace hack_pack_nixie_clock {

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
  switch (type_) {
    case SelectType::DISPLAY_MODE:
      publish_state("Time");
      break;
    case SelectType::COLON_COLOR_MODE: {
      ColonMode cm = parent_->get_colon_mode();
      switch (cm) {
        case COLON_AUTO_BLEND: publish_state("Auto Blend"); break;
        case COLON_MATCH_UNDERGLOW: publish_state("Match Underglow"); break;
        case COLON_FIXED: default: publish_state("Fixed"); break;
      }
      break;
    }
  }
}

void HackPackNixieSelect::control(const std::string &value) {
  if (!parent_) return;
  switch (type_) {
    case SelectType::DISPLAY_MODE:
      if (value == "Time") parent_->set_display_mode(MODE_TIME);
      else if (value == "Timer") parent_->set_display_mode(MODE_TIMER);
      else if (value == "Alarm View") parent_->set_display_mode(MODE_ALARM);
      else if (value == "Slot Machine") parent_->trigger_slot_machine(2500);
      else if (value == "Faces") parent_->trigger_face_animation();
      else if (value == "Custom Text") parent_->set_display_mode(MODE_CUSTOM_TEXT);
      else if (value == "Off") parent_->set_display_mode(MODE_OFF);
      break;

    case SelectType::COLON_COLOR_MODE:
      if (value == "Auto Blend") parent_->set_colon_mode(COLON_AUTO_BLEND);
      else if (value == "Match Underglow") parent_->set_colon_mode(COLON_MATCH_UNDERGLOW);
      else if (value == "Fixed") parent_->set_colon_mode(COLON_FIXED);
      break;
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

void HackPackNixieTime::update_time(uint8_t hour, uint8_t minute) {
  this->hour_ = hour;
  this->minute_ = minute;
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
