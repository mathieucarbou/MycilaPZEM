# MycilaPZEM

[![Latest Release](https://img.shields.io/github/release/mathieucarbou/MycilaPZEM.svg)](https://GitHub.com/mathieucarbou/MycilaPZEM/releases/)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/MycilaPZEM.svg)](https://registry.platformio.org/libraries/mathieucarbou/MycilaPZEM)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)

[![Build](https://github.com/mathieucarbou/MycilaPZEM/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/MycilaPZEM/actions/workflows/ci.yml)
[![GitHub latest commit](https://badgen.net/github/last-commit/mathieucarbou/MycilaPZEM)](https://GitHub.com/mathieucarbou/MycilaPZEM/commit/)

Arduino / ESP32 library for the PZEM-004T power and energy monitor (v3 and v4).

Note on “v4”: Some sellers now market a PZEM-004T “v4”. In practice, this is the same device as v3 with updated electronic components; the protocol and behavior are identical. This library fully supports both v3 and v4.

## Highlights

- ESP32-focused, robust, and fast
- Blocking and non-blocking (async) modes
- Multiple PZEM-004T v3/v4 devices on the same RX/TX port
- Auto-detect devices and manage device addresses
- Live energy reset in both modes
- Configurable core, stack size, task priority, and read timeout
- Callback mechanism for data change events

## Table of contents

- [About the hardware and compatibility](#about-the-hardware-and-compatibility)
- [Installation](#installation)
- [Quick start](#quick-start)
- [API overview (metrics and helpers)](#api-overview-metrics-and-helpers)
- [Wiring](#wiring)
- [Using multiple devices on one UART](#using-multiple-devices-on-one-uart)
- [Callbacks and async mode](#callbacks-and-async-mode)
  - [Callback example](#callback-example)
- [Energy reset](#energy-reset)
- [Read and set address](#read-and-set-address)
- [Start a PZEM with a specific address](#start-a-pzem-with-a-specific-address)
- [JSON support (optional)](#json-support-optional)
- [Performance notes](#performance-notes)
- [Troubleshooting](#troubleshooting)
- [Reference material](#reference-material)
- [Examples](#examples)
- [License and conduct](#license-and-conduct)

## About the hardware and compatibility

- Supported modules: PZEM-004T v3.0 and v4.0 (same protocol/registers)
- Supported MCUs: ESP32 (Library targets ESP32 specifically)
- UART levels: Most ESP32 UART pins are 3.3V. PZEM-004T v3 typically works with 3.3V UART signals, but a level shifter is recommended for strict compatibility. Power the PZEM as specified by the vendor.

If your unit is labeled or sold as “v4”, treat it as a v3 for wiring, protocol, and features. No special configuration is required.

## Installation

PlatformIO (recommended)

- Add the dependency in your `platformio.ini`:
  - `lib_deps = mathieucarbou/MycilaPZEM`

Arduino IDE

- Install manually by copying this repository into your `libraries` folder, or use the ZIP library install feature.

Optional JSON support

- Define `-D MYCILA_JSON_SUPPORT` in your build flags and add ArduinoJson to your project.

## Quick start

Recommended usage: non-blocking (async) with a callback

```c++
Mycila::PZEM pzem;

void setup() {
  pzem.begin(Serial1, 14, 27, MYCILA_PZEM_DEFAULT_ADDRESS, true); // async
  // React to fresh values as soon as they are read
  pzem.setCallback([](const Mycila::PZEM::EventType evt, const Mycila::PZEM::Data &data){
    if (evt == Mycila::PZEM::EventType::EVT_READ) {
      // Example: print a few metrics
      Serial.printf("V=%.2fV I=%.3fA P=%.1fW PF=%.2f\n", data.voltage, data.current, data.activePower, data.powerFactor);
    }
  });
}

void loop() {
  // no need to call read() in async mode
}
```

## API overview (metrics and helpers)

The `Data` structure exposed by this library contains the following metrics and helpers:

```c++
/** Frequency in hertz (Hz). */
float frequency = NAN; // Hz

/** Voltage in volts (V). */
float voltage = NAN;

/** Current in amperes (A). */
float current = NAN;

/** Active power in watts (W). */
float activePower = NAN;

/** Power factor. */
float powerFactor = NAN;

/** Apparent power in volt-amperes (VA). */
float apparentPower = NAN;

/** Reactive power in volt-amperes reactive (VAr). */
float reactivePower = NAN;

/** Active energy in watt-hours (Wh). */
uint32_t activeEnergy = 0;

/** Compute THDi (assumes THDu = 0). */
float thdi(float phi = 0) const;

/** Compute load resistance (ohms). */
float resistance() const;

/** Compute dimmed voltage (V = P / I). */
float dimmedVoltage() const;

/** Compute nominal power (P = V^2 / R). */
float nominalPower() const;
```

## Wiring

Typical ESP32 ↔ PZEM-004T wiring:

- ESP32 GND ↔ PZEM GND
- ESP32 RX (e.g., GPIO 14) ↔ PZEM TX
- ESP32 TX (e.g., GPIO 27) ↔ PZEM RX
- PZEM VCC: power as specified by your board (commonly 5V). Do not power from 3.3V unless your module allows it.
- Current transformer (CT) clamp: place around the live wire only, with orientation arrow pointing toward the load.

Notes:

- UART levels: While many PZEM-004T v3/v4 modules accept 3.3V UART, using a level shifter between ESP32 (3.3V) and PZEM (5V side) is recommended for robustness.
- Use a dedicated, stable power supply for the PZEM. Brown-outs can cause read errors or address loss.
- On ESP32, `Serial1` default pins vary by board. Always pass explicit RX/TX pins in `begin()`.

Example pin mapping used in examples: RX=14, TX=27

## Using multiple devices on one UART

This library supports multiple PZEM-004T devices on the same RX/TX port using PZEM addresses.

Basic steps:

1. Connect the first PZEM-004T and set its address (use `examples/SetAddress/SetAddress.ino`).
2. Disconnect it, connect the second one, and set a different address.
3. Connect both devices to the same RX/TX port; read each by its address.

## Callbacks and async mode

- `PZEM::Callback` is invoked when new data is read and a metric changed.
- Use it to react instantly and avoid polling delays.

See `examples/Callback` and `examples/CallbackAsync`.

## Energy reset

```c++
pzem.resetEnergy();
```

## Read and set address

```c++
pzem.readDeviceAddress();
pzem.readDeviceAddress(true); // will read the address and use it as the new address

pzem.setDeviceAddress(0x42);
```

## Start a PZEM with a specific address

```c++
pzem.begin(Serial1, 14, 27, address);
```

## JSON support (optional)

JSON is optional; the recommended way is async + callbacks. If you still need JSON serialization, enable:
`-D MYCILA_JSON_SUPPORT` and include ArduinoJson:

```c++
#include <ArduinoJson.h>
```

### Callback example

Reading a load for ~2 seconds after it is turned on:

```c++
  static Mycila::PZEM::Data prevData;
  pzem.setCallback([](const Mycila::PZEM::EventType eventType, const Mycila::PZEM::Data& data) {
    int64_t now = esp_timer_get_time();
    switch (eventType) {
      case Mycila::PZEM::EventType::EVT_READ:
        Serial.printf(" - %" PRId64 " EVT_READ\n", now);
        if (data != prevData) {
          Serial.printf(" - %" PRIu32 " EVT_CHANGE: %f V, %f A, %f W\n", millis(), data.voltage, data.current, data.activePower);
          prevData = data;
        }
        break;
      default:
        Serial.printf(" - %" PRId64 " ERR\n", now);
        break;
    }
  });
```

```text
 - 227 EVT_CHANGE: 236.199997 V, 0.000000 A, 0.000000 W
 - 287749 EVT_READ
 - 347736 EVT_READ
 - 407822 EVT_READ
 - 468011 EVT_READ
 - 527993 EVT_READ
 - 588081 EVT_READ
 - 648269 EVT_READ
 - 708261 EVT_READ
 - 768343 EVT_READ
 - 828434 EVT_READ
 - 888633 EVT_READ
 - 948717 EVT_READ
 - 1008799 EVT_READ
 - 1068889 EVT_READ
 - 1128980 EVT_READ
 - 1189069 EVT_READ
 - 1249167 EVT_READ
 - 1249 EVT_CHANGE: 233.399994 V, 2.262000 A, 497.899994 W
 - 1309258 EVT_READ
 - 1369438 EVT_READ
 - 1434762 EVT_READ
 - 1494647 EVT_READ
 - 1554730 EVT_READ
 - 1614816 EVT_READ
 - 1674910 EVT_READ
 - 1674 EVT_CHANGE: 233.399994 V, 2.262000 A, 497.899994 W
 - 1734995 EVT_READ
 - 1795086 EVT_READ
 - 1855168 EVT_READ
 - 1915365 EVT_READ
 - 1975447 EVT_READ
 - 2035538 EVT_READ
 - 2095633 EVT_READ
 - 2155718 EVT_READ
 - 2215800 EVT_READ
 - 2275891 EVT_READ
 - 2335982 EVT_READ
 - 2396071 EVT_READ
 - 2461382 EVT_READ
 - 2521268 EVT_READ
 - 2581354 EVT_READ
 - 2641551 EVT_READ
 - 2701644 EVT_READ
 - 2701 EVT_CHANGE: 233.899994 V, 2.261000 A, 539.500000 W
 - 2761619 EVT_READ
 - 2821705 EVT_READ
 - 2881902 EVT_READ
 - 2941987 EVT_READ
 - 3002075 EVT_READ
 - 3062161 EVT_READ
 - 3122255 EVT_READ
 - 3182440 EVT_READ
 - 3242428 EVT_READ
 - 3302514 EVT_READ
 - 3362608 EVT_READ
 - 3422690 EVT_READ
 - 3488113 EVT_READ
 - 3547891 EVT_READ
 - 3608088 EVT_READ
 - 3668170 EVT_READ
 - 3728260 EVT_READ
 - 3788346 EVT_READ
 - 3848441 EVT_READ
 - 3908634 EVT_READ
 - 3908 EVT_CHANGE: 236.600006 V, 0.000000 A, 0.000000 W
```

## Performance notes

Here are below some test results for the PZEM for 50 consecutive reads on a nominal load of about 650W, controlled with a random SSR relay (0-100%).

### PerfTest1

```text
pzem.read():
 - Errors: 0
 - Average read time: 120433 us
 - Min read time: 107118 us
 - Max read time: 126020 us
```

### PerfTest2

```text
pzem.read():
 - ROUND: 1
   * Load Detection time: 1207002 us (10 reads)
   * Ramp up time: 2534171 us (21 reads)
   * Ramp down time: 2537724 us (21 reads)
 - ROUND: 2
   * Load Detection time: 1328073 us (11 reads)
   * Ramp up time: 2655242 us (22 reads)
   * Ramp down time: 2539094 us (21 reads)
 - ROUND: 3
   * Load Detection time: 1206870 us (10 reads)
   * Ramp up time: 2534040 us (21 reads)
   * Ramp down time: 2537968 us (21 reads)
 - ROUND: 4
   * Load Detection time: 1328062 us (11 reads)
   * Ramp up time: 2655334 us (22 reads)
   * Ramp down time: 2538888 us (21 reads)
```

The "Load Detection time" is the time it takes for the PZEM to see a load `> 0` just after the relay was closed on a constant resistive load.

The "Ramp up time" is the time it takes for the PZEM to stabilize (with a maximum delta of 1% from last measurement) and see an expected load value.

The "Ramp down time" is the time it takes for the PZEM to return to 0W after the relay was opened.

You can run the `PerfTests` from the examples.
The numbers might change a little but the order of magnitude should stay the same.

**What these results mean ?**

- It takes **more than 1.2 second** for the PZEM to report a change of a load appearing on the wire.
  Reading the PZEM more frequently (in a async loop) and faster will allow a change to be seen as fast as possible, but it will still take at least 1 second for the change to be seen.
  **The best reactivity is achieved when reading the PZEM in a loop, and using callback mechanism.**

- It takes **more than 2.5 seconds** for the PZEM to report a load value which is stable and according to what we expect.
  This duration contains the duration for the load to reach its nominal power, plus the duration it takes for the PZEM to stabilize its measurements.

The PZEM likely applies a moving-average-like window of ~2 seconds with updates every 1 second.

## Troubleshooting

No data / timeouts

- Verify RX/TX cross-over (ESP32 RX ↔ PZEM TX and ESP32 TX ↔ PZEM RX)
- Confirm ground is shared (GND ↔ GND)
- Ensure correct pins given to `begin(Serial1, rxPin, txPin, ...)`
- Try a slower loop and avoid frequent re-open/close of Serial

CRC errors / noise

- Shorten UART wires, twist RX/TX with GND where possible
- Add a common ground reference; avoid star grounds with noisy loads
- Provide stable power to PZEM; avoid powering from weak USB ports

Address issues

- New/unknown device? Use address `MYCILA_PZEM_ADDRESS_GENERAL (0xF8)` or run `search()`
- Use `examples/SetAddress` to set unique addresses before multi-drop wiring
- Read back with `readDeviceAddress(true)` to confirm

Multiple devices on one UART

- Assign unique addresses first
- If you share one HardwareSerial across instances, call `setSharedSerial(true)` on each
- Poll devices in a loop or use async with callbacks for responsiveness

CT wiring pitfalls

- Clamp only the live conductor, not both L and N together
- Observe the arrow direction; reverse if negative readings appear
- Ensure the clamp is fully closed and seated

## Reference material

- [PZEM-004T-V3.0-Datasheet-User-Manual.pdf](https://mathieu.carbou.me/MycilaPZEM/PZEM-004T-V3.0-Datasheet-User-Manual.pdf)
- [PZEM-004T V3](https://innovatorsguru.com/pzem-004t-v3/)
- [mandulaj/PZEM-004T-v30](https://github.com/mandulaj/PZEM-004T-v30)
- [Peacefair PZEM-004T v3.0 Open CT + USB 100A](https://aliexpress.com/item/33045826345.html) - [manufacturer page](https://en.peacefair.cn/product/769.html)
- [Peacefair PZEM-004T v4.0 Open CT + USB 100A](https://aliexpress.com/item/1005009611591855.html)  - [manufacturer page](https://en.peacefair.cn/product/772.html) **recommended**

## Examples

Browse the `examples/` folder for ready-to-run sketches:

- `Read` and `ReadAsync`: single-device blocking vs async
- `ReadMultiAsync`: multiple devices on one UART
- `Callback` and `CallbackAsync`: event-driven usage
- `SetAddress`: assign a specific PZEM address
- `EnergyReset`: reset accumulated energy
- `PerfTest1` and `PerfTest2`: performance measurements

## License and conduct

- License: MIT
- Contributor Covenant: see `CODE_OF_CONDUCT.md`
