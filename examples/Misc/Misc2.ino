// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#include <Arduino.h>
#include <MycilaPZEM.h>
#include <esp_err.h>
#include <vector>

// you can use core 0 if your callback is VERY fast and non blocking, otherwise use core 1 to avoid blocking the Arduino core tasks
#define PZEM_ASYNC_TASK_CORE 0

// you should be able to decrease it to 2048 or 3072.
// it all depends on what you do in your callback
// this is to be tested.
#define PZEM_ASYNC_TASK_STACK_SIZE 4096

static Mycila::PZEM pzem1; // 0x01
static Mycila::PZEM pzem2; // 0x02
static Mycila::PZEM pzem3; // 0x03
static Mycila::PZEM pzem4; // 0x04

static std::vector<Mycila::PZEM*> pzemSerial1 = {&pzem1, &pzem2};
static std::vector<Mycila::PZEM*> pzemSerial2 = {&pzem3, &pzem4};

static void async_read(void* params) {
  std::vector<Mycila::PZEM*>* pzems = (std::vector<Mycila::PZEM*>*)params;
  while (true) {
    bool activity = false;
    for (Mycila::PZEM* pzem : *pzems)
      activity |= pzem->read();
    if (!activity)
      vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

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

  ESP_ERROR_CHECK(xTaskCreateUniversal(async_read, "pzem_serial1", PZEM_ASYNC_TASK_STACK_SIZE, &pzemSerial1, 1, NULL, PZEM_ASYNC_TASK_CORE));
  ESP_ERROR_CHECK(xTaskCreateUniversal(async_read, "pzem_serial2", PZEM_ASYNC_TASK_STACK_SIZE, &pzemSerial2, 1, NULL, PZEM_ASYNC_TASK_CORE));
}

void loop() {
  // arduino core task not needed
  vTaskDelete(NULL);
}
