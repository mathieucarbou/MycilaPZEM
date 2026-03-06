/*
  * This example shows how to read the energy data of several PZEM004Tv3 devices on the same Serial port.
  * PZEM have to be assigned addresses before using this example.
  *
  * The circuit:
  * - PZEM004Tv3 #1 connected to Serial1 (RX=14, TX=27)
  * - PZEM004Tv3 #2 connected to Serial1 (RX=14, TX=27)
  *
  * Compile with -D MYCILA_JSON_SUPPORT to enable JSON support
  * Add ArduinoJson library to your project
*/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MycilaPZEM.h>

Mycila::PZEM pzem1; // 0x01
Mycila::PZEM pzem2; // 0x02

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  pzem1.setSharedSerial(true);
  pzem1.begin(Serial1, 14, 27, 0x01, true);

  pzem1.setSharedSerial(true);
  pzem2.begin(Serial1, 14, 27, 0x02, true);
}

void loop() {
  delay(1000);
  if (pzem1.isEnabled()) {
    JsonDocument doc;
    pzem1.toJson(doc.to<JsonObject>());
    Serial.printf("0x%02X ", pzem1.getDeviceAddress());
    serializeJson(doc, Serial);
    Serial.println();
  }

  if (pzem2.isEnabled()) {
    JsonDocument doc;
    pzem2.toJson(doc.to<JsonObject>());
    Serial.printf("0x%02X ", pzem2.getDeviceAddress());
    serializeJson(doc, Serial);
    Serial.println();
  }
}
