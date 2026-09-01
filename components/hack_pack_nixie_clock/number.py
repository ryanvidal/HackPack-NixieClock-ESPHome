import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ID,
    CONF_ICON,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_STEP,
    CONF_MODE,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

HackPackNixieNumber = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieNumber", number.Number, cg.Component
)
NumberType = hack_pack_nixie_clock_ns.enum("NumberType", is_class=True)

CONF_TIMER_DURATION_MINUTES = "timer_duration_minutes"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_TIMER_DURATION_MINUTES): number.number_schema(
        HackPackNixieNumber,
        icon="mdi:timer-sand",
        unit_of_measurement="min",
    ).extend({
        cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
    }),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_TIMER_DURATION_MINUTES in config:
        conf = config[CONF_TIMER_DURATION_MINUTES]
        var = await number.new_number(
            conf,
            min_value=1.0,
            max_value=180.0,
            step=1.0,
        )
        await cg.register_component(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(NumberType.TIMER_DURATION_MINUTES))
        cg.add(parent.register_number(NumberType.TIMER_DURATION_MINUTES, var))
