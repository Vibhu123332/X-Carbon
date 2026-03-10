#include <WiFi.h>
#include <WebServer.h>

/* WIFI SETTINGS */

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

/* SERVER */

WebServer server(80);

/* SENSOR PINS */

const int carbonSensorPin = 34;     // MQ135 or analog sensor
const int dustSensorPin = 35;

/* VARIABLES */

int carbonValue = 0;
int dustValue = 0;
float inkProduced = 0;
int aqi = 0;

/* HANDLE DATA REQUEST */

void handleData() {

  carbonValue = analogRead(carbonSensorPin);
  dustValue = analogRead(dustSensorPin);

  /* simple calculation logic */

  inkProduced = carbonValue * 0.02;
  aqi = carbonValue / 2;

  String json = "{";
  json += "\"carbon\":";
  json += carbonValue;
  json += ",";
  json += "\"dust\":";
  json += dustValue;
  json += ",";
  json += "\"ink\":";
  json += inkProduced;
  json += ",";
  json += "\"aqi\":";
  json += aqi;
  json += "}";

  server.send(200, "application/json", json);
}

/* ROOT PAGE */

void handleRoot() {
  server.send(200,"text/plain","X-Carbon ESP32 Server Running");
}

/* SETUP */

void setup() {

  Serial.begin(115200);

  pinMode(carbonSensorPin,INPUT);
  pinMode(dustSensorPin,INPUT);

  WiFi.begin(ssid,password);

  Serial.println("Connecting to WiFi...");

  while(WiFi.status()!=WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected!");

  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",handleRoot);
  server.on("/data",handleData);

  server.begin();

  Serial.println("Server started");
}

/* LOOP */

void loop(){

  server.handleClient();

}