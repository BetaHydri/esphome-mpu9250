import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_TEMPERATURE,
    UNIT_CELSIUS,
    UNIT_DEGREE_PER_SECOND,
    UNIT_METER_PER_SECOND_SQUARED,
    UNIT_MICROTESLA,
    UNIT_DEGREES,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

from . import MPU9250Component

CONF_ACCEL = "accel"
CONF_GYRO = "gyro"
CONF_MAG = "mag"
CONF_HEADING = "heading"
CONF_USE_MADGWICK = "use_madgwick"
CONF_DECLINATION = "declination"


def vec3(unit):
    return cv.Schema(
        {
            cv.Optional("x"): sensor.sensor_schema(unit_of_measurement=unit),
            cv.Optional("y"): sensor.sensor_schema(unit_of_measurement=unit),
            cv.Optional("z"): sensor.sensor_schema(unit_of_measurement=unit),
        }
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MPU9250Component),
        cv.Optional(CONF_ACCEL): vec3(UNIT_METER_PER_SECOND_SQUARED),
        cv.Optional(CONF_GYRO): vec3(UNIT_DEGREE_PER_SECOND),
        cv.Optional(CONF_MAG): vec3(UNIT_MICROTESLA),
        cv.Optional(CONF_HEADING): sensor.sensor_schema(
            unit_of_measurement=UNIT_DEGREES,
            icon="mdi:compass",
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_USE_MADGWICK, default=True): cv.boolean,
        cv.Optional(CONF_DECLINATION, default=0.0): cv.float_,
    }
)


async def to_code(config):
    var = await cg.get_variable(config[CONF_ID])

    if CONF_ACCEL in config:
        for a in "xyz":
            s = await sensor.new_sensor(config[CONF_ACCEL][a])
            cg.add(getattr(var, f"set_accel_{a}")(s))

    if CONF_GYRO in config:
        for a in "xyz":
            s = await sensor.new_sensor(config[CONF_GYRO][a])
            cg.add(getattr(var, f"set_gyro_{a}")(s))

    if CONF_MAG in config:
        for a in "xyz":
            s = await sensor.new_sensor(config[CONF_MAG][a])
            cg.add(getattr(var, f"set_mag_{a}")(s))

    if CONF_HEADING in config:
        s = await sensor.new_sensor(config[CONF_HEADING])
        cg.add(var.set_heading(s))

    if CONF_TEMPERATURE in config:
        s = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature(s))

    cg.add(var.set_use_madgwick(config[CONF_USE_MADGWICK]))
    cg.add(var.set_declination(config[CONF_DECLINATION]))
