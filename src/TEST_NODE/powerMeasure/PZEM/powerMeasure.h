#ifndef POWER_MEASURE_H
#define POWER_MEASURE_H
#include <stdint.h>

struct powerMeasure {
  uint16_t voltage;
  uint16_t frequency;
  uint16_t pf;
  uint16_t alarms;
  uint32_t current;
  uint32_t power;
  uint32_t energy;
  bool isValid;
  unsigned long last_measured_ms;
  powerMeasure()
      : voltage(0),
        frequency(0),
        pf(0),
        alarms(0),
        current(0),
        power(0),
        energy(0),
        isValid(false),
        last_measured_ms(0) {}
};

struct powerDevice {

  uint8_t command;
  uint8_t addr[4];
  uint8_t data;
  uint8_t crc;
};

#endif