#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>
#include "BluetoothSerial.h"  // Include Bluetooth Serial Library

// AS5600 Magnetic Encoder
AS5600 encoder;
BluetoothSerial SerialBT;  // Create Bluetooth Serial Object

unsigned long lastTime = 0;
int lastAngle = 0;
float rpm = 0.0;
float radPerSec = 0.0;

void setup() {
    Serial.begin(115200);  // USB Serial (for debugging)
    SerialBT.begin("ESP_Magnetic_Encoder");  // Bluetooth Device Name
    delay(2000); // Give time to initialize

    Serial.println("\nESP32 Booting...");

    Wire.begin(21, 22);  // SDA = GPIO 21, SCL = GPIO 22
    if (!encoder.begin()) {
        Serial.println("AS5600 Not Detected! Check Wiring.");
        while (1); // Halt execution
    }

    Serial.println("AS5600 Initialized!");

    lastAngle = encoder.readAngle(); // Store initial angle
    lastTime = millis();
}

void loop() {
    static unsigned long lastSendTime = 0;
    unsigned long currentTime = millis();

    // if (SerialBT.available()) {
    //     char received = SerialBT.read();
    //     Serial.print(received);  // Echo received data
    // }

    // Send sensor data every 100ms
    if (currentTime - lastSendTime >= 50) {  
        lastSendTime = currentTime;

        int rawAngle = encoder.readAngle();
        float angle = rawAngle * 0.087; // Convert to degrees
        unsigned long deltaTime = currentTime - lastTime;

        if (deltaTime > 0) {
            int deltaAngle = rawAngle - lastAngle;

            // Handle angle wraparound (AS5600 measures from 0-4095)
            if (deltaAngle > 2048) deltaAngle -= 4096;
            else if (deltaAngle < -2048) deltaAngle += 4096;

            float revolutions = deltaAngle / 4096.0;
            rpm = (revolutions / deltaTime) * 60000.0;  // Convert ms to minutes
            radPerSec = rpm * (2 * 3.14159265358979) / 60.0;
        }

        // Create binary packet
        uint8_t buffer[18];
        buffer[0] = 0xAA;
        buffer[1] = 0x55;
        memcpy(buffer + 2, &rawAngle, sizeof(rawAngle));  // 4 bytes
        memcpy(buffer + 6, &angle, sizeof(angle));    // 4 bytes
        memcpy(buffer + 10, &rpm, sizeof(rpm));        // 4 bytes
        memcpy(buffer + 14, &radPerSec, sizeof(radPerSec));  // 4 bytes

        // Send binary data over Bluetooth Serial
        SerialBT.write(buffer, sizeof(buffer));
        // Serial.println(angle);
        // printBufferBinary(buffer, sizeof(buffer),true);
        Serial.write(buffer, sizeof(buffer));

        lastAngle = rawAngle;
        lastTime = currentTime;
    }

    // // Check for incoming Bluetooth commands
    // if (SerialBT.available()) {
    //     String command = SerialBT.readStringUntil('\n');
    //     command.trim();

    //     if (command == "ping") {
    //         SerialBT.println("pong");  // Respond to keep-alive pings
    //     } else if (command == "status") {
    //         SerialBT.printf("Angle: %.2f, RPM: %.2f, Rad/S: %.2f\n", (lastAngle * 0.087), rpm, radPerSec);
    //     }
    // }
}

// void printBufferBinaryconst uint8_t* buffer, size_t length, bool debug = false) {
//     // Send raw binary over USB Serial
//     Serial.write(buffer, length);

//     // Optional debug print of binary content
//     if (debug) {
//         Serial.print("Binary Packet: ");
//         for (size_t i = 0; i < length; ++i) {
//             for (int bit = 7; bit >= 0; --bit) {
//                 Serial.print((buffer[i] >> bit) & 1);
//             }
//             Serial.print(" ");
//         }
//         Serial.println();
//     }
// }
