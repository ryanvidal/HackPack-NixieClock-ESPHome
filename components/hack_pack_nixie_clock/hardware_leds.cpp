#include "hardware_leds.h"

#if defined(USE_ESP_IDF)
#include "esp_cpu.h"
#include "esp_attr.h"
#include "soc/gpio_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#elif defined(USE_ARDUINO)
#include <Arduino.h>
#endif

namespace esphome {
namespace hack_pack_nixie_clock {

// 256-entry Perceptual Gamma 2.4 curve table with non-zero floor for x > 0
const uint8_t GAMMA8_TABLE[256] = {
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

#if defined(USE_ESP_IDF)
void IRAM_ATTR ws2812_transmit(uint8_t pin, const uint8_t *data, size_t len) {
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
        while ((uint32_t)(esp_cpu_get_cycle_count() - start) < c_1h) {}

        REG_WRITE(GPIO_OUT_W1TC_REG, mask);
        start = esp_cpu_get_cycle_count();
        while ((uint32_t)(esp_cpu_get_cycle_count() - start) < c_1l) {}
      } else {
        REG_WRITE(GPIO_OUT_W1TS_REG, mask);
        uint32_t start = esp_cpu_get_cycle_count();
        while ((uint32_t)(esp_cpu_get_cycle_count() - start) < c_0h) {}

        REG_WRITE(GPIO_OUT_W1TC_REG, mask);
        start = esp_cpu_get_cycle_count();
        while ((uint32_t)(esp_cpu_get_cycle_count() - start) < c_0l) {}
      }
    }
  }

  // WS2812 Reset Latch: >300us LOW
  REG_WRITE(GPIO_OUT_W1TC_REG, mask);
  uint32_t reset_cycles = 300 * cpu_mhz;
  uint32_t start = esp_cpu_get_cycle_count();
  while ((uint32_t)(esp_cpu_get_cycle_count() - start) < reset_cycles) {}

  portEXIT_CRITICAL(&mux);
}
#else
void ws2812_transmit(uint8_t pin, const uint8_t *data, size_t len) {
  // Arduino fallback (stub / standard bit-bang)
}
#endif

}  // namespace hack_pack_nixie_clock
}  // namespace esphome
