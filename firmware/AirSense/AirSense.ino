/*
  Air Quality Monitor
  Wiring:
    MQ2   -> A0
    MQ7   -> A1
    MQ135 -> A2
    DHT11 -> D7 (digital)
    Buzzer-> D8

  Features:
    - smoothing (moving average)
    - robust DHT reads with retries
    - treat saturated ADC (>=1000) as invalid
    - non-blocking buzzer beep pattern
    - simple hysteresis for alarms
    - Vcc read helper (AVR only) if you want to inspect supply
*/

#include <DHT.h>

// Pins
const int MQ2_PIN    = A0;
const int MQ7_PIN    = A1;
const int MQ135_PIN  = A2;
const int DHT_PIN    = 7;   // DHT data -> digital pin 7
const int BUZZER_PIN = 8;

// DHT setup
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// Thresholds (tune after observing baselines)
int THRESH_MQ2   = 300;
int THRESH_MQ7   = 300;
int THRESH_MQ135 = 300;

// Smoothing
const int SMOOTH_WINDOW = 6; // increase for more smoothing
int mq2Buf[SMOOTH_WINDOW];
int mq7Buf[SMOOTH_WINDOW];
int mq135Buf[SMOOTH_WINDOW];
int idx_mq2 = 0, idx_mq7 = 0, idx_mq135 = 0;

// Timing
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 1500;
unsigned long lastDHTread = 0;
const unsigned long DHT_INTERVAL = 2000UL; // DHT11 safe minimum

// Buzzer non-blocking pattern
bool buzzerOn = false;
unsigned long buzzerLastToggle = 0;
const unsigned long BUZZ_ON_MS  = 200;
const unsigned long BUZZ_OFF_MS = 200;

// Alarm & hysteresis
bool alarmState = false;
const int HYST = 25; // ADC counts of hysteresis

// DHT failure tracking
int dhtFailCount = 0;

// Helper: initialize analog buffers safely
int initAnalogSafe(int pin) {
  int v = analogRead(pin);
  delay(5);
  return v;
}

// Read Vcc (millivolts) using internal 1.1V reference (AVR only)
long readVcc() {
#if defined(__AVR__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  long result = ADC;
  long vcc = 1125300L / result; // (1.1*1023*1000)/result
  return vcc;
#else
  return -1;
#endif
}

// Robust DHT read with retries
bool readDHTrobust(float &t, float &h, int tries = 5) {
  for (int i = 0; i < tries; i++) {
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) return true;
    delay(150);
  }
  return false;
}

// Safe analog read: return -1 if saturated/invalid
int safeAnalog(int pin) {
  int v = analogRead(pin);
  if (v >= 1000) return -1; // treat as saturated/invalid
  return v;
}

// Moving average update
int smoothedReadingAndUpdate(int buf[], int &index, int newVal) {
  buf[index] = newVal;
  index++;
  if (index >= SMOOTH_WINDOW) index = 0;
  long sum = 0;
  for (int i = 0; i < SMOOTH_WINDOW; i++) sum += buf[i];
  return (int)(sum / SMOOTH_WINDOW);
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println(F("=== Air Quality Monitor (DHT -> D7) starting ==="));
  Serial.println(F("Make sure DHT data has 4.7-10k pull-up to 5V and short data wire"));
  Serial.println(F("Warm MQ sensors ~10-30 min for stable readings"));
  Serial.println();

  // Init smoothing buffers
  int v2  = initAnalogSafe(MQ2_PIN);
  int v7  = initAnalogSafe(MQ7_PIN);
  int v135= initAnalogSafe(MQ135_PIN);
  for (int i = 0; i < SMOOTH_WINDOW; i++) {
    mq2Buf[i] = (v2 >= 1000 ? 512 : v2);      // if initial saturated, pick mid value
    mq7Buf[i] = (v7 >= 1000 ? 512 : v7);
    mq135Buf[i] = (v135 >= 1000 ? 512 : v135);
  }
}

void loop() {
  unsigned long now = millis();

  // Read sensors (safeAnalog treats >=1000 as invalid (-1))
  int raw_mq2   = safeAnalog(MQ2_PIN);
  int raw_mq7   = safeAnalog(MQ7_PIN);
  int raw_mq135 = safeAnalog(MQ135_PIN);

  // Replace invalid (-1) with previous buffer average value to keep smoothing stable
  int fallback_mq2   = mq2Buf[(idx_mq2 + SMOOTH_WINDOW - 1) % SMOOTH_WINDOW];
  int fallback_mq7   = mq7Buf[(idx_mq7 + SMOOTH_WINDOW - 1) % SMOOTH_WINDOW];
  int fallback_mq135 = mq135Buf[(idx_mq135 + SMOOTH_WINDOW - 1) % SMOOTH_WINDOW];

  int in_mq2   = (raw_mq2 == -1) ? fallback_mq2 : raw_mq2;
  int in_mq7   = (raw_mq7 == -1) ? fallback_mq7 : raw_mq7;
  int in_mq135 = (raw_mq135 == -1) ? fallback_mq135 : raw_mq135;

  int val_mq2   = smoothedReadingAndUpdate(mq2Buf, idx_mq2, in_mq2);
  int val_mq7   = smoothedReadingAndUpdate(mq7Buf, idx_mq7, in_mq7);
  int val_mq135 = smoothedReadingAndUpdate(mq135Buf, idx_mq135, in_mq135);

  // DHT read (robust, with retries) every DHT_INTERVAL ms
  static float humidity = NAN;
  static float tempC = NAN;
  if (now - lastDHTread >= DHT_INTERVAL) {
    float h, t;
    bool ok = readDHTrobust(t, h, 4);
    if (ok) {
      humidity = h;
      tempC = t;
    } else {
      dhtFailCount++;
    }
    lastDHTread = now;
  }

  // Alarm detection (use valid values only)
  bool triggered = false;
  bool mq2_valid = (raw_mq2 != -1);
  bool mq7_valid = (raw_mq7 != -1);
  bool mq135_valid = (raw_mq135 != -1);

  if (mq2_valid && val_mq2 >= THRESH_MQ2) triggered = true;
  if (mq7_valid && val_mq7 >= THRESH_MQ7) triggered = true;
  if (mq135_valid && val_mq135 >= THRESH_MQ135) triggered = true;

  // Hysteresis handling
  if (triggered && !alarmState) {
    alarmState = true;
  } else if (!triggered && alarmState) {
    // Only clear alarm if well below thresholds - hysteresis prevents flicker
    bool belowAll = true;
    if (mq2_valid && !(val_mq2 < THRESH_MQ2 - HYST)) belowAll = false;
    if (mq7_valid && !(val_mq7 < THRESH_MQ7 - HYST)) belowAll = false;
    if (mq135_valid && !(val_mq135 < THRESH_MQ135 - HYST)) belowAll = false;
    if (belowAll) alarmState = false;
  }

  // Non-blocking buzzer pattern when alarm active
  if (alarmState) {
    if (!buzzerOn && (now - buzzerLastToggle >= BUZZ_OFF_MS)) {
      digitalWrite(BUZZER_PIN, HIGH); buzzerOn = true; buzzerLastToggle = now;
    } else if (buzzerOn && (now - buzzerLastToggle >= BUZZ_ON_MS)) {
      digitalWrite(BUZZER_PIN, LOW); buzzerOn = false; buzzerLastToggle = now;
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
    buzzerLastToggle = now;
  }

  // Serial print
  if (now - lastPrint >= PRINT_INTERVAL) {
    long vcc = readVcc(); // -1 on non-AVR boards
    Serial.print("Vcc(mV): ");
    if (vcc > 0) Serial.print(vcc); else Serial.print("N/A");
    Serial.print("  MQ2(A0): ");
    if (raw_mq2 == -1) Serial.print(String(val_mq2) + " (sat/invalid)");
    else Serial.print(val_mq2);
    Serial.print("  MQ7(A1): ");
    if (raw_mq7 == -1) Serial.print(String(val_mq7) + " (sat/invalid)");
    else Serial.print(val_mq7);
    Serial.print("  MQ135(A2): ");
    if (raw_mq135 == -1) Serial.print(String(val_mq135) + " (sat/invalid)");
    else Serial.print(val_mq135);

    if (!isnan(humidity) && !isnan(tempC)) {
      Serial.print("    Temp(C): "); Serial.print(tempC,1);
      Serial.print("  Hum(%): "); Serial.print(humidity,1);
    } else {
      Serial.print("    Temp/Hum: --");
    }
    Serial.print("  DHTfails: "); Serial.print(dhtFailCount);

    if (alarmState) {
      Serial.print("    >>> ALERT! ");
      Serial.print("Sources:");
      if (mq2_valid && val_mq2 >= THRESH_MQ2) Serial.print(" MQ2");
      if (mq7_valid && val_mq7 >= THRESH_MQ7) Serial.print(" MQ7");
      if (mq135_valid && val_mq135 >= THRESH_MQ135) Serial.print(" MQ135");
    }
    Serial.println();
    lastPrint = now;
  }

  // small loop delay to let ADC settle and keep code responsive
  delay(40);
}
