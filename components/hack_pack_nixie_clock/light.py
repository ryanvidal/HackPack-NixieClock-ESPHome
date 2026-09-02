import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import (
    CONF_GAMMA_CORRECT,
    CONF_ID,
    CONF_OUTPUT_ID,
    CONF_TYPE,
)

from . import (
    CONF_HACK_PACK_NIXIE_CLOCK_ID,
    ColorModeEnum,
    HackPackNixieClock,
    hack_pack_nixie_clock_ns,
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

# We default gamma_correct to 0.0 (disabled in ESPHome's light component layer) because the
# Hack Pack Nixie Clock C++ rendering engine applies a unified 256-entry Perceptual Gamma 2.4
# curve (GAMMA8_TABLE) directly during WS2812 hardware transmission.
#
# If ESPHome's default gamma of 2.8 remained active:
# 1. Solid colors from the color picker would be double-gamma corrected (crushing midtones).
# 2. Built-in hardware animations (Rainbow, Flow, etc.) would bypass ESPHome's gamma entirely.
#
# Setting gamma_correct=0.0 ensures linear RGB ingestion so that our perceptual gamma curve
# scales solid colors, custom colors, and all animation modes identically and smoothly.
CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(HackPackNixieLightOutput),
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
    cv.Required(CONF_TYPE): cv.enum(LIGHT_TYPES, lower=True),
    cv.Optional(CONF_GAMMA_CORRECT, default=0.0): cv.positive_float,
})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    light_var = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_light_type(config[CONF_TYPE]))
    cg.add(parent.register_light(config[CONF_TYPE], light_var))

    if config[CONF_TYPE] == "panel":
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
