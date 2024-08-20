#include "ModbusManager.h"

extern xSemaphoreHandle xSemaphore;
extern Modbus::ResultCode err;

const uint16_t NUM_COILS = 6;
const uint16_t NUM_HOLDREGS_SETTING = 20;
const uint16_t NUM_HOLDREGS_DATA = 11 * 5;
const uint16_t NUM_HOLDREGS = NUM_HOLDREGS_SETTING + NUM_HOLDREGS_DATA;
const uint16_t NUM_IREGS = 4;

uint16_t COIL_START_ADDRESS = 100;
uint16_t IREG_START_ADDRESS = 200;
uint16_t HREG_START_ADDRESS_SETTING = 1000;
uint16_t HREG_START_ADDRESS_DATA
    = HREG_START_ADDRESS_SETTING + NUM_HOLDREGS_SETTING;

uint16_t coilAddresses[NUM_COILS] = {};
uint16_t hregAddresses[NUM_HOLDREGS] = {};
uint16_t iregAddresses[NUM_IREGS] = {};

uint8_t coilPins[NUM_COILS] = {
    UPS_POWER_CUT_PIN,  // coilpin_2 -> C2
    LOAD_ON_PIN,        // coilpin_3 -> C3
    LOAD25P_ON_PIN,     // coilpin_4 -> C4
    LOAD50P_ON_PIN,     // coilpin_5 -> C5
    LOAD75P_ON_PIN,     // coilpin_6 -> C6
    LOAD_FULL_ON_PIN    // coilpin_7 -> C7
};

uint16_t coilValues[NUM_COILS] = {0x0000};

uint16_t cbCoilRead(TRegister* reg, uint16_t val) { return val; }

uint16_t cbCoilWrite(TRegister* reg, uint16_t val) {

  uint16_t address = reg->address.address;
  if (address < COIL_START_ADDRESS)
    return 0;
  uint8_t offset = address - COIL_START_ADDRESS;  // Calculate the index
  if (offset < NUM_COILS) {
    // Use mutex to protect coil write operations
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    coilValues[offset] = val;  // Update the coil value
    digitalWrite(coilPins[offset], COIL_BOOL(val));
    xSemaphoreGive(xSemaphore);
  }

  return val;
}

uint16_t cbHregSet(TRegister* reg, uint16_t val) { return val; }

// Callback function for holding register get
uint16_t cbHregGet(TRegister* reg, uint16_t val) {
  uint16_t address = reg->address.address;

  if (address >= HREG_START_ADDRESS_DATA) {
    // Handling data registers
    int index = address - HREG_START_ADDRESS_DATA;
    if (index >= 0 && index < NUM_HOLDREGS_DATA) {
      if (switchTest) {
        SwitchTestData& testData = switchTest->data();
        int switchTestIndex = index / 11;  // 11 registers per test
        int subIndex = index % 11;
        if (switchTestIndex < 5) {  // Ensure index is within bounds
          SwitchTestData::TestData& test = testData.switchTest[switchTestIndex];
          switch (subIndex) {
            case 0:
              val = test.testNo;
              break;
            case 1:
              val = (test.testTimestamp >> 16) & 0xFFFF;
              break;
            case 2:
              val = test.testTimestamp & 0xFFFF;
              break;
            case 3:
              val = (test.switchtime >> 16) & 0xFFFF;
              break;
            case 4:
              val = test.switchtime & 0xFFFF;
              break;
            case 5:
              val = (test.starttime >> 16) & 0xFFFF;
              break;
            case 6:
              val = test.starttime & 0xFFFF;
              break;
            case 7:
              val = (test.endtime >> 16) & 0xFFFF;
              break;
            case 8:
              val = test.endtime & 0xFFFF;
              break;
            case 9:
              val = test.load_percentage;
              break;
            case 10:
              val = test.valid_data ? 1 : 0;
              break;
            default:
              val = 0;
              break;  // Default value if subIndex is out of bounds
          }
        }
      }
    }
  } else if (address >= HREG_START_ADDRESS_SETTING
             && address < HREG_START_ADDRESS_DATA) {
    // Handling settings registers
    int index = address - HREG_START_ADDRESS_SETTING;
    if (index >= 0 && index < NUM_HOLDREGS_SETTING) {
      // Return the current value without modification
    }
  }
  return val;
}

void modbusRTU_Init() {

  for (uint16_t i = 0; i < NUM_COILS; ++i) {

    coilAddresses[i] = COIL_START_ADDRESS + i;
  }

  mb.addCoil(COIL_START_ADDRESS, false, NUM_COILS);
  mb.onSetCoil(COIL_START_ADDRESS, cbCoilWrite, NUM_COILS);
  mb.onGetCoil(COIL_START_ADDRESS, cbCoilRead, NUM_COILS);

  for (uint16_t i = 0; i < NUM_HOLDREGS; ++i) {
    hregAddresses[i] = HREG_START_ADDRESS_SETTING + i;
    mb.addHreg(hregAddresses[i]);
    mb.onSetHreg(hregAddresses[i], cbHregSet);
    mb.onGetHreg(hregAddresses[i], cbHregGet);
  }
  // Initialize input registers if needed
  for (uint16_t i = 0; i < NUM_IREGS; ++i) {
    iregAddresses[i] = IREG_START_ADDRESS + i;
    mb.addIreg(iregAddresses[i]);
  }
}

void updateModbusRTU() {
  // update coils

  for (int i = 0; i < NUM_COILS; i++) {
    digitalWrite(coilPins[i], coilValues[i] == 0XFF00 ? HIGH : LOW);
  }
  Serial.println("Printing mains power level...");
  Serial.println(digitalRead(SENSE_MAINS_POWER_PIN));
}