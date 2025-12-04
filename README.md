# Arduino Based Air-Quality Monitor
Arduino-based real-time air quality monitor using MQ-2, MQ-7, MQ-135, DHT11, and buzzer alert system.

# Features:
    - smoothing (moving average)
    - robust DHT reads with retries
    - treat saturated ADC (>=1000) as invalid
    - non-blocking buzzer beep pattern
    - simple hysteresis for alarms
    - Vcc read helper (AVR only) if you want to inspect supply
    
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

Buzzer Pinout
| Pin | Label  | Description            |
| --- | ------ | ---------------------- |
| +   | SIGNAL | Arduino digital **D8** |
| –   | GND    | Ground                 |

Arduino Pin Configuration
| Sensor / Module       | Arduino Pin  |
| --------------------- | ------------ |
| MQ-2 Analog Output    | **A0**       |
| MQ-7 Analog Output    | **A1**       |
| MQ-135 Analog Output  | **A2**       |
| DHT11 Data            | **D7**       |
| Buzzer Signal         | **D8**       |
| Power for all sensors | **5V & GND** |
