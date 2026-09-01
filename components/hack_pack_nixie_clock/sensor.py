import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_ICON,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_ACCURACY_DECIMALS,
    CONF_ENTITY_CATEGORY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

CONF_TIMER_REMAINING_SECONDS = "timer_remaining_seconds"

SENSORS = {
    CONF_TIMER_REMAINING_SECONDS: ("Timer Remaining Seconds", "mdi:timer-outline", "s", 0, ENTITY_CATEGORY_DIAGNOSTIC),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_TIMER_REMAINING_SECONDS): sensor.sensor_schema(
        icon="mdi:timer-outline",
        unit_of_measurement="s",
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_TIMER_REMAINING_SECONDS in config:
        conf = config[CONF_TIMER_REMAINING_SECONDS]
        var = await sensor.new_sensor(conf)
        cg.add(parent.set_timer_remaining_sensor(var))
