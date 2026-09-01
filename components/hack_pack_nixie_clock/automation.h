#pragma once

#include "esphome/core/automation.h"
#include "hack_pack_nixie_clock.h"
#include "types.h"
#include <string>

namespace esphome {
namespace hack_pack_nixie_clock {

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
