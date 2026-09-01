import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    CONF_ID,
    CONF_ICON,
    CONF_OPTIONS,
    CONF_ENTITY_CATEGORY,
    ENTITY_CATEGORY_CONFIG,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

HackPackNixieSelect = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieSelect", select.Select, cg.Component
)
SelectType = hack_pack_nixie_clock_ns.enum("SelectType", is_class=True)

CONF_COLON_COLOR_MODE = "colon_color_mode"

COLON_COLOR_MODE_OPTIONS = [
    "Auto Blend",
    "Match Underglow",
    "Fixed",
]

SELECTS = {
    CONF_COLON_COLOR_MODE: (SelectType.COLON_COLOR_MODE, "Colon Color Mode", "mdi:circle-half-full", COLON_COLOR_MODE_OPTIONS, ENTITY_CATEGORY_CONFIG),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
}).extend({
    cv.Optional(key): select.select_schema(
        HackPackNixieSelect,
        icon=info[2],
        entity_category=info[4] if info[4] else cv.UNDEFINED,
    )
    for key, info in SELECTS.items()
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    for key, info in SELECTS.items():
        if key in config:
            conf = config[key]
            var = await select.new_select(conf, options=info[3])
            await cg.register_component(var, conf)
            cg.add(var.set_parent(parent))
            cg.add(var.set_type(info[0]))
            cg.add(parent.register_select(info[0], var))
