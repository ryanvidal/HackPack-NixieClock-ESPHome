#pragma once

#include <cstdint>
#include <string>

namespace esphome {
namespace hack_pack_nixie_clock {

/// @brief Primary operating modes for the 6-digit Nixie tube display.
enum DisplayMode : uint8_t {
  MODE_TIME = 0,        ///< Real-time clock (HH:MM:SS or HH:MM)
  MODE_TIMER,           ///< Countdown timer
  MODE_ALARM,           ///< Alarm configuration & view mode
  MODE_CUSTOM_TEXT,     ///< Scrolling text / notification message
  MODE_FACE,            ///< Animated expressive ASCII faces
  MODE_SLOT_MACHINE,    ///< Cathode anti-poisoning cycling animation
  MODE_OFF              ///< Tubes dark / standby mode
};

/// @brief Built-in color effects for the 7-segment WS2812 panel LEDs.
enum ColorMode : uint8_t {
  COLOR_RAINBOW = 0,    ///< Smooth rainbow spectrum wave
  COLOR_SOLID,          ///< Static custom RGB color
  COLOR_GRADIENT,       ///< 2-color smooth gradient across tubes
  COLOR_FLOW,           ///< Flowing animated multi-color gradient
  COLOR_WIPE,           ///< Tube-by-tube color wipe animation
  COLOR_PULSE,          ///< Expanding center-out color pulse
  COLOR_BOUNCE          ///< Bouncing color streak
};

/// @brief Blending and color behavior for the inter-tube colon LEDs.
enum ColonMode : uint8_t {
  COLON_AUTO_BLEND = 0,     ///< Automatically blends color of adjacent tubes
  COLON_MATCH_UNDERGLOW,    ///< Follows the lower badge underglow color
  COLON_FIXED               ///< Uses fixed configured color
};

/// @brief State machine phases for scrolling text messages.
enum CustomTextPhase : uint8_t {
  TEXT_PHASE_IDLE = 0,      ///< No message active
  TEXT_PHASE_BLANK_START,   ///< Initial 1-second blank pause
  TEXT_PHASE_FLASH_ALERT,   ///< 2-second pulsating "- - - " alert banner
  TEXT_PHASE_SCROLL,        ///< Character-by-character window scrolling
  TEXT_PHASE_BLANK_END      ///< Trailing 1-second blank pause
};

/// @brief Hardware LED bus identifier.
enum class LightType : uint8_t {
  PANEL = 0,        ///< 42 WS2812 LEDs on GPIO0 (6 tubes x 7 segments)
  UNDERGLOW = 1     ///< 13 WS2812 LEDs on GPIO1 (2 colons, 8 badge, 3 indicators)
};

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

enum class ButtonType : uint8_t {
  START_TIMER,
  STOP_TIMER,
  STOP_ALARM,
  TRIGGER_FACE,
  TRIGGER_SLOT_MACHINE,
  PLAY_SOUND,
};

enum class SelectType : uint8_t {
  COLON_COLOR_MODE,
};

enum class NumberType : uint8_t {
  TIMER_DURATION_MINUTES,
};

enum class TextType : uint8_t {
  TIMER_DURATION,
};

enum class TimeType : uint8_t {
  ALARM_TIME,
};

/// @brief Physical button debouncing state tracker.
struct ButtonState {
  uint8_t pin{0};
  bool last_reading{true};
  bool is_pressed{false};
  uint32_t press_start{0};
  bool long_press_handled{false};
};

// =============================================================================
// NVS Storage Schema Versioning
// IMPORTANT: Increment NIXIE_STORAGE_VERSION whenever changing the fields, order,
// or data types in NixieClockStorage to prevent reading stale flash data!
// =============================================================================
#define NIXIE_STORAGE_MAGIC 0x4E58   ///< 'NX' magic identifier
#define NIXIE_STORAGE_VERSION 1      ///< Current storage schema version

/// @brief Non-volatile storage struct for persistent user preferences.
/// Includes magic header and versioning to prevent flash corruption across upgrades.
struct NixieClockStorage {
  uint16_t magic{NIXIE_STORAGE_MAGIC};
  uint16_t version{NIXIE_STORAGE_VERSION};
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

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
