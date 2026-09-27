# VR-Skateboard-Arduino
ESP32 firmware for the VR Skateboard magnetic encoder, streaming angle and rotational velocity data over Bluetooth Serial.

Hardware
- ESP32
- AS5600 magnetic rotary encoder
- Magnetic encoder magnet

Wiring
AS5600 SDA -> ESP32 GPIO 21
AS5600 SCL -> ESP32 GPIO 22

Bluetooth device
ESP_Magnetic_Encoder

Update rate
50 ms / ~20 Hz

Packet format
Byte 0-1   Header: 0xAA 0x55
Byte 2-5   Raw encoder angle
Byte 6-9   Angle in degrees
Byte 10-13 RPM
Byte 14-17 Angular velocity (rad/s)
