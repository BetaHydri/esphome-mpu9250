# ESPHome MPU9250 External Component

A full 9-axis IMU (Inertial Measurement Unit) external component for [ESPHome](https://esphome.io/), supporting the **InvenSense MPU9250** (MPU6500 accelerometer/gyroscope + AK8963 magnetometer).

Provides accelerometer, gyroscope, magnetometer, and tilt-compensated compass heading via Madgwick sensor fusion — all directly in Home Assistant.

## Features

- **Accelerometer** (X/Y/Z) — m/s²
- **Gyroscope** (X/Y/Z) — °/s
- **Magnetometer / AK8963** (X/Y/Z) — µT, accessed via I2C bypass mode
- **Compass Heading** — tilt-compensated yaw via Madgwick AHRS filter
- **Die Temperature** — internal MPU9250 die temperature in °C
- **Magnetometer Calibration** — hard-iron calibration via button entity in Home Assistant (30-second routine)
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

mpu9250:

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

mpu9250:
  address: 0x68
  update_interval: 200ms

button:
  - platform: mpu9250
    calibrate:
      name: "MPU9250 Calibrate Magnetometer"

sensor:
  - platform: mpu9250
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
    temperature:
      name: "MPU9250 Temperature"
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

#### Hub Component (`mpu9250:`)

| Variable | Type | Default | Description |
|---|---|---|---|
| `address` | int | `0x68` | I2C address of the MPU9250 |
| `update_interval` | time | `100ms` | How often to read the sensor |

#### Sensor Platform (`sensor: platform: mpu9250`)

| Variable | Type | Default | Description |
|---|---|---|---|
| `accel` | schema | — | Accelerometer axes (x, y, z). Values in m/s² |
| `gyro` | schema | — | Gyroscope axes (x, y, z). Values in °/s |
| `mag` | schema | — | Magnetometer axes (x, y, z). Values in µT |
| `heading` | sensor | — | Tilt-compensated compass heading (0–360°) |
| `temperature` | sensor | — | Internal die temperature in °C |
| `use_madgwick` | bool | `true` | Enable Madgwick AHRS filter for heading |
| `declination` | float | `0.0` | Magnetic declination in degrees for your location |

All axis groups (`accel`, `gyro`, `mag`) and `heading` are optional — include only what you need.

#### Button Platform (`button: platform: mpu9250`)

| Variable | Type | Description |
|---|---|---|
| `calibrate` | button | **Required.** Triggers a 30-second magnetometer hard-iron calibration |

**How calibration works:**

1. Press the calibration button in Home Assistant
2. Slowly rotate the sensor around all three axes for 30 seconds
3. The component tracks min/max magnetic field values on each axis
4. After 30 seconds, it computes midpoint offsets: `offset = (max + min) / 2`
5. All subsequent magnetometer readings are corrected by subtracting these offsets
6. Offsets are logged: check ESPHome logs for the computed values

> **Note:** Calibration offsets are stored in RAM and lost on reboot. For persistent calibration, note the logged offset values and apply them manually (persistent storage is a planned improvement).

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

### Vehicle Tilt Monitoring with Home Assistant

The raw accelerometer values from the MPU9250 can be used to compute vehicle tilt angles for RV/camper leveling. This requires two additional layers in Home Assistant: **template sensors** that convert raw acceleration to degrees, and **filter sensors** that smooth the output for stable dashboard display.

#### Why this is useful

- Raw accelerometer values (m/s²) are noisy and update at 200 ms — too jittery for a dashboard
- Converting to tilt angle via `asin(accel / 9.81)` gives human-readable degrees
- A low-pass filter removes vibration noise, producing a stable reading (e.g., `-1.4°`)
- The result is suitable for an RV leveling card showing front-back and left-right tilt

#### Processing chain

```
ESPHome (MPU9250)          Home Assistant
┌──────────────┐    ┌─────────────────┐    ┌──────────────────┐    ┌───────────┐
│ Accel X/Y/Z  │───>│ Template Sensor │───>│  Filter Sensor   │───>│ Dashboard │
│   (m/s²)     │    │ asin → degrees  │    │ lowpass smoothed │    │ Van Tilt  │
└──────────────┘    └─────────────────┘    └──────────────────┘    └───────────┘
```

#### Step 1: Create template sensors (HA UI)

Go to **Settings → Devices & Services → Helpers → Create Helper → Template → Template a sensor**.

**Hymer Gyro X Angle** (front-back tilt):

- **Name**: `Hymer Gyro X Angle`
- **State template**:

  ```jinja2
  {{ ((asin((states("sensor.hymer_gyro_mpu9250_mpu9250_accel_x") | float) / 9.81) * 180 / pi) - 0.7) * -1 }}
  ```

- **Unit of measurement**: `°`
- **State class**: `measurement_angle`

**Hymer Gyro Y Angle** (left-right tilt):

- **Name**: `Hymer Gyro Y Angle`
- **State template**:

  ```jinja2
  {{ ((asin((states("sensor.hymer_gyro_mpu9250_mpu9250_accel_y") | float) / 9.81) * 180 / pi) - 3.9) }}
  ```

- **Unit of measurement**: `°`
- **State class**: `measurement_angle`

> **Note:** The offset values (`- 0.7` for X, `- 3.9` for Y) compensate for the
> sensor's mounting angle in your specific vehicle. You will need to calibrate these
> for your own installation: park on a known level surface and note the raw angle
> values — those are your offsets.

#### Step 2: Create filter sensors (HA UI)

Go to **Settings → Devices & Services → Helpers → Create Helper → Filter**.

Create two filter sensors with these settings:

| Setting | Filtered Hymer Gyro X Angle | Filtered Hymer Gyro Y Angle |
|---|---|---|
| **Source entity** | `sensor.hymer_gyro_x_angle` | `sensor.hymer_gyro_y_angle` |
| **Filter** | Low-pass (Tiefpass) | Low-pass (Tiefpass) |
| **Window size** | 1 | 1 |
| **Time constant** | 5 | 5 |
| **Precision** | 1 | 1 |

The low-pass filter with time constant 5 smooths out vibration-induced jitter while
still responding to actual tilt changes within a few seconds.

#### Step 3: Use in a dashboard

Use the filtered sensors in a Lovelace card for your RV leveling display:

- `sensor.filtered_hymer_gyro_x_angle` — front/back tilt
- `sensor.filtered_hymer_gyro_y_angle` — left/right tilt

#### YAML equivalent (alternative to UI)

If you prefer YAML over the UI, add this to your `configuration.yaml`:

```yaml
template:
  - sensor:
      - name: "Hymer Gyro X Angle"
        unique_id: hymer_gyro_x_angle
        unit_of_measurement: "°"
        state_class: measurement_angle
        state: >
          {{ ((asin((states("sensor.hymer_gyro_mpu9250_mpu9250_accel_x")
               | float) / 9.81) * 180 / pi) - 0.7) * -1 }}

      - name: "Hymer Gyro Y Angle"
        unique_id: hymer_gyro_y_angle
        unit_of_measurement: "°"
        state_class: measurement_angle
        state: >
          {{ ((asin((states("sensor.hymer_gyro_mpu9250_mpu9250_accel_y")
               | float) / 9.81) * 180 / pi) - 3.9) }}

sensor:
  - platform: filter
    name: "Filtered Hymer Gyro X Angle"
    unique_id: filtered_hymer_gyro_x_angle
    entity_id: sensor.hymer_gyro_x_angle
    filters:
      - filter: lowpass
        time_constant: 5
        precision: 1

  - platform: filter
    name: "Filtered Hymer Gyro Y Angle"
    unique_id: filtered_hymer_gyro_y_angle
    entity_id: sensor.hymer_gyro_y_angle
    filters:
      - filter: lowpass
        time_constant: 5
        precision: 1
```

### Calibration Offsets Not Persistent

Magnetometer calibration offsets are stored in RAM. They are lost on reboot. To work around this, run calibration, note the offset values from the ESPHome log output, and consider using ESPHome `globals` with `restore_value: true` to persist them. Persistent storage is a planned improvement.

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
│       ├── __init__.py      # Hub component (registers MPU9250Component)
│       ├── sensor.py        # Sensor platform (accel/gyro/mag/heading)
│       ├── button.py        # Button platform (magnetometer calibration)
│       ├── mpu9250.h        # C++ component header
│       ├── mpu9250.cpp      # C++ I2C driver (MPU6500 + AK8963)
│       └── madgwick.h       # Madgwick AHRS filter (header-only)
├── example.yaml             # Full example configuration
├── LICENSE
└── README.md
```

## Contributing

Contributions are welcome. Priority areas:

1. **Configurable sensor ranges** — expose accel (±2/4/8/16g) and gyro (±250/500/1000/2000 °/s) range selection
2. **Persistent calibration** — store magnetometer offsets in flash

## License

MIT License. See [LICENSE](LICENSE).

## Acknowledgments

- [Madgwick AHRS algorithm](https://x-io.co.uk/open-source-imu-and-ahrs-algorithms/) by Sebastian Madgwick (now maintained as [Fusion](https://github.com/xioTechnologies/Fusion))
- [Uspizig/esphome-mpu9250](https://github.com/Uspizig/esphome-mpu9250) — original (incomplete) component that inspired this implementation
- [MPU-9250 Product Specification (PDF)](https://github.com/kriswiner/MPU9250/blob/master/Documents/PS-MPU-9250A-01.pdf)
- [MPU-9250 Register Map (PDF)](https://github.com/kriswiner/MPU9250/blob/master/Documents/RM-MPU-9250A-00.pdf)
- [AK8963 Magnetometer Datasheet (PDF)](https://github.com/kriswiner/MPU9250/blob/master/Documents/AK8963.pdf)
