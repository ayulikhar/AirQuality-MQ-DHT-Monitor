# AirQuality-MQ-DHT-Monitor
Arduino-based real-time air quality monitor using MQ-2, MQ-7, MQ-135, DHT11, and buzzer alert system.

MQ-2 Pinout (Smoke, LPG, Flammable Gas Sensor)
| Pin | Label    | Description                         |
| --- | -------- | ----------------------------------- |
| 1   | **AOUT** | Analog output → Arduino **A0**      |
| 2   | **DOUT** | Digital output (optional, not used) |
| 3   | **GND**  | Ground                              |
| 4   | **VCC**  | +5V supply                          |

MQ-7 Pinout (Carbon Monoxide Sensor)
| Pin | Label    | Description                    |
| --- | -------- | ------------------------------ |
| 1   | **AOUT** | Analog output → Arduino **A1** |
| 2   | **DOUT** | Digital output (unused)        |
| 3   | **GND**  | Ground                         |
| 4   | **VCC**  | +5V                            |

DHT11 Pinout (Temperature & Humidity Sensor)
| Pin | Label    | Description                     |
| --- | -------- | ------------------------------- |
| 1   | **VCC**  | +5V                             |
| 2   | **DATA** | Digital signal → Arduino **D7** |
| 3   | **GND**  | Ground                          |
