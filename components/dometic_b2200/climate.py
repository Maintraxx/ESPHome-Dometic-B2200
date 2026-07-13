import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, remote_transmitter, sensor, switch
from esphome.const import CONF_ID, CONF_NAME

DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = ["climate", "switch"]

CONF_TRANSMITTER_ID = "transmitter_id"
CONF_TEMPERATURE_SENSOR = "temperature_sensor"
CONF_TAIL_BYTE_1 = "tail_byte_1"
CONF_TAIL_BYTE_2 = "tail_byte_2"
CONF_LIGHT = "light"
CONF_SLEEP = "sleep"
CONF_I_FEEL = "i_feel"

ns = cg.esphome_ns.namespace("dometic_b2200")
DometicB2200Climate = ns.class_("DometicB2200Climate", climate.Climate, cg.Component)
DometicB2200Switch = ns.class_("DometicB2200Switch", switch.Switch, cg.Component)
DometicSwitchType = ns.enum("DometicSwitchType")

SWITCH_TYPES = {
    CONF_LIGHT: DometicSwitchType.LIGHT,
    CONF_SLEEP: DometicSwitchType.SLEEP,
    CONF_I_FEEL: DometicSwitchType.I_FEEL,
}

OPTIONAL_SWITCH_SCHEMA = switch.switch_schema(DometicB2200Switch).extend(
    {cv.GenerateID(CONF_ID): cv.declare_id(DometicB2200Switch)}
)

CONFIG_SCHEMA = (
    climate.climate_schema(DometicB2200Climate)
    .extend(
        {
            cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(
                remote_transmitter.RemoteTransmitterComponent
            ),
            cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_TAIL_BYTE_1, default=0x3B): cv.hex_uint8_t,
            cv.Optional(CONF_TAIL_BYTE_2, default=0x53): cv.hex_uint8_t,
            cv.Optional(CONF_LIGHT): OPTIONAL_SWITCH_SCHEMA,
            cv.Optional(CONF_SLEEP): OPTIONAL_SWITCH_SCHEMA,
            cv.Optional(CONF_I_FEEL): OPTIONAL_SWITCH_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))

    if CONF_TEMPERATURE_SENSOR in config:
        temperature_sensor = await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])
        cg.add(var.set_temperature_sensor(temperature_sensor))

    cg.add(var.set_tail_byte_1(config[CONF_TAIL_BYTE_1]))
    cg.add(var.set_tail_byte_2(config[CONF_TAIL_BYTE_2]))

    for key, switch_type in SWITCH_TYPES.items():
        if key not in config:
            continue
        switch_config = config[key]
        sw = cg.new_Pvariable(switch_config[CONF_ID])
        await cg.register_component(sw, switch_config)
        await switch.register_switch(sw, switch_config)
        cg.add(sw.set_parent(var))
        cg.add(sw.set_type(switch_type))
        cg.add(var.register_switch(sw))
