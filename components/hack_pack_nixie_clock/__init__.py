import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.automation import maybe_simple_id
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)
from esphome.components import (
    time as time_,
)

# Component namespace
hack_pack_nixie_clock_ns = cg.esphome_ns.namespace('hack_pack_nixie_clock')
HackPackNixieClock = hack_pack_nixie_clock_ns.class_('HackPackNixieClock', cg.Component)

DisplayModeEnum = hack_pack_nixie_clock_ns.enum("DisplayMode")
DISPLAY_MODES = {
    "TIME": DisplayModeEnum.MODE_TIME,
    "TIMER": DisplayModeEnum.MODE_TIMER,
    "ALARM": DisplayModeEnum.MODE_ALARM,
    "CUSTOM_TEXT": DisplayModeEnum.MODE_CUSTOM_TEXT,
    "FACE": DisplayModeEnum.MODE_FACE,
    "SLOT_MACHINE": DisplayModeEnum.MODE_SLOT_MACHINE,
    "OFF": DisplayModeEnum.MODE_OFF,
}

ColorModeEnum = hack_pack_nixie_clock_ns.enum("ColorMode")
COLOR_MODES = {
    "RAINBOW": ColorModeEnum.COLOR_RAINBOW,
    "SOLID": ColorModeEnum.COLOR_SOLID,
    "GRADIENT": ColorModeEnum.COLOR_GRADIENT,
    "FLOW": ColorModeEnum.COLOR_FLOW,
    "WIPE": ColorModeEnum.COLOR_WIPE,
    "PULSE": ColorModeEnum.COLOR_PULSE,
    "BOUNCE": ColorModeEnum.COLOR_BOUNCE,
}

# Automation Actions
DisplayTextAction = hack_pack_nixie_clock_ns.class_("DisplayTextAction", automation.Action)
TriggerSlotMachineAction = hack_pack_nixie_clock_ns.class_("TriggerSlotMachineAction", automation.Action)
TriggerFaceAnimationAction = hack_pack_nixie_clock_ns.class_("TriggerFaceAnimationAction", automation.Action)
PlaySoundAction = hack_pack_nixie_clock_ns.class_("PlaySoundAction", automation.Action)
StartTimerAction = hack_pack_nixie_clock_ns.class_("StartTimerAction", automation.Action)
StopTimerAction = hack_pack_nixie_clock_ns.class_("StopTimerAction", automation.Action)
ResetTimerAction = hack_pack_nixie_clock_ns.class_("ResetTimerAction", automation.Action)
StopAlarmAction = hack_pack_nixie_clock_ns.class_("StopAlarmAction", automation.Action)
ArmAlarmAction = hack_pack_nixie_clock_ns.class_("ArmAlarmAction", automation.Action)
DisarmAlarmAction = hack_pack_nixie_clock_ns.class_("DisarmAlarmAction", automation.Action)
SetAlarmAction = hack_pack_nixie_clock_ns.class_("SetAlarmAction", automation.Action)
SetDisplayModeAction = hack_pack_nixie_clock_ns.class_("SetDisplayModeAction", automation.Action)
SetColorModeAction = hack_pack_nixie_clock_ns.class_("SetColorModeAction", automation.Action)
ShowTimeAction = hack_pack_nixie_clock_ns.class_("ShowTimeAction", automation.Action)
ShowTimerAction = hack_pack_nixie_clock_ns.class_("ShowTimerAction", automation.Action)

HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA = maybe_simple_id({
    cv.GenerateID(): cv.use_id(HackPackNixieClock),
})

# Configuration Keys
CONF_HACK_PACK_NIXIE_CLOCK_ID = "hack_pack_nixie_clock_id"
CONF_PANEL_PIN = "panel_pin"
CONF_UNDERGLOW_PIN = "underglow_pin"
CONF_PLAY_PIN = "play_pin"
CONF_REC_PIN = "rec_pin"
CONF_BTN_TOP = "btn_top_pin"
CONF_BTN_CENTER = "btn_center_pin"
CONF_BTN_UP = "btn_up_pin"
CONF_BTN_DOWN = "btn_down_pin"
CONF_BTN_LEFT = "btn_left_pin"
CONF_BTN_RIGHT = "btn_right_pin"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HackPackNixieClock),
    cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
    
    # Hardware Pins with Hack Pack Box 15 (ESP32-C3) Defaults
    cv.Optional(CONF_PANEL_PIN, default=0): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_UNDERGLOW_PIN, default=1): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_PLAY_PIN, default=5): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_REC_PIN, default=4): pins.internal_gpio_output_pin_number,
    
    cv.Optional(CONF_BTN_TOP, default=19): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_CENTER, default=10): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_UP, default=9): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_DOWN, default=8): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_LEFT, default=7): pins.internal_gpio_input_pin_number,
    cv.Optional(CONF_BTN_RIGHT, default=6): pins.internal_gpio_input_pin_number,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_panel_pin(config[CONF_PANEL_PIN]))
    cg.add(var.set_underglow_pin(config[CONF_UNDERGLOW_PIN]))
    cg.add(var.set_play_pin(config[CONF_PLAY_PIN]))
    cg.add(var.set_rec_pin(config[CONF_REC_PIN]))

    cg.add(var.set_btn_top_pin(config[CONF_BTN_TOP]))
    cg.add(var.set_btn_center_pin(config[CONF_BTN_CENTER]))
    cg.add(var.set_btn_up_pin(config[CONF_BTN_UP]))
    cg.add(var.set_btn_down_pin(config[CONF_BTN_DOWN]))
    cg.add(var.set_btn_left_pin(config[CONF_BTN_LEFT]))
    cg.add(var.set_btn_right_pin(config[CONF_BTN_RIGHT]))

    if CONF_TIME_ID in config:
        time_component = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_source(time_component))


@automation.register_action(
    "hack_pack_nixie_clock.display_text",
    DisplayTextAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Required("message"): cv.templatable(cv.string),
        cv.Optional("scroll_speed_ms", default=350): cv.templatable(cv.positive_int),
    }),
    synchronous=True,
)
async def display_text_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["message"], args, cg.std_string)
    cg.add(var.set_message(template_))
    template_speed = await cg.templatable(config["scroll_speed_ms"], args, cg.uint32)
    cg.add(var.set_scroll_speed_ms(template_speed))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.trigger_slot_machine",
    TriggerSlotMachineAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Optional("duration_ms", default=2500): cv.templatable(cv.positive_int),
    }),
    synchronous=True,
)
async def trigger_slot_machine_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["duration_ms"], args, cg.uint32)
    cg.add(var.set_duration_ms(template_))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.trigger_face_animation",
    TriggerFaceAnimationAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def trigger_face_animation_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.play_sound",
    PlaySoundAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Optional("duration_ms", default=150): cv.templatable(cv.positive_int),
    }),
    synchronous=True,
)
async def play_sound_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["duration_ms"], args, cg.uint32)
    cg.add(var.set_duration_ms(template_))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.start_timer",
    StartTimerAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Optional("duration"): cv.templatable(cv.string),
    }),
    synchronous=True,
)
async def start_timer_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    if "duration" in config:
        template_ = await cg.templatable(config["duration"], args, cg.std_string)
        cg.add(var.set_duration(template_))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.stop_timer",
    StopTimerAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def stop_timer_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.reset_timer",
    ResetTimerAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def reset_timer_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.stop_alarm",
    StopAlarmAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def stop_alarm_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.arm_alarm",
    ArmAlarmAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def arm_alarm_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.disarm_alarm",
    DisarmAlarmAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def disarm_alarm_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.set_alarm",
    SetAlarmAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Required("hour"): cv.templatable(cv.int_range(min=0, max=23)),
        cv.Required("minute"): cv.templatable(cv.int_range(min=0, max=59)),
    }),
    synchronous=True,
)
async def set_alarm_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_h = await cg.templatable(config["hour"], args, cg.uint8)
    cg.add(var.set_hour(template_h))
    template_m = await cg.templatable(config["minute"], args, cg.uint8)
    cg.add(var.set_minute(template_m))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.set_display_mode",
    SetDisplayModeAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Required("mode"): cv.templatable(cv.enum(DISPLAY_MODES, upper=True)),
    }),
    synchronous=True,
)
async def set_display_mode_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["mode"], args, DisplayModeEnum)
    cg.add(var.set_mode(template_))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.set_color_mode",
    SetColorModeAction,
    cv.Schema({
        cv.GenerateID(): cv.use_id(HackPackNixieClock),
        cv.Required("mode"): cv.templatable(cv.enum(COLOR_MODES, upper=True)),
    }),
    synchronous=True,
)
async def set_color_mode_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config["mode"], args, ColorModeEnum)
    cg.add(var.set_mode(template_))
    return var


@automation.register_action(
    "hack_pack_nixie_clock.show_time",
    ShowTimeAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def show_time_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "hack_pack_nixie_clock.show_timer",
    ShowTimerAction,
    HACK_PACK_NIXIE_CLOCK_ACTION_SCHEMA,
    synchronous=True,
)
async def show_timer_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
