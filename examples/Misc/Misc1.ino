// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#include <Arduino.h>
#include <MycilaPZEM.h>

static Mycila::PZEM pzem1; // 0x01
static Mycila::PZEM pzem2; // 0x02
static Mycila::PZEM pzem3; // 0x03
static Mycila::PZEM pzem4; // 0x04

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  pzem1.setCallback([](Mycila::PZEM::EventType eventType, const Mycila::PZEM::Data& data) { Serial.printf("PZEM 1 Power: %.1f W\n", data.activePower); });
  pzem2.setCallback([](Mycila::PZEM::EventType eventType, const Mycila::PZEM::Data& data) { Serial.printf("PZEM 2 Power: %.1f W\n", data.activePower); });
  pzem3.setCallback([](Mycila::PZEM::EventType eventType, const Mycila::PZEM::Data& data) { Serial.printf("PZEM 3 Power: %.1f W\n", data.activePower); });
  pzem4.setCallback([](Mycila::PZEM::EventType eventType, const Mycila::PZEM::Data& data) { Serial.printf("PZEM 4 Power: %.1f W\n", data.activePower); });

  // Serial1
  pzem1.setSharedSerial(true);
  pzem1.begin(Serial1, 14, 27, 0x01);
  pzem2.setSharedSerial(true);
  pzem2.begin(Serial1, 14, 27, 0x02);

  // Serial2
  pzem3.setSharedSerial(true);
  pzem3.begin(Serial2, 16, 17, 0x03);
  pzem4.setSharedSerial(true);
  pzem4.begin(Serial2, 16, 17, 0x04);
}

void loop() {
  pzem1.read();
  pzem2.read();
  pzem3.read();
  pzem4.read();
}
