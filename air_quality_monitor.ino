/*
  ============================================================
  AIR QUALITY MONITOR - CO + AQI + Bluetooth
  Sensors: MQ-7 (Carbon Monoxide), MQ-135 (AQI/Air Quality)
  Display: 16x2 I2C LCD
  Bluetooth: HC-05
  ============================================================
*/

#include <Wire.h>                    // I2C communication ke liye library
#include <LiquidCrystal_I2C.h>       // LCD ke liye library
#include <SoftwareSerial.h>          // HC-05 Bluetooth ke liye library

// ── LCD Setup ──────────────────────────────────────────────
// LCD ka address 0x27 hai, 16 characters aur 2 lines
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── Sensor Pins ────────────────────────────────────────────
int mq7_pin  = A0;   // MQ-7  → Carbon Monoxide (CO) detect karta hai
int mq135_pin = A1;  // MQ-135 → AQI/Air Quality detect karta hai
                     // (smoke, NH3, NOx, alcohol, benzene, CO2 bhi pakadta hai)

// ── HC-05 Bluetooth Pins ───────────────────────────────────
// SoftwareSerial se hum koi bhi digital pin use kar sakte hain
// RX → Arduino pin 10  (HC-05 ke TX se connect karo)
// TX → Arduino pin 11  (HC-05 ke RX se connect karo)
// DHYAN RAHO: HC-05 ka RX 3.3V ka hai, 5V directly mat lagao
//             Voltage divider use karo: 1kΩ + 2kΩ resistor
SoftwareSerial bluetooth(10, 11);  // (RX, TX)

// ── AQI Category helper variables ─────────────────────────
// MQ-135 ki raw value ko AQI range mein convert karne ke liye
// Yeh approximate mapping hai, calibration se aur accurate hoga
int aqi_value = 0;

// ── Timing variables ──────────────────────────────────────
unsigned long previousMillis = 0;
const long interval = 2000;  // Har 2 second mein data update hoga

// ── Display mode (LCD par kaun sa sensor dikhao) ──────────
int displayMode = 0;  // 0 = CO, 1 = AQI

// ============================================================
void setup() {
  // Serial Monitor start karna (debug ke liye)
  Serial.begin(9600);
  Serial.println("=== Air Quality Monitor Starting ===");

  // HC-05 Bluetooth start karna
  // HC-05 ka default baud rate 9600 hai
  bluetooth.begin(9600);
  bluetooth.println("HC-05 Connected! Air Quality Monitor Ready.");
  Serial.println("HC-05 Bluetooth: Ready hai bhai!");

  // LCD initialize karna
  lcd.init();
  lcd.backlight();  // LCD ki light on karo

  // Startup message dikhana
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Air Monitor   ");
  lcd.setCursor(0, 1);
  lcd.print(" CO + AQI + BT  ");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQ7: CO Sensor  ");
  lcd.setCursor(0, 1);
  lcd.print("MQ135: AQI Sens ");
  delay(2000);

  lcd.clear();
  Serial.println("Setup complete! Readings start hone wale hain...");
}

// ============================================================
void loop() {
  unsigned long currentMillis = millis();

  // ── Bluetooth se command receive karna ──────────────────
  // Agar koi phone/website se command bheje toh yahan process hoga
  if (bluetooth.available()) {
    char command = bluetooth.read();
    handleBluetoothCommand(command);
  }

  // ── Har 2 second mein sensor data update karna ──────────
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Sensor values padhna
    int co_raw  = analogRead(mq7_pin);    // MQ-7 se CO ka raw value
    int aqi_raw = analogRead(mq135_pin);  // MQ-135 se AQI ka raw value

    // AQI calculate karna (0-500 scale par map karna)
    // MQ-135 ka output 0-1023 hota hai → hum isko 0-500 AQI scale par laate hain
    aqi_value = map(aqi_raw, 0, 1023, 0, 500);

    // CO level (ppm approximate)
    // MQ-7 ke liye bhi approximate conversion
    float co_ppm = (co_raw / 1023.0) * 1000.0;  // 0-1000 ppm range

    // ── LCD par dikhana ─────────────────────────────────
    updateLCD(co_raw, aqi_value, co_ppm);

    // ── Serial Monitor par dikhana ──────────────────────
    printToSerial(co_raw, aqi_raw, co_ppm, aqi_value);

    // ── Bluetooth par data bhejna ───────────────────────
    sendBluetoothData(co_raw, aqi_raw, co_ppm, aqi_value);

    // Display mode toggle karna (CO → AQI → CO → ...)
    displayMode = (displayMode + 1) % 2;
  }
}

// ============================================================
// LCD Update Function
// Alternating display: pehle CO dikhao, phir AQI
// ============================================================
void updateLCD(int co_raw, int aqi, float co_ppm) {
  lcd.clear();

  if (displayMode == 0) {
    // CO (Carbon Monoxide) display
    lcd.setCursor(0, 0);
    lcd.print("CO Level (MQ-7):");
    lcd.setCursor(0, 1);
    lcd.print("Raw:");
    lcd.print(co_raw);
    lcd.print(" ~");
    lcd.print((int)co_ppm);
    lcd.print("ppm");
  } else {
    // AQI (Air Quality Index) display
    lcd.setCursor(0, 0);
    lcd.print("AQI (MQ-135):   ");
    lcd.setCursor(0, 1);
    lcd.print("AQI:");
    lcd.print(aqi);
    lcd.print(" ");
    lcd.print(getAQICategory(aqi));
  }
}

// ============================================================
// Serial Monitor Output Function
// ============================================================
void printToSerial(int co_raw, int aqi_raw, float co_ppm, int aqi) {
  Serial.println("────────────────────────────");
  Serial.print("MQ-7  CO  Raw Value : ");
  Serial.println(co_raw);
  Serial.print("CO Approximate (ppm): ");
  Serial.println(co_ppm);
  Serial.print("MQ-135 AQI Raw Value: ");
  Serial.println(aqi_raw);
  Serial.print("AQI Value           : ");
  Serial.println(aqi);
  Serial.print("Air Quality         : ");
  Serial.println(getAQICategory(aqi));
  Serial.println("────────────────────────────");
}

// ============================================================
// Bluetooth Data Send Function
// JSON format mein data bhejna taaki website easily parse kar sake
// ============================================================
void sendBluetoothData(int co_raw, int aqi_raw, float co_ppm, int aqi) {
  // JSON format: {"co_raw":350,"co_ppm":342.1,"aqi":125,"status":"Unhealthy"}
  bluetooth.print("{");
  bluetooth.print("\"co_raw\":");   bluetooth.print(co_raw);    bluetooth.print(",");
  bluetooth.print("\"co_ppm\":");   bluetooth.print(co_ppm, 1); bluetooth.print(",");
  bluetooth.print("\"aqi_raw\":"); bluetooth.print(aqi_raw);   bluetooth.print(",");
  bluetooth.print("\"aqi\":");      bluetooth.print(aqi);       bluetooth.print(",");
  bluetooth.print("\"status\":\""); bluetooth.print(getAQICategory(aqi)); bluetooth.print("\"");
  bluetooth.println("}");

  // Serial Monitor par bhi confirm karna
  Serial.println("Bluetooth par data bheja: JSON format");
}

// ============================================================
// Bluetooth Command Handler
// Phone ya website se commands receive karne ke liye
// Commands:
//   'C' → CO reading bhejo
//   'A' → AQI reading bhejo
//   'D' → Dono readings bhejo
//   'H' → Help / available commands list
//   'R' → Reset / restart message
// ============================================================
void handleBluetoothCommand(char cmd) {
  Serial.print("Bluetooth se command mila: ");
  Serial.println(cmd);

  int co_raw  = analogRead(mq7_pin);
  int aqi_raw = analogRead(mq135_pin);
  float co_ppm = (co_raw / 1023.0) * 1000.0;
  int aqi = map(aqi_raw, 0, 1023, 0, 500);

  switch (cmd) {
    case 'C':  // CO reading maango
    case 'c':
      bluetooth.print("CO Reading → Raw: ");
      bluetooth.print(co_raw);
      bluetooth.print(" | PPM: ");
      bluetooth.println(co_ppm, 1);
      break;

    case 'A':  // AQI reading maango
    case 'a':
      bluetooth.print("AQI Reading → Value: ");
      bluetooth.print(aqi);
      bluetooth.print(" | Status: ");
      bluetooth.println(getAQICategory(aqi));
      break;

    case 'D':  // Dono readings ek saath maango
    case 'd':
      sendBluetoothData(co_raw, aqi_raw, co_ppm, aqi);
      break;

    case 'H':  // Help maango
    case 'h':
      bluetooth.println("=== Commands ===");
      bluetooth.println("C → CO Level");
      bluetooth.println("A → AQI Level");
      bluetooth.println("D → All Data (JSON)");
      bluetooth.println("H → Help");
      bluetooth.println("R → Status Reset");
      break;

    case 'R':  // Reset message
    case 'r':
      bluetooth.println("Monitor Reset! Sab theek hai.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BT: Reset CMD!  ");
      delay(1000);
      break;

    default:
      bluetooth.print("Unknown command: ");
      bluetooth.println(cmd);
      bluetooth.println("'H' type karo help ke liye.");
      break;
  }
}

// ============================================================
// AQI Category Function
// AQI value ke hisaab se category batata hai
// Standard AQI scale:
//   0-50   → Good (Accha)
//   51-100 → Moderate (Theek hai)
//   101-150 → Unhealthy for Sensitive Groups
//   151-200 → Unhealthy (Bura)
//   201-300 → Very Unhealthy (Bahut Bura)
//   301+   → Hazardous (Khatarnak!)
// ============================================================
String getAQICategory(int aqi) {
  if (aqi <= 50)        return "Good";       // Hawa saaf hai
  else if (aqi <= 100)  return "Moderate";   // Theek-thaak hai
  else if (aqi <= 150)  return "Sens.Grp";   // Sensitive logon ke liye bura
  else if (aqi <= 200)  return "Unhealthy";  // Bura hai
  else if (aqi <= 300)  return "V.Unhealthy";// Bahut bura hai
  else                  return "HAZARDOUS";  // Khatarnak! Bahar mat jao
}

/*
  ============================================================
  WIRING GUIDE (Connection Diagram)
  ============================================================

  MQ-7 Sensor (Carbon Monoxide):
    VCC  → Arduino 5V
    GND  → Arduino GND
    AOUT → Arduino A0  ← mq7_pin

  MQ-135 Sensor (Air Quality / AQI):
    VCC  → Arduino 5V
    GND  → Arduino GND
    AOUT → Arduino A1  ← mq135_pin

  LCD 16x2 I2C Module:
    VCC  → Arduino 5V
    GND  → Arduino GND
    SDA  → Arduino A4 (Uno) ya SDA pin
    SCL  → Arduino A5 (Uno) ya SCL pin

  HC-05 Bluetooth Module:
    VCC  → Arduino 5V
    GND  → Arduino GND
    TXD  → Arduino Pin 10 (RX) ← bluetooth RX
    RXD  → Arduino Pin 11 (TX) via Voltage Divider!
             (Arduino TX 5V → 1kΩ → HC-05 RX)
             (Junction → 2kΩ → GND)
             Yeh zaruri hai warna HC-05 kharab ho sakta hai!

  ============================================================
  WEBSITE/APP CONNECTION (HC-05 se):
  ============================================================
  1. HC-05 ko Bluetooth se pair karo (default PIN: 1234 ya 0000)
  2. Web Browser mein Web Bluetooth API use karo
  3. Ya Python script se serial port connect karo
  4. JSON data aayega: {"co_raw":..., "co_ppm":..., "aqi":..., "status":"..."}
  5. Commands bhejo: C, A, D, H, R
  ============================================================
*/
