#pragma once

#include "esphome/core/component.h"
#include "types.h"

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

namespace esphome {
namespace hack_pack_nixie_clock {

class HackPackNixieClock;

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

#ifdef USE_BUTTON
class HackPackNixieButton : public button::Button, public Component {
public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(ButtonType type) { type_ = type; }
  void press_action() override;

protected:
  HackPackNixieClock *parent_{nullptr};
  ButtonType type_{ButtonType::RESET_TIMER};
};
#endif

#ifdef USE_SELECT
class HackPackNixieSelect : public select::Select, public Component {
public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(SelectType type) { type_ = type; }
  void setup() override;
  void control(const std::string &value) override;

protected:
  HackPackNixieClock *parent_{nullptr};
  SelectType type_{SelectType::COLON_COLOR_MODE};
};
#endif

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

#ifdef USE_DATETIME_TIME
class HackPackNixieTime : public datetime::TimeEntity, public Component {
public:
  void set_parent(HackPackNixieClock *parent) { parent_ = parent; }
  void set_type(TimeType type) { type_ = type; }
  void setup() override;
  void control(const datetime::TimeCall &call) override;
  void update_time(uint8_t hour, uint8_t minute, uint8_t second = 0);

protected:
  HackPackNixieClock *parent_{nullptr};
  TimeType type_{TimeType::ALARM_TIME};
};
#endif

} // namespace hack_pack_nixie_clock
} // namespace esphome
