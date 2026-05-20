# ESPHome MPU9250 External Component

A full 9-axis IMU (Inertial Measurement Unit) external component for [ESPHome](https://esphome.io/), supporting the **InvenSense MPU9250** (MPU6500 accelerometer/gyroscope + AK8963 magnetometer).

Provides accelerometer, gyroscope, magnetometer, and tilt-compensated compass heading via Madgwick sensor fusion — all directly in Home Assistant.

## Features

- **Accelerometer** (X/Y/Z) — m/s²
- **Gyroscope** (X/Y/Z) — °/s
- **Magnetometer / AK8963** (X/Y/Z) — µT, accessed via I2C bypass mode
- **Compass Heading** — tilt-compensated yaw via Madgwick AHRS filter
- **Magnetic Declination** — configurable offset for your geographic location
- **ESPHome / Home Assistant** compatible
- Works on **ESP8266** (D1 Mini) and **ESP32**

## Installation

Add this to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/BetaHydri/esphome-mpu9250
      ref: main
    components: [mpu9250]
```

## Wiring

| MPU9250 Pin | ESP8266 (D1 Mini) | ESP32 |
|---|---|---|
| VCC | 3.3V | 3.3V |
| GND | GND | GND |
| SDA | D2 (GPIO4) | GPIO21 |
| SCL | D1 (GPIO5) | GPIO22 |
| AD0 | GND (address 0x68) | GND (address 0x68) |

> Pull AD0 high for address `0x69` and set `address: 0x69` in the YAML.

## Configuration

### Minimal

```yaml
i2c:
  sda: D2
  scl: D1

sensor:
  - platform: mpu9250
    accel:
      x:
        name: "Accel X"
      y:
        name: "Accel Y"
      z:
        name: "Accel Z"
```

### Full Example

```yaml
i2c:
  sda: D2
  scl: D1
  scan: false

sensor:
  - platform: mpu9250
    address: 0x68
    update_interval: 200ms
    accel:
      x:
        name: "MPU9250 Accel X"
        id: accel_x
      y:
        name: "MPU9250 Accel Y"
        id: accel_y
      z:
        name: "MPU9250 Accel Z"
        id: accel_z
    gyro:
      x:
        name: "MPU9250 Gyro X"
      y:
        name: "MPU9250 Gyro Y"
      z:
        name: "MPU9250 Gyro Z"
    mag:
      x:
        name: "MPU9250 Mag X"
      y:
        name: "MPU9250 Mag Y"
      z:
        name: "MPU9250 Mag Z"
    heading:
      name: "Compass Heading"
    declination: 3.6
    use_madgwick: true

  # Computed: total acceleration magnitude
  - platform: template
    name: "Total Acceleration"
    unit_of_measurement: "m/s²"
    accuracy_decimals: 3
    update_interval: 200ms
    lambda: |-
      if (isnan(id(accel_x).state) || isnan(id(accel_y).state) || isnan(id(accel_z).state)) {
        return NAN;
      }
      return sqrtf(pow(id(accel_x).state, 2) +
                   pow(id(accel_y).state, 2) +
                   pow(id(accel_z).state, 2));
```

### Configuration Variables

| Variable | Type | Default | Description |
|---|---|---|---|
| `address` | int | `0x68` | I2C address of the MPU9250 |
| `update_interval` | time | `100ms` | How often to read the sensor |
| `accel` | schema | — | Accelerometer axes (x, y, z). Values in m/s² |
| `gyro` | schema | — | Gyroscope axes (x, y, z). Values in °/s |
| `mag` | schema | — | Magnetometer axes (x, y, z). Values in µT |
| `heading` | sensor | — | Tilt-compensated compass heading (0–360°) |
| `use_madgwick` | bool | `true` | Enable Madgwick AHRS filter for heading |
| `declination` | float | `0.0` | Magnetic declination in degrees for your location |

All axis groups (`accel`, `gyro`, `mag`) and `heading` are optional — include only what you need.

### Magnetic Declination

The `declination` value compensates for the difference between magnetic north and true north at your location. Find your value at [magnetic-declination.com](http://www.magnetic-declination.com/).

Examples:
- Berlin, Germany: ~3.6°
- London, UK: ~-0.5°
- New York, USA: ~-13.0°

## How It Works

### Sensor Architecture

The MPU9250 is a multi-chip module containing two dies:

1. **MPU6500** — 3-axis accelerometer + 3-axis gyroscope (address `0x68`/`0x69`)
2. **AK8963** — 3-axis magnetometer (address `0x0C`, accessed via I2C bypass)

This component enables I2C bypass mode on the MPU6500 (register `0x37`), which connects the AK8963 directly to the external I2C bus. This allows ESPHome to read the magnetometer without going through the MPU6500's internal I2C master.

### Madgwick Filter

The [Madgwick AHRS algorithm](https://x-io.co.uk/open-source-imu-and-ahrs-algorithms/) fuses accelerometer, gyroscope, and magnetometer data into a stable quaternion orientation. The heading (yaw) is extracted from this quaternion, providing a tilt-compensated compass bearing that is significantly more stable than a raw `atan2(mag_y, mag_x)` calculation.

### Sensor Defaults

| Sensor | Range | Resolution |
|---|---|---|
| Accelerometer | ±2g | 16384 LSB/g |
| Gyroscope | ±250 °/s | 131 LSB/°/s |
| Magnetometer | ±4800 µT | 0.15 µT/LSB (16-bit mode) |

## Known Limitations

### Magnetometer Calibration (Not Yet Implemented)

The magnetometer readings are affected by hard-iron distortion from nearby ferromagnetic materials (vehicle chassis, metal enclosures, etc.). Without calibration, the compass heading may have a constant offset or non-uniform error.

**What's needed to add calibration:**

A `button` platform entity that triggers a 30-second hard-iron calibration routine:

1. **`button.py`** — A new platform file in `components/mpu9250/` that registers a `button.mpu9250` platform. ESPHome requires multi-platform components to have separate `<platform>.py` files for each platform they provide. The button schema should reference the parent `MPU9250Component` via `cv.use_id()`.

2. **`__init__.py` changes** — Must export the shared `mpu9250_ns` namespace and `MPU9250Component` class so both `sensor.py` and `button.py` can import them. Also needs a `CONFIG_SCHEMA` with `cv.Schema({})` and `async def to_code()` to register as a proper hub component.

3. **C++ changes** — Re-add `set_calibrate_button()`, `start_calibration()`, and the calibration state fields (`mag_min_`, `mag_max_`, `mag_offset_`, `calibrating_`, `calib_start_`) to `mpu9250.h` and `mpu9250.cpp`. The calibration routine collects min/max magnetometer values over 30 seconds, then computes the midpoint offset `(max + min) / 2` for each axis.

4. **Persistent offsets** — Ideally, calibration offsets should be saved to flash (via ESPHome `globals` or `Preferences`) so they survive reboots.

Contributions welcome — see [Contributing](#contributing).

### ESP8266 Memory

The Madgwick filter adds ~2 KB of RAM usage. On ESP8266 boards with tight memory, you may need to disable it (`use_madgwick: false`) and use a simpler heading calculation via a template sensor:

```yaml
heading:  # omit this
use_madgwick: false

# Use template sensor instead
- platform: template
  name: "Simple Heading"
  unit_of_measurement: "°"
  lambda: |-
    if (isnan(id(mag_x).state) || isnan(id(mag_y).state)) return NAN;
    float h = atan2(id(mag_y).state, id(mag_x).state) * 180.0 / 3.14159265;
    if (h < 0) h += 360;
    return h;
```

### MPU9250 Discontinuation

The MPU9250 has been discontinued by InvenSense/TDK. Many modules sold today are clones or counterfeits. Genuine modules should return device ID `0x71` (WHO_AM_I register `0x75`). If you get `0x70` or other values, the magnetometer may not work.

## Compatibility

| Board | Tested | Notes |
|---|---|---|
| ESP8266 D1 Mini | Pending | Primary target, I2C on D1/D2 |
| ESP32 DevKit | Pending | I2C on GPIO21/22 |

Tested with ESPHome **2026.4.5**.

## File Structure

```
esphome-mpu9250/
├── components/
│   └── mpu9250/
│       ├── __init__.py      # Component registration
│       ├── sensor.py        # ESPHome sensor platform (YAML schema + codegen)
│       ├── mpu9250.h        # C++ component header
│       ├── mpu9250.cpp      # C++ I2C driver (MPU6500 + AK8963)
│       └── madgwick.h       # Madgwick AHRS filter (header-only)
├── example.yaml             # Full example configuration
├── LICENSE
└── README.md
```

## Contributing

Contributions are welcome. Priority areas:

1. **Magnetometer calibration button** — see [Known Limitations](#magnetometer-calibration-not-yet-implemented)
2. **Configurable sensor ranges** — expose accel (±2/4/8/16g) and gyro (±250/500/1000/2000 °/s) range selection
3. **Temperature sensor** — the MPU6500 has an on-die temperature sensor (register `0x41`)
4. **Persistent calibration** — store magnetometer offsets in flash

## License

MIT License. See [LICENSE](LICENSE).

## Acknowledgments

- [Madgwick AHRS algorithm](https://x-io.co.uk/open-source-imu-and-ahrs-algorithms/) by Sebastian Madgwick (now maintained as [Fusion](https://github.com/xioTechnologies/Fusion))
- [Uspizig/esphome-mpu9250](https://github.com/Uspizig/esphome-mpu9250) — original (incomplete) component that inspired this implementation
- [MPU-9250 Product Specification (PDF)](https://github.com/kriswiner/MPU9250/blob/master/MPU-9250%20Spec.pdf) — via kriswiner's reference collection
- [MPU-9250 Register Map (PDF)](https://github.com/kriswiner/MPU9250/blob/master/MPU-9250%20Register%20Map.pdf) — via kriswiner's reference collection
