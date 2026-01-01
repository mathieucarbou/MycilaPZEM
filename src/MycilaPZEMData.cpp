// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#include "MycilaPZEM.h"

static constexpr float DEG_TO_RAD_F = static_cast<float>(3.14159265358979323846 / 180.0);

float Mycila::PZEM::Data::thdi(float phi) const {
  if (powerFactor == 0)
    return NAN;
  const float cosPhi = std::cos(phi * DEG_TO_RAD_F);
  return 100.0f * std::sqrt((cosPhi * cosPhi) / (powerFactor * powerFactor) - 1);
}

float Mycila::PZEM::Data::resistance() const { return current == 0 ? NAN : std::abs(activePower / (current * current)); }

float Mycila::PZEM::Data::dimmedVoltage() const { return current == 0 ? NAN : std::abs(activePower / current); }

float Mycila::PZEM::Data::nominalPower() const { return activePower == 0 ? NAN : std::abs(voltage * voltage * current * current / activePower); }

void Mycila::PZEM::Data::clear() {
  frequency = NAN;
  voltage = NAN;
  current = NAN;
  activePower = NAN;
  reactivePower = NAN;
  apparentPower = NAN;
  powerFactor = NAN;
  activeEnergy = 0;
}

bool Mycila::PZEM::Data::operator==(const Mycila::PZEM::Data& other) const {
  return (std::isnan(frequency) ? std::isnan(other.frequency) : frequency == other.frequency) &&
         (std::isnan(voltage) ? std::isnan(other.voltage) : voltage == other.voltage) &&
         (std::isnan(current) ? std::isnan(other.current) : current == other.current) &&
         (std::isnan(activePower) ? std::isnan(other.activePower) : activePower == other.activePower) &&
         (std::isnan(reactivePower) ? std::isnan(other.reactivePower) : reactivePower == other.reactivePower) &&
         (std::isnan(apparentPower) ? std::isnan(other.apparentPower) : apparentPower == other.apparentPower) &&
         (std::isnan(powerFactor) ? std::isnan(other.powerFactor) : powerFactor == other.powerFactor) &&
         (activeEnergy == other.activeEnergy);
}

void Mycila::PZEM::Data::operator=(const Mycila::PZEM::Data& other) {
  frequency = other.frequency;
  voltage = other.voltage;
  current = other.current;
  activePower = other.activePower;
  reactivePower = other.reactivePower;
  apparentPower = other.apparentPower;
  powerFactor = other.powerFactor;
  activeEnergy = other.activeEnergy;
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::PZEM::Data::toJson(const JsonObject& root) const {
  if (!std::isnan(frequency))
    root["frequency"] = frequency;
  if (!std::isnan(voltage))
    root["voltage"] = voltage;
  if (!std::isnan(current))
    root["current"] = current;
  if (!std::isnan(activePower))
    root["active_power"] = activePower;
  if (!std::isnan(reactivePower))
    root["reactive_power"] = reactivePower;
  if (!std::isnan(apparentPower))
    root["apparent_power"] = apparentPower;
  if (!std::isnan(powerFactor))
    root["power_factor"] = powerFactor;
  root["active_energy"] = activeEnergy;
  float r = resistance();
  float d = dimmedVoltage();
  float n = nominalPower();
  float t = thdi();
  if (!std::isnan(r))
    root["resistance"] = r;
  if (!std::isnan(d))
    root["dimmed_voltage"] = d;
  if (!std::isnan(n))
    root["nominal_power"] = n;
  if (!std::isnan(t))
    root["thdi_0"] = t;
}
#endif
