import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
)

from . import (
    CONF_HACK_PACK_NIXIE_CLOCK_ID,
    HackPackNixieClock,
    hack_pack_nixie_clock_ns,
)

HackPackNixieSwitch = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieSwitch", switch.Switch, cg.Component
)
SwitchType = hack_pack_nixie_clock_ns.enum("SwitchType", is_class=True)

CONF_FORMAT_24HR = "format_24hr"
CONF_LEADING_ZERO = "leading_zero"
CONF_COLON_BLINKING = "colon_blinking"
CONF_AM_PM_INDICATORS = "am_pm_indicators"
CONF_PERIODIC_FACES = "periodic_faces"
CONF_PHYSICAL_BUTTONS = "physical_buttons"
CONF_LINK_BRIGHTNESS = "link_brightness"
CONF_ALARM_ENABLED = "alarm_enabled"
CONF_RECORD_SOUND = "record_sound"
CONF_TIMER_RUNNING = "timer_running"

SWITCHES = {
    CONF_FORMAT_24HR: (SwitchType.FORMAT_24HR, "24-Hour Format", "mdi:clock-time-twelve-outline", ENTITY_CATEGORY_CONFIG),
    CONF_LEADING_ZERO: (SwitchType.LEADING_ZERO, "Leading Zero", "mdi:numeric-0", ENTITY_CATEGORY_CONFIG),
    CONF_COLON_BLINKING: (SwitchType.COLON_BLINKING, "Colon Blinking", "mdi:dots-vertical", ENTITY_CATEGORY_CONFIG),
    CONF_AM_PM_INDICATORS: (SwitchType.AM_PM_INDICATORS, "AM⁄PM Indicators", "mdi:theme-light-dark", ENTITY_CATEGORY_CONFIG),
    CONF_PERIODIC_FACES: (SwitchType.PERIODIC_FACES, "Periodic Face Animations", "mdi:emoticon-outline", ENTITY_CATEGORY_CONFIG),
    CONF_PHYSICAL_BUTTONS: (SwitchType.PHYSICAL_BUTTONS, "Physical Buttons Enabled", "mdi:gesture-tap-button", ENTITY_CATEGORY_CONFIG),
    CONF_LINK_BRIGHTNESS: (SwitchType.LINK_BRIGHTNESS, "Link Panel & Underglow Brightness", "mdi:link-variant", ENTITY_CATEGORY_CONFIG),
    CONF_ALARM_ENABLED: (SwitchType.ALARM_ENABLED, "Alarm Enabled", "mdi:alarm-check", None),
    CONF_RECORD_SOUND: (SwitchType.RECORD_SOUND, "Record Sound (Hold)", "mdi:microphone", None),
    CONF_TIMER_RUNNING: (SwitchType.TIMER_RUNNING, "Timer Running", "mdi:timer-play-outline", None),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
}).extend({
    cv.Optional(key): switch.switch_schema(
        HackPackNixieSwitch,
        icon=info[2],
        entity_category=info[3] if info[3] else cv.UNDEFINED,
    )
    for key, info in SWITCHES.items()
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    for key, info in SWITCHES.items():
        if key in config:
            conf = config[key]
            var = await switch.new_switch(conf)
            await cg.register_component(var, conf)
            cg.add(var.set_parent(parent))
            cg.add(var.set_type(info[0]))
            cg.add(parent.register_switch(info[0], var))
