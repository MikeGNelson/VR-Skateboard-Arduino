# Balance Board Bluetooth

ESP32 firmware for a custom balance-board controller, reading load, distance, and accelerometer data and transmitting the resulting sensor data over Bluetooth Serial or serial.

## Hardware

The controller uses:

- ESP32
- 4 × CS1237 24-bit ADCs
- 4 × VL53L1X distance sensors
- KX134 accelerometer
- Qwiic I2C multiplexer
- 4 × load-cell inputs
- 2 × NeoPixel LEDs

## Sensor Data

The firmware reads four CS1237 ADC channels and converts the measurements into normalized load values.

When elastic sensing is enabled, it also reads:

- Four VL53L1X distance measurements
- X, Y, and Z acceleration from the KX134

Running averages are used to track sensor values and reject outliers when updating calibration ranges.

## Bluetooth

The ESP32 advertises over Bluetooth Serial as:

```text
BalanceBoard
```

Each loop assembles the current sensor values into a comma-separated packet and transmits it using Bluetooth Serial.

With elastic sensing enabled, the packet format is:

```text
elastic,load1,load2,load3,load4,distance1,distance2,distance3,distance4,accelX,accelY,accelZ
```

When elastic sensing is disabled, only the elastic flag and four load values are transmitted.

## CS1237 Connections

The four CS1237 ADCs use the following ESP32 pins:

| ADC   | SCLK | DOUT |
| ----- | ---: | ---: |
| ADC 1 |    4 |    5 |
| ADC 2 |    6 |    7 |
| ADC 3 |    8 |    9 |
| ADC 4 |   10 |   11 |

## I2C Multiplexer

The Qwiic multiplexer assigns the sensors to the following ports:

| Port | Device  |
| ---: | ------- |
|    0 | VL53L1X |
|    1 | VL53L1X |
|    2 | VL53L1X |
|    3 | VL53L1X |
|    4 | KX134   |

## Dependencies

The firmware uses:

- `BluetoothSerial`
- `Wire`
- `SparkFun_I2C_Mux_Arduino_Library`
- `SparkFun_VL53L1X`
- `SparkFun_KX13X`
- `Adafruit_NeoPixel`
- `RunningAverage`
- `CS1237`

The `CS1237` library used by this project is available separately in the CS1237 Arduino repository.

## License

MIT License
