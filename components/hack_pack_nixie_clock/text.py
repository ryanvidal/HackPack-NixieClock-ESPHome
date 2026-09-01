import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text
from esphome.const import (
    CONF_ID,
    CONF_ICON,
    CONF_MODE,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

HackPackNixieText = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieText", text.Text, cg.Component
)
TextType = hack_pack_nixie_clock_ns.enum("TextType", is_class=True)

CONF_TIMER_DURATION = "timer_duration"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Optional(CONF_TIMER_DURATION): text.text_schema(
        HackPackNixieText,
        icon="mdi:timer-edit-outline",
        mode="TEXT",
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    if CONF_TIMER_DURATION in config:
        conf = config[CONF_TIMER_DURATION]
        var = await text.new_text(conf)
        await cg.register_component(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_type(TextType.TIMER_DURATION))
        cg.add(parent.register_text(TextType.TIMER_DURATION, var))
