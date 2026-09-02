import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import (
    CONF_HACK_PACK_NIXIE_CLOCK_ID,
    HackPackNixieClock,
)

CONF_TIMER_REMAINING = "timer_remaining"
CONF_VERSION = "version"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_TIMER_REMAINING): text_sensor.text_sensor_schema(
        icon="mdi:timer-sand-full",
    ),
    cv.Optional(CONF_VERSION): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:tag-outline",
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_TIMER_REMAINING in config:
        conf = config[CONF_TIMER_REMAINING]
        var = await text_sensor.new_text_sensor(conf)
        cg.add(parent.set_timer_remaining_text_sensor(var))
    if CONF_VERSION in config:
        conf = config[CONF_VERSION]
        var = await text_sensor.new_text_sensor(conf)
        cg.add(parent.set_version_text_sensor(var))
