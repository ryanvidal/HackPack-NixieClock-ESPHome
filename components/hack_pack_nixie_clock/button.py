import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import (
    CONF_ID,
    CONF_ICON,
)
from . import hack_pack_nixie_clock_ns, HackPackNixieClock, CONF_HACK_PACK_NIXIE_CLOCK_ID

HackPackNixieButton = hack_pack_nixie_clock_ns.class_(
    "HackPackNixieButton", button.Button, cg.Component
)
ButtonType = hack_pack_nixie_clock_ns.enum("ButtonType", is_class=True)

CONF_RESET_TIMER = "reset_timer"
CONF_STOP_ALARM = "stop_alarm"
CONF_TRIGGER_FACE = "trigger_face"
CONF_TRIGGER_SLOT_MACHINE = "trigger_slot_machine"
CONF_PLAY_SOUND = "play_sound"

BUTTONS = {
    CONF_RESET_TIMER: (ButtonType.RESET_TIMER, "Reset Countdown Timer", "mdi:timer-refresh-outline"),
    CONF_STOP_ALARM: (ButtonType.STOP_ALARM, "Stop Ringing Alarm", "mdi:alarm-snooze"),
    CONF_TRIGGER_FACE: (ButtonType.TRIGGER_FACE, "Trigger Face Animation", "mdi:emoticon-wink-outline"),
    CONF_TRIGGER_SLOT_MACHINE: (ButtonType.TRIGGER_SLOT_MACHINE, "Cathode Cleaning (Slot Machine)", "mdi:slot-machine"),
    CONF_PLAY_SOUND: (ButtonType.PLAY_SOUND, "Play Audio Recording", "mdi:volume-high"),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_HACK_PACK_NIXIE_CLOCK_ID): cv.use_id(HackPackNixieClock),
}).extend({
    cv.Optional(key): button.button_schema(
        HackPackNixieButton,
        icon=info[2],
    )
    for key, info in BUTTONS.items()
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_HACK_PACK_NIXIE_CLOCK_ID])
    for key, info in BUTTONS.items():
        if key in config:
            conf = config[key]
            var = await button.new_button(conf)
            await cg.register_component(var, conf)
            cg.add(var.set_parent(parent))
            cg.add(var.set_type(info[0]))
