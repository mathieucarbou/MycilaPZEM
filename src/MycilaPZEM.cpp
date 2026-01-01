// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#include "MycilaPZEM.h"

#ifdef MYCILA_LOGGER_SUPPORT
  #include <MycilaLogger.h>
extern Mycila::Logger logger;
  #define LOGD(tag, format, ...) logger.debug(tag, format, ##__VA_ARGS__)
  #define LOGI(tag, format, ...) logger.info(tag, format, ##__VA_ARGS__)
  #define LOGW(tag, format, ...) logger.warn(tag, format, ##__VA_ARGS__)
  #define LOGE(tag, format, ...) logger.error(tag, format, ##__VA_ARGS__)
#else
  #define LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
  #define LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
  #define LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
  #define LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#endif

#define TAG "PZEM004T"

#define PZEM_BAUD_RATE 9600
#define PZEM_TIMEOUT   200

#define PZEM_CMD_RHR  0x03 // Read holding register
#define PZEM_CMD_RIR  0X04 // Read input register
#define PZEM_CMD_WSR  0x06 // Write single register
#define PZEM_CMD_REST 0x42 // Reset Energy

#define PZEM_REGISTER_ADDRESS 0x0002

#define PZEM_REGISTER_VOLTAGE   0x0000
#define PZEM_REGISTER_CURRENT   0x0001 // 0x0001 (low) 0x0002 (high)
#define PZEM_REGISTER_POWER     0x0003 // 0x0003 (low) 0x0004 (high)
#define PZEM_REGISTER_ENERGY    0x0005 // 0x0005 (low) 0x0006 (high)
#define PZEM_REGISTER_FREQUENCY 0x0007
#define PZEM_REGISTER_PF        0x0008

#define PZEM_REGISTER_LEN   2 // 2 bytes per register
#define PZEM_REGISTER_COUNT 9 // 9 registers

#define PZEM_RESPONSE_SIZE_READ_METRICS 23 // address(1), cmd(1), len(1), data(9*2), crc(2)
#define PZEM_RESPONSE_SIZE_READ_ADDR    7  // address(1), cmd(1), len(1), address(2), crc(2)
#define PZEM_RESPONSE_SIZE_RESET        4  // address(1), cmd(1), crc(2)
#define PZEM_RESPONSE_SIZE_SET_ADDR     8  // address(1), cmd(1), len(1), register(1), data(2), crc(2)

#define PZEM_RESPONSE_READ_DATA 3
#define PZEM_RESPONSE_ADDRESS   0

#ifndef GPIO_IS_VALID_OUTPUT_GPIO
  #define GPIO_IS_VALID_OUTPUT_GPIO(gpio_num) ((gpio_num >= 0) && \
                                               (((1ULL << (gpio_num)) & SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0))
#endif

#ifndef GPIO_IS_VALID_GPIO
  #define GPIO_IS_VALID_GPIO(gpio_num) ((gpio_num >= 0) && \
                                        (((1ULL << (gpio_num)) & SOC_GPIO_VALID_GPIO_MASK) != 0))
#endif

// Pre computed CRC table
// clang-format off
static constexpr uint16_t crcTable[] PROGMEM = {
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
  0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
  0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
  0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
  0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
  0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
  0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
  0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
  0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
  0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
  0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
  0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
  0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};
// clang-format on

#if MYCILA_PZEM_ASYNC_MAX_INSTANCES > 0
Mycila::PZEM* Mycila::PZEM::_instances[MYCILA_PZEM_ASYNC_MAX_INSTANCES];
#endif

TaskHandle_t Mycila::PZEM::_taskHandle = NULL;
std::mutex Mycila::PZEM::_mutex;
size_t Mycila::PZEM::_serialUsers = 0;

void Mycila::PZEM::begin(HardwareSerial& serial,
                         const int8_t rxPin,
                         const int8_t txPin,
                         const uint8_t address,
                         const bool async) {
  if (_enabled)
    return;

  if (GPIO_IS_VALID_GPIO(rxPin)) {
    _pinRX = (gpio_num_t)rxPin;
  } else {
    LOGE(TAG, "Disable PZEM: Invalid RX pin: %" PRId8, rxPin);
    _pinRX = GPIO_NUM_NC;
    return;
  }

  if (GPIO_IS_VALID_OUTPUT_GPIO(txPin)) {
    _pinTX = (gpio_num_t)txPin;
  } else {
    LOGE(TAG, "Disable PZEM: Invalid TX pin: %" PRId8, txPin);
    _pinTX = GPIO_NUM_NC;
    return;
  }

  if (address < MYCILA_PZEM_ADDRESS_MIN || address > MYCILA_PZEM_ADDRESS_GENERAL) {
    LOGE(TAG, "Disable PZEM: Invalid address: 0x%02X", address);
    return;
  }

  _serial = &serial;
  _address = address;

  if (async) {
#if MYCILA_PZEM_ASYNC_MAX_INSTANCES > 0
    LOGI(TAG, "Enable PZEM @ 0x%02X on Serial RX (PZEM TX Pin): %" PRId8 " and Serial TX (PZEM RX Pin): %" PRId8, address, rxPin, txPin);
    _enabled = _add(this);
#else
    _enabled = false;
#endif

  } else {
    _enabled = true;
    _openSerial(_pinRX, _pinTX);
    if (_address == MYCILA_PZEM_ADDRESS_GENERAL && readDeviceAddress(true) == MYCILA_PZEM_ADDRESS_UNKNOWN) {
      LOGE(TAG, "Unable to read any PZEM @ 0x%02X: Please verify that the device is powered on and that its address is correctly set if not using the default address", address);
      serial.flush();
      _drop();
    } else {
      LOGI(TAG, "Enable PZEM @ 0x%02X on Serial RX (PZEM TX Pin): %" PRId8 " and Serial TX (PZEM RX Pin): %" PRId8, _address, _pinRX, _pinTX);
    }
  }
}

void Mycila::PZEM::end() {
  if (_enabled) {
    LOGI(TAG, "Disable PZEM @ 0x%02X", _address);
    _enabled = false;
#if MYCILA_PZEM_ASYNC_MAX_INSTANCES > 0
    _remove(this);
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sharedSerial && _serialUsers)
      _serialUsers--;
    if (!_sharedSerial || !_serialUsers) {
      LOGD(TAG, "Close Serial");
      _serial->end();
    }
    _serial = nullptr;
    _lastAddress = MYCILA_PZEM_ADDRESS_UNKNOWN;
    _address = MYCILA_PZEM_ADDRESS_GENERAL;
    _data.clear();
  }
}

///////////////////////////////////////////////////////////////////////////////
// read
///////////////////////////////////////////////////////////////////////////////

bool Mycila::PZEM::read(uint8_t address) {
  if (!_enabled)
    return false;

  std::lock_guard<std::mutex> lock(_mutex);

  _send(address, PZEM_CMD_RIR, PZEM_REGISTER_VOLTAGE, PZEM_REGISTER_COUNT);
  ReadResult result = _timedRead(address, PZEM_RESPONSE_SIZE_READ_METRICS);

  if (result == ReadResult::READ_TIMEOUT) {
    // reset live values in case of read timeout
    _data.clear();
    if (_callback) {
      _callback(EventType::EVT_READ_TIMEOUT, _data);
    }
    return false;
  }

  if (result == ReadResult::READ_ERROR_COUNT || result == ReadResult::READ_ERROR_CRC) {
    // reset live values in case of read failure
    _data.clear();
    if (_callback) {
      _callback(EventType::EVT_READ_ERROR, _data);
    }
    return false;
  }

  if (result == ReadResult::READ_ERROR_ADDRESS) {
    // we have set a destination address, but we read another device
    if (_callback) {
      _callback(EventType::EVT_READ_ERROR, _data);
    }
    return false;
  }

  assert(result == ReadResult::READ_SUCCESS);

  _data.voltage = (static_cast<uint32_t>(_buffer[3]) << 8 | static_cast<uint32_t>(_buffer[4])) * 0.1f;                                                                                            // Raw voltage in 0.1V
  _data.current = (static_cast<uint32_t>(_buffer[5]) << 8 | static_cast<uint32_t>(_buffer[6] | static_cast<uint32_t>(_buffer[7]) << 24 | static_cast<uint32_t>(_buffer[8]) << 16)) * 0.001f;      // Raw current in 0.001A
  _data.activePower = (static_cast<uint32_t>(_buffer[9]) << 8 | static_cast<uint32_t>(_buffer[10] | static_cast<uint32_t>(_buffer[11]) << 24 | static_cast<uint32_t>(_buffer[12]) << 16)) * 0.1f; // Raw power in 0.1W
  _data.activeEnergy = (static_cast<uint32_t>(_buffer[13]) << 8 | static_cast<uint32_t>(_buffer[14] | static_cast<uint32_t>(_buffer[15]) << 24 | static_cast<uint32_t>(_buffer[16]) << 16));      // Raw Energy in 1Wh
  _data.frequency = (static_cast<uint32_t>(_buffer[17]) << 8 | static_cast<uint32_t>(_buffer[18])) * 0.1f;                                                                                        // Raw Frequency in 0.1Hz
  _data.powerFactor = (static_cast<uint32_t>(_buffer[19]) << 8 | static_cast<uint32_t>(_buffer[20])) * 0.01f;                                                                                     // Raw pf in 0.01

  // calculate remaining metrics

  // S = P / PF
  _data.apparentPower = _data.powerFactor == 0 ? 0 : std::abs(_data.activePower / _data.powerFactor);
  // Q = std::sqrt(S^2 - P^2)
  _data.reactivePower = std::sqrt(_data.apparentPower * _data.apparentPower - _data.activePower * _data.activePower);

  _time = millis();

  if (_callback) {
    _callback(EventType::EVT_READ, _data);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// resetEnergy
///////////////////////////////////////////////////////////////////////////////

bool Mycila::PZEM::resetEnergy(uint8_t address) {
  if (!_enabled)
    return false;

  LOGD(TAG, "resetEnergy(0x%02X)", address);

  std::lock_guard<std::mutex> lock(_mutex);

  _buffer[0] = address;
  _buffer[1] = PZEM_CMD_REST;
  _crcSet(_buffer, 4);

  _serial->write(_buffer, 4);
  _serial->flush();

  ReadResult result = _timedRead(address, PZEM_RESPONSE_SIZE_RESET);

  return result == ReadResult::READ_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// settings
///////////////////////////////////////////////////////////////////////////////

bool Mycila::PZEM::setDeviceAddress(const uint8_t address, const uint8_t newAddress) {
  if (!_enabled)
    return false;

  if (newAddress < 0x01 || newAddress > 0xF7) {
    LOGE(TAG, "Invalid address: 0x%02X", newAddress);
    return false;
  }

  std::lock_guard<std::mutex> lock(_mutex);

  LOGD(TAG, "setDeviceAddress(0x%02X, 0x%02X)", address, newAddress);

  _send(address, PZEM_CMD_WSR, PZEM_REGISTER_ADDRESS, newAddress);
  ReadResult result = _timedRead(address, PZEM_RESPONSE_SIZE_SET_ADDR);

  if (result != ReadResult::READ_SUCCESS && result != ReadResult::READ_ERROR_ADDRESS) {
    LOGD(TAG, "Unable to set address @ 0x%02X", address);
    return false;
  }

  if (!_canRead(newAddress)) {
    LOGE(TAG, "Unable to read PZEM @ 0x%02X", newAddress);
    return false;
  }

  // update destination address if needed
  if (_address != MYCILA_PZEM_ADDRESS_GENERAL && _address == address) {
    _address = newAddress;
  }

  return true;
}

uint8_t Mycila::PZEM::readDeviceAddress(bool update) {
  if (!_enabled)
    return false;

  std::lock_guard<std::mutex> lock(_mutex);

  uint8_t address = MYCILA_PZEM_ADDRESS_UNKNOWN;

  _send(MYCILA_PZEM_ADDRESS_GENERAL, PZEM_CMD_RHR, PZEM_REGISTER_ADDRESS, 1);
  ReadResult result = _timedRead(MYCILA_PZEM_ADDRESS_GENERAL, PZEM_RESPONSE_SIZE_READ_ADDR);

  if (result == ReadResult::READ_SUCCESS) {
    address = _buffer[4];
    if (update) {
      _address = address;
    }
  } else {
    LOGD(TAG, "Unable to read address @ 0x%02X", MYCILA_PZEM_ADDRESS_GENERAL);
  }

  return address;
}

size_t Mycila::PZEM::search(uint8_t* addresses, const size_t maxCount) {
  if (!_enabled)
    return 0;

  if (maxCount == 0)
    return 0;

  std::lock_guard<std::mutex> lock(_mutex);

  size_t count = 0;

  for (uint8_t address = MYCILA_PZEM_ADDRESS_MIN; address <= MYCILA_PZEM_ADDRESS_MAX && count < maxCount; address++) {
    _send(address, PZEM_CMD_RHR, PZEM_REGISTER_ADDRESS, 1);
    if (_timedRead(address, PZEM_RESPONSE_SIZE_READ_ADDR) == ReadResult::READ_SUCCESS) {
      addresses[count++] = address;
      LOGD(TAG, "Found PZEM @ 0x%02X", address);
    }
    yield();
  }

  return count;
}

///////////////////////////////////////////////////////////////////////////////
// toJson
///////////////////////////////////////////////////////////////////////////////

#ifdef MYCILA_JSON_SUPPORT
void Mycila::PZEM::toJson(const JsonObject& root) const {
  root["enabled"] = _enabled;
  root["address"] = _address;
  root["time"] = _time;
  _data.toJson(root);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// I/O
///////////////////////////////////////////////////////////////////////////////

Mycila::PZEM::ReadResult Mycila::PZEM::_timedRead(uint8_t expectedAddress, size_t expectedLen) {
  size_t count = 0;
  while (count < expectedLen) {
    size_t read = _serial->readBytes(_buffer + count, expectedLen - count);
    if (read) {
      count += read;
    } else {
      break;
    }
  }

#ifdef MYCILA_PZEM_DEBUG
  Serial.printf("[PZEM] timedRead() %d < ", count);
  for (size_t i = 0; i < count; i++) {
    Serial.printf("0x%02X ", _buffer[i]);
  }
  Serial.println();
#endif

  _drop();

  // timeout ?
  if (count == 0) {
    // LOGD(TAG, "timedRead() timeout");
    return ReadResult::READ_TIMEOUT;
  }

  // check length
  if (count != expectedLen) {
    LOGD(TAG, "timedRead() error: len %d != %d", count, expectedLen);
    return ReadResult::READ_ERROR_COUNT;
  }

  // CRC check
  if (!_crcCheck(_buffer, count)) {
    LOGD(TAG, "timedRead() error: bad CRC");
    return ReadResult::READ_ERROR_CRC;
  }

  _lastAddress = _buffer[PZEM_RESPONSE_ADDRESS];

  // address check
  if (expectedAddress != MYCILA_PZEM_ADDRESS_GENERAL && expectedAddress != _lastAddress) {
    LOGD(TAG, "timedRead(0x%02X) error: wrong device address 0x%02X", expectedAddress, _lastAddress);
    return ReadResult::READ_ERROR_ADDRESS;
  }

  return ReadResult::READ_SUCCESS;
}

bool Mycila::PZEM::_canRead(uint8_t address) {
#ifdef MYCILA_PZEM_DEBUG
  Serial.printf("[PZEM] _canRead(0x%02X)\n", address);
#endif
  _send(address, PZEM_CMD_RHR, PZEM_REGISTER_ADDRESS, 1);
  return _timedRead(address, PZEM_RESPONSE_SIZE_READ_ADDR) == ReadResult::READ_SUCCESS;
}

void Mycila::PZEM::_send(uint8_t address, uint8_t cmd, uint16_t rAddr, uint16_t val) {
  _buffer[0] = address; // Set slave address
  _buffer[1] = cmd;     // Set command

  _buffer[2] = (rAddr >> 8) & 0xFF; // Set high byte of register address
  _buffer[3] = (rAddr) & 0xFF;      // Set low byte =//=

  _buffer[4] = (val >> 8) & 0xFF; // Set high byte of register value
  _buffer[5] = (val) & 0xFF;      // Set low byte =//=

  _crcSet(_buffer, 8); // Set CRC of frame

#ifdef MYCILA_PZEM_DEBUG
  Serial.printf("[PZEM] send(0x%02X) %d > ", address, 8);
  for (size_t i = 0; i < 8; i++) {
    Serial.printf("0x%02X ", _buffer[i]);
  }
  Serial.println();
#endif

  _serial->write(_buffer, 8); // send frame
  _serial->flush();
}

size_t Mycila::PZEM::_drop() {
  size_t count = 0;
  if (_serial->available()) {
#ifdef MYCILA_PZEM_DEBUG
    Serial.printf("[PZEM] drop < ");
#endif
    while (_serial->available()) {
#ifdef MYCILA_PZEM_DEBUG
      Serial.printf("0x%02X ", _serial->read());
#else
      _serial->read();
#endif
      count++;
    }
#ifdef MYCILA_PZEM_DEBUG
    Serial.println();
#endif
  }
  return count;
}

void Mycila::PZEM::_openSerial(const uint8_t rxPin, const uint8_t txPin) {
  if (_sharedSerial && _serialUsers++)
    return;
  LOGD(TAG, "openSerial()");
  _serial->begin(PZEM_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
  _serial->setTimeout(PZEM_TIMEOUT);
  while (!_serial)
    yield();
  while (!_serial->availableForWrite())
    yield();
  _drop();
  _serial->flush(false);
}

///////////////////////////////////////////////////////////////////////////////
// CRC
///////////////////////////////////////////////////////////////////////////////

void Mycila::PZEM::_crcSet(uint8_t* buf, uint16_t len) {
  if (len <= 2)
    return;
  uint16_t crc = _crc16(buf, len - 2);
  buf[len - 2] = crc & 0xFF;        // Low byte first
  buf[len - 1] = (crc >> 8) & 0xFF; // High byte second
}

bool Mycila::PZEM::_crcCheck(const uint8_t* buf, uint16_t len) {
  if (len <= 2)
    return false;
  uint16_t crc = _crc16(buf, len - 2); // Compute CRC of data
  return (static_cast<uint16_t>(buf[len - 2]) | static_cast<uint16_t>(buf[len - 1]) << 8) == crc;
}

uint16_t Mycila::PZEM::_crc16(const uint8_t* data, uint16_t len) {
  uint8_t nTemp;         // CRC table index
  uint16_t crc = 0xFFFF; // Default value
  while (len--) {
    nTemp = *data++ ^ crc;
    crc >>= 8;
    crc ^= static_cast<uint16_t>(pgm_read_word(&crcTable[nTemp]));
  }
  return crc;
}

///////////////////////////////////////////////////////////////////////////////
// static
///////////////////////////////////////////////////////////////////////////////

#if MYCILA_PZEM_ASYNC_MAX_INSTANCES > 0
bool Mycila::PZEM::_add(PZEM* pzem) {
  for (size_t i = 0; i < MYCILA_PZEM_ASYNC_MAX_INSTANCES; i++) {
    if (_instances[i] == nullptr) {
      LOGD(TAG, "Adding instance at address 0x%02X to async task...", pzem->_address);

      if (_taskHandle == NULL) {
        pzem->_openSerial(pzem->_pinRX, pzem->_pinTX);

        if (!pzem->_canRead(pzem->_address)) {
          LOGW(TAG, "Unable to read at address 0x%02X. Please verify that the device is powered and that its address is correctly set.", pzem->_address);
        }

        _instances[i] = pzem;

        LOGD(TAG, "Creating async task 'pzemTask'...");
        if (xTaskCreateUniversal(_pzemTask, "pzemTask", MYCILA_PZEM_ASYNC_STACK_SIZE, nullptr, MYCILA_PZEM_ASYNC_PRIORITY, &_taskHandle, MYCILA_PZEM_ASYNC_CORE) == pdPASS) {
          return true;
        } else {
          _instances[i] = nullptr;
          LOGE(TAG, "Failed to create task 'pzemTask'");
          return false;
        }

      } else {
        if (!pzem->_canRead(pzem->_address)) {
          LOGW(TAG, "Unable to read at address 0x%02X. Please verify that the device is powered and that its address is correctly set.", pzem->_address);
        }
        _instances[i] = pzem;
      }

      return true;
    }
  }
  return false;
}

void Mycila::PZEM::_remove(PZEM* pzem) {
  // check for remaining tasks
  bool remaining = false;
  for (size_t i = 0; i < MYCILA_PZEM_ASYNC_MAX_INSTANCES; i++) {
    if (_instances[i] != nullptr && _instances[i] != pzem) {
      remaining = true;
      break;
    }
  }
  // first delete  the task if no remaining instances
  if (!remaining && _taskHandle != NULL) {
    LOGD(TAG, "Deleting async task 'pzemTask'...");
    vTaskDelete(_taskHandle);
    _taskHandle = NULL;
  }
  // in any case, remove the instance from the list
  for (size_t i = 0; i < MYCILA_PZEM_ASYNC_MAX_INSTANCES; i++) {
    if (_instances[i] == pzem) {
      _instances[i] = nullptr;
      break;
    }
  }
}

void Mycila::PZEM::_pzemTask(void* params) {
  while (true) {
    bool read = false;
    for (size_t i = 0; i < MYCILA_PZEM_ASYNC_MAX_INSTANCES; i++) {
      if (_instances[i] != nullptr) {
        read |= _instances[i]->read();
        yield();
      }
    }
    if (!read)
      delay(10);
  }
  _taskHandle = NULL;
  vTaskDelete(NULL);
}
#endif
