import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)
from esphome.components import (
    time as time_,
)

# Component namespace
hack_pack_nixie_clock_ns = cg.esphome_ns.namespace('hack_pack_nixie_clock')
HackPackNixieClock = hack_pack_nixie_clock_ns.class_('HackPackNixieClock', cg.Component)

# Configuration Keys
CONF_PANEL_PIN = "panel_pin"
CONF_UNDERGLOW_PIN = "underglow_pin"
CONF_PLAY_PIN = "play_pin"
CONF_REC_PIN = "rec_pin"
CONF_BTN_TOP = "btn_top_pin"
CONF_BTN_CENTER = "btn_center_pin"
CONF_BTN_UP = "btn_up_pin"
CONF_BTN_DOWN = "btn_down_pin"
CONF_BTN_LEFT = "btn_left_pin"
CONF_BTN_RIGHT = "btn_right_pin"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HackPackNixieClock),
    cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
    
    # Hardware Pins with Hack Pack Box 15 (ESP32-C3) Defaults
    cv.Optional(CONF_PANEL_PIN, default=0): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_UNDERGLOW_PIN, default=1): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_PLAY_PIN, default=5): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_REC_PIN, default=4): pins.internal_gpio_output_pin_number,
    
    cv.Optional(CONF_BTN_TOP, default=19): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_CENTER, default=10): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_UP, default=9): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_DOWN, default=8): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_LEFT, default=7): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_RIGHT, default=6): pins.internal_gpio_input_pin_number,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_panel_pin(config[CONF_PANEL_PIN]))
    cg.add(var.set_underglow_pin(config[CONF_UNDERGLOW_PIN]))
    cg.add(var.set_play_pin(config[CONF_PLAY_PIN]))
    cg.add(var.set_rec_pin(config[CONF_REC_PIN]))

    cg.add(var.set_btn_top_pin(config[CONF_BTN_TOP]))
    cg.add(var.set_btn_center_pin(config[CONF_BTN_CENTER]))
    cg.add(var.set_btn_up_pin(config[CONF_BTN_UP]))
    cg.add(var.set_btn_down_pin(config[CONF_BTN_DOWN]))
    cg.add(var.set_btn_left_pin(config[CONF_BTN_LEFT]))
    cg.add(var.set_btn_right_pin(config[CONF_BTN_RIGHT]))

    if CONF_TIME_ID in config:
        time_component = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_source(time_component))
