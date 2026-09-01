import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import (
    CONF_ID,
    CONF_OUTPUT_ID,
    CONF_TYPE,
)
from . import (
    hack_pack_nixie_clock_ns,
    HackPackNixieClock,
    ColorModeEnum,
    CONF_HACK_PACK_NIXIE_CLOCK_ID,
)

HackPackNixieLightOutput = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieLightOutput", light.LightOutput
)
HackPackNixieEffect = hack_pack_nixie_clock_ns.class_("HackPackNixieEffect")
LightType = hack_pack_nixie_clock_ns.enum("LightType", is_class=True)

LIGHT_TYPES = {
    "panel": LightType.PANEL,
    "underglow": LightType.UNDERGLOW,
}

CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(HackPackNixieLightOutput),
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Required(CONF_TYPE): cv.enum(LIGHT_TYPES, lower=True),
})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_light_type(config[CONF_TYPE]))

    if config[CONF_TYPE] == "panel":
        light_var = await cg.get_variable(config[CONF_ID])
        effects = []
        for name, mode in [
            ("Rainbow", ColorModeEnum.COLOR_RAINBOW),
            ("Gradient", ColorModeEnum.COLOR_GRADIENT),
            ("Flow", ColorModeEnum.COLOR_FLOW),
            ("Wipe", ColorModeEnum.COLOR_WIPE),
            ("Pulse", ColorModeEnum.COLOR_PULSE),
            ("Bounce", ColorModeEnum.COLOR_BOUNCE),
        ]:
            eff = HackPackNixieEffect.new(name, mode, parent)
            effects.append(eff)
        cg.add(light_var.add_effects(effects))
