import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = []
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

mpu9250_ns = cg.esphome_ns.namespace("mpu9250")
MPU9250Component = mpu9250_ns.class_(
    "MPU9250Component", cg.PollingComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MPU9250Component),
        }
    )
    .extend(i2c.i2c_device_schema(0x68))
    .extend(cv.polling_component_schema("100ms"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
