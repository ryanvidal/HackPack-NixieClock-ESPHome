import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import datetime
from esphome.const import (
    CONF_ID,
    CONF_ICON,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

HackPackNixieTime = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieTime", datetime.TimeEntity, cg.Component
)
TimeType = hack_pack_nixie_clock_ns.enum("TimeType", is_class=True)

CONF_ALARM_TIME = "alarm_time"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_ALARM_TIME): datetime.time_schema(
        HackPackNixieTime,
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_ALARM_TIME in config:
        conf = config[CONF_ALARM_TIME]
        var = await datetime.new_datetime(conf)
        await cg.register_component(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(TimeType.ALARM_TIME))
        cg.add(parent.register_time(TimeType.ALARM_TIME, var))
