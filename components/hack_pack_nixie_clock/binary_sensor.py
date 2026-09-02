import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor

from . import (
    CONF_HACK_PACK_NIXIE_CLOCK_ID,
    HackPackNixieClock,
)

CONF_ALARM_RINGING = "alarm_ringing"
CONF_TIMER_RINGING = "timer_ringing"

BINARY_SENSORS = {
    CONF_ALARM_RINGING: ("Alarm Ringing", "mdi:alarm-bell"),
    CONF_TIMER_RINGING: ("Timer Ringing", "mdi:bell-ring-outline"),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_ALARM_RINGING): binary_sensor.binary_sensor_schema(
        icon="mdi:alarm-bell",
    ),
    cv.Optional(CONF_TIMER_RINGING): binary_sensor.binary_sensor_schema(
        icon="mdi:bell-ring-outline",
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_ALARM_RINGING in config:
        conf = config[CONF_ALARM_RINGING]
        var = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_alarm_ringing_binary_sensor(var))
    if CONF_TIMER_RINGING in config:
        conf = config[CONF_TIMER_RINGING]
        var = await binary_sensor.new_binary_sensor(conf)
        cg.add(parent.set_timer_ringing_binary_sensor(var))
