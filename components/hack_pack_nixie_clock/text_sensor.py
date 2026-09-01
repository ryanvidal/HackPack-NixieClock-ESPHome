import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    CONF_ICON,
    CONF_ENTITY_CATEGORY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

CONF_TIMER_REMAINING = "timer_remaining"
CONF_ACTIVE_DISPLAY_CHARS = "active_display_chars"

TEXT_SENSORS = {
    CONF_TIMER_REMAINING: ("Timer Remaining", "mdi:timer-sand-full", None),
    CONF_ACTIVE_DISPLAY_CHARS: ("Active Display Characters", "mdi:format-text", ENTITY_CATEGORY_DIAGNOSTIC),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_TIMER_REMAINING): text_sensor.text_sensor_schema(
        icon="mdi:timer-sand-full",
    ),
    cv.Optional(CONF_ACTIVE_DISPLAY_CHARS): text_sensor.text_sensor_schema(
        icon="mdi:format-text",
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_TIMER_REMAINING in config:
        conf = config[CONF_TIMER_REMAINING]
        var = await text_sensor.new_text_sensor(conf)
        cg.add(parent.set_timer_remaining_text_sensor(var))
    if CONF_ACTIVE_DISPLAY_CHARS in config:
        conf = config[CONF_ACTIVE_DISPLAY_CHARS]
        var = await text_sensor.new_text_sensor(conf)
        cg.add(parent.set_active_display_chars_text_sensor(var))
