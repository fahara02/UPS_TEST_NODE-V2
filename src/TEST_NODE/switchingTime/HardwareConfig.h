#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H
#include <stdint.h>

#define RX_RS485_PIN 16
#define TX_RS485_PIN 17

#define thermoSO_PIN 19   // VSPI MISO PIN
#define thermoSCK_PIN 18  // VSPI_CLK
#define thermoCS_PIN 5    // VSPI CS

#define SENSE_MAINS_POWER_PIN 23
#define SENSE_UPS_POWER_PIN 22
#define SENSE_UPS_POWER_DOWN 21

#define UPS_POWER_CUT_PIN 32
#define LOAD_ON_PIN 33
#define LOAD25P_ON_PIN 25
#define LOAD50P_ON_PIN 26
#define LOAD75P_ON_PIN 27
#define LOAD_FULL_ON_PIN 14
#define TEST_END_INT_PIN 12
#define LOAD_PWM_PIN 13

const uint16_t maxVARating = 4000;

struct switchTestConfig {
  uint8_t max_retest = 3;
  unsigned long min_valid_switch_time_ms = 0;
  unsigned long max_valid_switch_time_ms = 10000;
  unsigned long debounceDelay = 100;
};

#endif