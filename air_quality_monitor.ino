// ==== PIN DEFINITIONS ====
#define MQ7_PIN 34
#define MQ135_PIN 35
#define FAN_PIN 12   // Relay or MOSFET
#define PM_SENSOR_RX 16
#define PM_SENSOR_TX 17

// Thresholds (adjust after testing)
int CO_THRESHOLD = 300;
int AIR_THRESHOLD = 400;

#include <HardwareSerial.h>
HardwareSerial PMSerial(1);

// ==== SETUP ====
void setup() {
  Serial.begin(115200);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  PMSerial.begin(9600, SERIAL_8N1, PM_SENSOR_RX, PM_SENSOR_TX);

  Serial.println("X Carbon System Started...");
}

// ==== LOOP ====
void loop() {

  // Read gas sensors
  int mq7_value = analogRead(MQ7_PIN);
  int mq135_value = analogRead(MQ135_PIN);

  // Simulated PM value (replace with real parsing)
  int pm_value = readPM();

  Serial.print("MQ7: "); Serial.print(mq7_value);
  Serial.print(" | MQ135: "); Serial.print(mq135_value);
  Serial.print(" | PM: "); Serial.println(pm_value);

  // ==== AGENTIC DECISION LOGIC ====
  if (mq7_value > CO_THRESHOLD || mq135_value > AIR_THRESHOLD || pm_value > 150) {
    
    Serial.println("Pollution HIGH → Activating Fan");
    digitalWrite(FAN_PIN, HIGH);

    // You can also send signal to drone controller here
    // Example: Serial.println("MOVE TO TARGET ZONE");

  } else {
    
    Serial.println("Air Clean → Fan OFF");
    digitalWrite(FAN_PIN, LOW);
  }

  delay(2000);
}

// ==== PM SENSOR FUNCTION (BASIC) ====
int readPM() {
  if (PMSerial.available()) {
    return PMSerial.read(); // Simplified (replace with real decoding)
  }
  return 0;
}
