#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H
#include "HardwareConfig.h"
#include "ModbusRTU.h"
#include "SwitchTest.h"

extern ModbusRTU mb;
extern SwitchTest* switchTest;

extern const uint16_t NUM_COILS;
extern const uint16_t NUM_HOLDREGS_SETTING;
extern const uint16_t NUM_HOLDREGS_DATA;
extern const uint16_t NUM_HOLDREGS;
extern const uint16_t NUM_IREGS;

extern uint16_t COIL_START_ADDRESS;
extern uint16_t IREG_START_ADDRESS;
extern uint16_t HREG_START_ADDRESS_SETTING;
extern uint16_t HREG_START_ADDRESS_DATA;

extern uint16_t coilAddresses[];
extern uint16_t hregAddresses[];
extern uint16_t iregAddresses[];

// extern int coilPins[];
// extern uint16_t coilValues[];

uint16_t cbCoilWrite(TRegister* reg, uint16_t val);
uint16_t cbCoilRead(TRegister* reg, uint16_t val);

uint16_t cbHregSet(TRegister* reg, uint16_t val);
// Callback function for holding register get
uint16_t cbHregGet(TRegister* reg, uint16_t val);
void modbusRTU_Init();

void updateModbusRTU();

#endif