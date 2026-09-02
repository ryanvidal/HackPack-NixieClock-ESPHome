# Hack Pack Nixie Clock (Box 15) ESPHome Custom Component

[![ESPHome Version](https://img.shields.io/badge/ESPHome-2024%2B-blue.svg)](https://esphome.io)
[![Platform](https://img.shields.io/badge/Platform-ESP32--C3-orange.svg)](https://espressif.com)
[![Framework](https://img.shields.io/badge/Framework-ESP--IDF-green.svg)](https://esphome.io/components/esp32.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An ESPHome custom component for the **CrunchLabs Hack Pack Box 15 (Smart Nixie Clock)** based on the **ESP32-C3 Mini**. 

This integration provides a feature-complete **Home Assistant** integration while preserving and enhancing all animations, underglow effects, 7-segment character glyphs, timers, alarms, audio module controls, and physical button navigation.

---

## Features

- 💾 **Non-Volatile Flash Persistence**: Restores all state, colors, brightness, format modes, alarms, button lock, and timer duration automatically across power cycles using ESPHome NVS Preferences.
- 🕒 **Native Time & Date Synchronization**: Automatically keeps accurate time synced from Home Assistant / NTP with 12-hour and 24-hour modes.
- 🎨 **Native RGB Color Pickers**: Full Home Assistant color wheel support for both the Nixie tube panel and underglow LEDs via native `light` entities with brightness sliders and built-in animation effects.
- 🌈 **7 Color Animation Modes**: `Rainbow`, `Solid`, `Gradient`, `Flow`, `Wipe`, `Pulse`, and `Bounce`.
- 🔗 **Optical 80% Brightness Linking**: Synchronizes tube panel and underglow brightness levels using a calibrated 80% ratio to match lumens behind the acrylic darkening film.
- 💡 **Dynamic Underglow & Colons**: Underglow color control, AM/PM indicators, colon blinking, and automatic inter-panel color blending. Colons automatically turn completely off during message displays.
- ⏰ **Smart Alarm & Countdown Timers**: Native Home Assistant time-picker widget for alarm and idiomatic `HH:MM:SS` duration picker for countdown timers with live formatted `"HH:MM:SS"` sensors and audio beeper notifications.
- 🎙️ **Voice Speaker Media Player**: Exposes the onboard sound recorder module with record/playback triggers.
- 🕹️ **Local Physical Buttons**: Full button debouncing and hardware navigation preserved, with the ability to lock/disable buttons from Home Assistant.
- 🚀 **Flicker-Free IRAM Driver**: Custom cycle-accurate WS2812 bit-banging engine running in internal RAM (`IRAM_ATTR`) with Perceptual Gamma 2.4 curve table, eliminating FreeRTOS/WiFi single-core preemption and digit flickering.
- 💬 **Multi-Phase Alert & Scrolling Text Service**: Attention-grabbing message board sequence: 1s blank $\rightarrow$ 2s flashing `"- - - "` alert strobe $\rightarrow$ smooth right-to-left scrolling $\rightarrow$ 1s trailing blank $\rightarrow$ return to clock.

---

## Repository Structure

```text
hack-pack-nixie-clock/
├── components/
│   └── hack_pack_nixie_clock/
│       ├── __init__.py          # Component schema, actions & setup
│       ├── types.h              # Enums, structs & NVS storage schema
│       ├── font_map.h           # 7-segment character lookup tables
│       ├── hardware_leds.h/.cpp # WS2812 IRAM driver & Gamma 2.4 table
│       ├── light_output.h/.cpp  # RGB LightOutput, LightEffects & Listener
│       ├── entities.h/.cpp      # Switch, Button, Select, Number, Text, Time entities
│       ├── automation.h         # ESPHome action template classes
│       ├── hack_pack_nixie_clock.h
│       └── hack_pack_nixie_clock.cpp
├── packages/
│   └── nixie_clock.yaml         # Ready-to-use drop-in package
├── examples/
│   ├── nixie_clock_basic.yaml
│   └── nixie_clock_advanced.yaml
└── README.md
```

---

## Quickstart

Add this repository to your ESPHome configuration using `external_components`:

```yaml
external_components:
  - source: github://ryanvidal/HackPack-NixieClockESPHome
    components: [ hack_pack_nixie_clock ]

time:
  - platform: homeassistant
    id: homeassistant_time

# All pins automatically default to the stock Hack Pack Box 15 hardware!
hack_pack_nixie_clock:
  id: clock_hub
  time_id: homeassistant_time

# Native Color Pickers & Media Player
light:
  - platform: hack_pack_nixie_clock
    type: panel
    name: "Nixie Tube Lighting"
    id: light_panel
  - platform: hack_pack_nixie_clock
    type: underglow
    name: "Underglow Lighting"
    id: light_underglow

media_player:
  - platform: hack_pack_nixie_clock
    name: "Voice Speaker"
    id: nixie_speaker
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

## Home Assistant Entities & Categories

Entities are categorized cleanly into standard Home Assistant sections (**Controls**, **Configuration**, and **Diagnostic**):

### 🎮 Primary Controls

| Domain | Entity Name | Description |
| :--- | :--- | :--- |
| **Light** | `Nixie Tube Lighting` | Full RGB color wheel, brightness slider, and animation effects dropdown. |
| **Light** | `Underglow Lighting` | Full RGB color wheel and brightness slider for underglow. |
| **Media Player** | `Voice Speaker` | Native media player entity with Play/Stop actions for sound module. |
| **DateTime** | `Alarm Time` | Native time picker widget to set alarm time (`HH:MM:SS`). |
| **DateTime** | `Timer Duration` | Idiomatic duration picker widget to set countdown duration (`HH:MM:SS`). |
| **Select** | `Display Mode` | Selects: `Time`, `Timer`, `Alarm View`, `Slot Machine`, `Faces`, `Custom Text`, `Off`. |
| **Select** | `Color Animation Mode` | Selects: `Rainbow`, `Solid`, `Gradient`, `Flow`, `Wipe`, `Pulse`, `Bounce`. |
| **Select** | `Colon Color Mode` | Selects: `Auto Blend`, `Match Underglow`, `Fixed`. |
| **Switch** | `Alarm Enabled` | Arms or disarms the alarm. |
| **Button** | `Start Countdown Timer` | Starts countdown timer using configured duration. |
| **Button** | `Stop Countdown Timer` | Cancels active countdown timer. |
| **Button** | `Arm Alarm` / `Disarm Alarm` | Arms or disarms the alarm. |
| **Button** | `Stop Ringing Alarm` | Silences an active alarm beeper. |
| **Button** | `Trigger Face Animation` | Runs face animation sequence on demand. |
| **Button** | `Cathode Cleaning (Slot Machine)` | Runs cathode rejuvenation effect on demand. |
| **Button** | `Play Audio Recording` | Pulses `GPIO 5` to play recorded voice message. |

### ⚙️ Configuration (`entity_category: config`)

| Domain | Entity Name | Description |
| :--- | :--- | :--- |
| **Switch** | `Link Panel & Underglow Brightness` | Synchronizes panel and underglow brightness levels. |
| **Switch** | `24-Hour Format` | Toggles 12-hour vs 24-hour time format. |
| **Switch** | `Leading Zero` | Toggles leading zero on single-digit hours (`" 9:00"` vs `"09:00"`). |
| **Switch** | `Colon Blinking` | Enables/disables 1Hz blinking colons. |
| **Switch** | `AM⁄PM Indicators` | Enables/disables amber AM and purple PM LEDs. |
| **Switch** | `Periodic Face Animations` | Enables/disables automatic periodic face animations. |
| **Switch** | `Physical Buttons Enabled` | Enables or locks local button inputs. |
| **Switch** | `Record Sound (Hold)` | Holds `GPIO 4` high to record audio message. |

### 📊 Status & Diagnostic (`entity_category: diagnostic`)

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

### 1. Display Alert & Scrolling Custom Text
Displays a 1s blank screen $\rightarrow$ 2s flashing `"!!!!!!"` alert strobe $\rightarrow$ smooth text scroll $\rightarrow$ 1s blank screen $\rightarrow$ returns to previous mode:
```yaml
action: esphome.hack_pack_nixie_clock_display_text
data:
  message: "HELLO CRUNCHLABS!"
  scroll_speed_ms: 300
```

### 2. Start Countdown Timer
Starts or resumes the countdown timer with an optional natural language duration string:
```yaml
action: esphome.hack_pack_nixie_clock_start_timer
data:
  duration: "10m"
```

### 3. Show Time (Clock View)
Switches the Nixie tube display mode to standard real-time clock view:
```yaml
action: esphome.hack_pack_nixie_clock_show_time
```

### 4. Show Timer (Timer View)
Switches the Nixie tube display mode to the countdown timer view:
```yaml
action: esphome.hack_pack_nixie_clock_show_timer
```

### 5. Trigger Face Animation
Runs the character face sequence (`O __O`, `O<>O`, `-<>-`, `^<>^`):
```yaml
action: esphome.hack_pack_nixie_clock_trigger_face_animation
```

### 6. Cathode Cleaning / Slot Machine
Cycles all digits rapidly across all 6 panels to prevent cathode poisoning:
```yaml
action: esphome.hack_pack_nixie_clock_trigger_slot_machine
data:
  duration_ms: 3000
```

### 7. Play Audio Recording
Pulses the voice module playback pin:
```yaml
action: esphome.hack_pack_nixie_clock_play_sound
```

---

## Recommended Home Assistant Dashboard Card Layout

Below is a clean Lovelace dashboard card layout organizing all controls into logical sections:

```yaml
type: vertical-stack
cards:
  # --------------------------------------------------------------------------
  # Section: Lighting & Color Pickers
  # --------------------------------------------------------------------------
  - type: entities
    title: 🎨 Nixie Lighting & Colors
    show_header_toggle: false
    entities:
      - entity: light.hack_pack_nixie_clock_nixie_tube_lighting
        name: Nixie Tubes
      - entity: light.hack_pack_nixie_clock_underglow_lighting
        name: Underglow
      - entity: select.hack_pack_nixie_clock_color_animation_mode
        name: Animation Effect
      - entity: select.hack_pack_nixie_clock_colon_color_mode
        name: Colon Mode
      - entity: switch.hack_pack_nixie_clock_link_panel_underglow_brightness
        name: Link Brightness

  # --------------------------------------------------------------------------
  # Section: Alarm & Countdown Timer
  # --------------------------------------------------------------------------
  - type: entities
    title: ⏰ Alarm & Timer
    show_header_toggle: false
    entities:
      - entity: datetime.hack_pack_nixie_clock_alarm_time
        name: Alarm Time
      - entity: switch.hack_pack_nixie_clock_alarm_enabled
        name: Alarm Armed
      - entity: text.hack_pack_nixie_clock_timer_duration
        name: Timer Duration
      - entity: switch.hack_pack_nixie_clock_timer_running
        name: Timer Running
      - entity: sensor.hack_pack_nixie_clock_timer_remaining
        name: Timer Countdown
      - entity: button.hack_pack_nixie_clock_reset_countdown_timer
        name: Reset Timer

  # --------------------------------------------------------------------------
  # Section: Voice Module & Fun Effects
  # --------------------------------------------------------------------------
  - type: media-control
    entity: media_player.hack_pack_nixie_clock_voice_speaker

  - type: horizontal-stack
    cards:
      - type: button
        name: Faces
        icon: mdi:emoticon-wink-outline
        tap_action:
          action: call-service
          service: button.press
          target:
            entity_id: button.hack_pack_nixie_clock_trigger_face_animation
      - type: button
        name: Cathode Clean
        icon: mdi:slot-machine
        tap_action:
          action: call-service
          service: button.press
          target:
            entity_id: button.hack_pack_nixie_clock_cathode_cleaning_slot_machine
```

---

## ⚡ Power Management & OTA Update Brownout Protection

### The Brownout Phenomenon During OTA
The ESP32-C3 Mini MCU powers all **55 WS2812 LEDs** (42 tube panel segments + 13 underglow LEDs), Wi-Fi transceiver, and internal SPI flash memory from the USB 5V/3.3V power rails.

During an **Over-The-Air (OTA) firmware update**, high-throughput Wi-Fi reception combined with rapid SPI flash memory write/erase cycles creates temporary current surges. If the LEDs remain illuminated during flashing:
1. Voltage can briefly sag below the default ESP32-C3 hardware brownout detection threshold (~2.8V).
2. The hardware brownout detector immediately triggers a hard reset/reboot.
3. The OTA update fails midway or connection is lost.

### Built-in Firmware Mitigations
This integration includes multi-layered protections configured out of the box in [`packages/nixie_clock.yaml`](packages/nixie_clock.yaml):

1. **Automatic Pre-Flight Blackout Hook (`set_ready_for_ota()`)**:
   When an OTA update begins, the component's `ota: on_begin` trigger automatically zeroes out all 55 LEDs and disables the display, reducing current draw to near zero before writing flash sectors.
2. **Wi-Fi Output Power Reduction (13 dBm)**:
   In `set_ready_for_ota()`, the Wi-Fi transceiver transmission power is throttled down to **13 dBm** via ESP-IDF `esp_wifi_set_max_tx_power(52)`, significantly cutting Wi-Fi power consumption during the payload receipt and SPI flash write phases.
3. **Post-Flash Dark Mode (`on_end`)**:
   Maintains blackout state and low Wi-Fi power until the MCU successfully finishes rebooting into the new firmware.
4. **Calibrated Brownout Level Configuration**:
   In `esp32.framework.sdkconfig_options`, `CONFIG_ESP_BROWNOUT_DET_LVL_SEL_0: y` lowers the brownout trigger threshold to **2.41V**, giving maximum headroom against transient voltage dips.

### Best Practices for Reliable Updates
- **Power Supply**: Power the clock with a dedicated **5V 2A (10W+)** USB-C power supply. Avoid unpowered USB hubs or long, thin USB cables with high internal resistance.
- **Manual Blackout (Optional)**: If you are building a custom YAML without the package import, ensure `set_ready_for_ota()` is invoked in your `ota: on_begin` hook or turn off the lights before flashing.

---

## License

MIT License - feel free to use and adapt for your own smart home projects!
