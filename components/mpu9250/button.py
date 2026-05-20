import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

from . import MPU9250Component, mpu9250_ns

MPU9250CalibrateButton = mpu9250_ns.class_(
    "MPU9250CalibrateButton", button.Button
)

CONF_CALIBRATE = "calibrate"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MPU9250Component),
        cv.Required(CONF_CALIBRATE): button.button_schema(
            MPU9250CalibrateButton,
            icon="mdi:compass-outline",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])
    if CONF_CALIBRATE in config:
        b = await button.new_button(config[CONF_CALIBRATE])
        cg.add(parent.set_calibrate_button(b))
