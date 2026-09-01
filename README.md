# Hack Pack Nixie Clock (Box 15) ESPHome Custom Component

[![ESPHome Version](https://img.shields.io/badge/ESPHome-2024%2B-blue.svg)](https://esphome.io)
[![Platform](https://img.shields.io/badge/Platform-ESP32--C3-orange.svg)](https://espressif.com)
[![Framework](https://img.shields.io/badge/Framework-ESP--IDF-green.svg)](https://esphome.io/components/esp32.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An ESPHome custom component for the **CrunchLabs Hack Pack Box 15 (Smart Nixie Clock)** based on the **ESP32-C3 Mini**. 

This integration replaces the stock standalone web server with a native **Home Assistant** integration while preserving and enhancing all animations, underglow effects, 7-segment character glyphs, timers, alarms, audio module controls, and physical button navigation.

---

## Features

- 🕒 **Native Time & Date Synchronization**: Automatically keeps accurate time synced from Home Assistant / NTP with 12-hour and 24-hour modes.
- 🎨 **7 Color Animation Modes**: `Rainbow`, `Solid`, `Gradient`, `Flow`, `Wipe`, `Pulse`, and `Bounce`.
- 💡 **Dynamic Underglow & Colons**: Underglow color control, AM/PM indicators, colon blinking, and automatic inter-panel color blending.
- ⏰ **Smart Alarm & Countdown Timers**: Native Home Assistant time-picker widget for alarm, countdown timers with live formatted `"HH:MM:SS"` sensors and audio beeper notifications.
- 🎙️ **Voice Recording & Playback**: Integrated control for the onboard sound recorder module.
- 🕹️ **Local Physical Buttons**: Full button debouncing and hardware navigation preserved, with the ability to lock/disable buttons from Home Assistant.
- 🚀 **Flicker-Free IRAM Driver**: Custom cycle-accurate WS2812 bit-banging engine running in internal RAM (`IRAM_ATTR`), eliminating FreeRTOS/WiFi single-core preemption and digit flickering.
- 💬 **Custom Text & Animation Services**: Send custom messages to the nixie tubes or trigger face animations and cathode cleaning from Home Assistant automations.

---

## Repository Structure

To use this repo as a remote ESPHome external component, the directory structure is organized as follows:

```text
hack-pack-nixie-clock/
├── components/
│   └── hack_pack_nixie_clock/
│       ├── __init__.py
│       ├── hack_pack_nixie_clock.h
│       └── hack_pack_nixie_clock.cpp
├── examples/
│   └── nixie_clock_complete.yaml
└── README.md
```

---

## Quickstart

Add this repository to your ESPHome configuration using `external_components`:

```yaml
external_components:
  - source: github://YOUR_GITHUB_USERNAME/hack-pack-nixie-clock
    components: [ hack_pack_nixie_clock ]

time:
  - platform: homeassistant
    id: homeassistant_time

# All pins automatically default to the stock Hack Pack Box 15 hardware!
hack_pack_nixie_clock:
  id: clock_hub
  time_id: homeassistant_time
```

---

## Hardware Pin Mapping (Hack Pack Box 15 Defaults)

Every pin is configured to the stock hardware by default, but can be overridden in YAML for custom hardware:

| Pin Name in YAML | Default GPIO | Function |
| :--- | :--- | :--- |
| `panel_pin` | `GPIO0` | 42 WS2812 LEDs (6 Nixie tubes × 7 segments) |
| `underglow_pin` | `GPIO1` | 13 WS2812 LEDs (2 Colons, 8 Badge, 1 Alarm, 1 AM, 1 PM) |
| `play_pin` | `GPIO5` | Voice module trigger output |
| `rec_pin` | `GPIO4` | Voice module record output |
| `btn_top_pin` | `GPIO19` | Top Snooze / Stop button (Active LOW) |
| `btn_center_pin` | `GPIO10` | D-pad Center button (Active LOW) |
| `btn_up_pin` | `GPIO9` | D-pad Up button (Active LOW) |
| `btn_down_pin` | `GPIO8` | D-pad Down button (Active LOW) |
| `btn_left_pin` | `GPIO7` | D-pad Left button (Active LOW) |
| `btn_right_pin` | `GPIO6` | D-pad Right button (Active LOW) |

---

## Home Assistant Entities

### Controls & Settings

| Domain | Entity Name | Description |
| :--- | :--- | :--- |
| **DateTime** | `Alarm Time` | Native time picker widget to set alarm time. |
| **Number** | `Panel Brightness` | Adjusts tube panel brightness (0–100%). |
| **Number** | `Underglow Brightness` | Adjusts underglow brightness (0–100%). |
| **Number** | `Panel Color Position` | Color wheel position (0–255) for the tubes. |
| **Number** | `Underglow Color Position` | Color wheel position (0–255) for the underglow. |
| **Number** | `Timer Hours / Min / Sec` | Sets countdown timer duration. |
| **Select** | `Color Animation Mode` | Selects: `Rainbow`, `Solid`, `Gradient`, `Flow`, `Wipe`, `Pulse`, `Bounce`. |
| **Select** | `Display Mode` | Selects: `Time`, `Timer`, `Alarm View`, `Slot Machine`, `Faces`, `Custom Text`, `Off`. |
| **Select** | `Colon Color Mode` | Selects: `Auto Blend`, `Match Underglow`, `Fixed`. |
| **Switch** | `24-Hour Format` | Toggles 12-hour vs 24-hour time format. |
| **Switch** | `Leading Zero` | Toggles leading zero on single-digit hours (`" 9:00"` vs `"09:00"`). |
| **Switch** | `Colon Blinking` | Enables/disables 1Hz blinking colons. |
| **Switch** | `AM⁄PM Indicators` | Enables/disables amber AM and purple PM LEDs. |
| **Switch** | `Periodic Face Animations` | Enables/disables automatic 30s periodic face animations. |
| **Switch** | `Alarm Enabled` | Arms or disarms the alarm. |
| **Switch** | `Physical Buttons Enabled` | Enables or locks local button inputs. |
| **Switch** | `Record Sound (Hold)` | Holds `GPIO 4` high to record audio message. |

### Actions & Triggers

| Domain | Entity Name | Description |
| :--- | :--- | :--- |
| **Button** | `Arm Alarm` / `Disarm Alarm` | Arms or disarms the alarm. |
| **Button** | `Stop Ringing Alarm` | Silences an active alarm beeper. |
| **Button** | `Start / Stop Countdown Timer` | Starts or stops the countdown timer. |
| **Button** | `Trigger Face Animation` | Runs face animation sequence on demand. |
| **Button** | `Cathode Cleaning (Slot Machine)` | Runs cathode rejuvenation effect on demand. |
| **Button** | `Play Audio Recording` | Pulses `GPIO 5` to play recorded voice message. |

### Status & Sensors

| Domain | Entity Name | Description |
| :--- | :--- | :--- |
| **Sensor** | `Timer Remaining Seconds` | Numeric countdown timer seconds. |
| **Text Sensor** | `Timer Remaining` | Live formatted `"HH:MM:SS"` countdown string. |
| **Text Sensor** | `Active Display Characters` | Live 6-character string illuminated on the tubes. |
| **Binary Sensor** | `Alarm Ringing` | `ON` when the alarm is actively beeping. |
| **Binary Sensor** | `Timer Running` | `ON` when countdown timer is active. |
| **Binary Sensor** | `Timer Ringing` | `ON` when timer has reached zero and is beeping. |

---

## Home Assistant Actions / Services

You can trigger custom actions on the clock from Home Assistant scripts and automations:

### 1. Display Custom Text
Displays arbitrary text (letters, numbers, symbols) on the 6 nixie tube panels:
```yaml
action: esphome.hack_pack_nixie_clock_display_text
data:
  message: "HELLO "
  duration_seconds: 10
```

### 2. Trigger Face Animation
Runs the character face sequence (`O __O`, `O<>O`, `-<>-`, `^<>^`):
```yaml
action: esphome.hack_pack_nixie_clock_trigger_face_animation
```

### 3. Cathode Cleaning / Slot Machine
Cycles all digits rapidly across all 6 panels to prevent cathode poisoning:
```yaml
action: esphome.hack_pack_nixie_clock_trigger_slot_machine
data:
  duration_ms: 3000
```

### 4. Play Audio Recording
Pulses the voice module playback pin:
```yaml
action: esphome.hack_pack_nixie_clock_play_sound
```

---

## Complete Example YAML Configuration

See [`examples/nixie_clock_complete.yaml`](examples/nixie_clock_complete.yaml) for a complete, ready-to-flash configuration file.

---

## License

MIT License - feel free to use and adapt for your own smart home projects!
