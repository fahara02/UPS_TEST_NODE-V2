#ifndef PZEM_H
#define PZEM_H
#include "powerMeasure.h"
#include <Arduino.h>
#include <cstdint>

#if defined(PZEM004_SOFTSERIAL)
#include <SoftwareSerial.h>
#endif

#include "PZEM_constants.h"

enum class pMeasureState {
  IDLE,
  SENDING_COMMAND,
  READING_REGISTERS,
  WRITING_REGISTERS,
  CONNECTED,
  DISCONNECTED,
  TIMEOUT,
  ERROR,
  RESET,
  CALIBRATE

};

enum class RegisterType { INPUT_REG, HOLDING_REG, CAL_REG, RESET_REG };
enum class ReceiveStatus {
  SUCCESS,
  TIMEOUT,
  LENGTH_ERROR,
  ABNORMAL_CODE,
  CRC_ERROR,
  BUFFER_OVERFLOW
};
struct ReceivedResult {
  ReceiveStatus status;
  uint16_t bytes_received;
};

class PZEM {
public:
#if defined(PZEM004_SOFTSERIAL)

  PZEM(uint8_t receivePin, uint8_t transmitPin,
       uint8_t addr = PZEM_DEFAULT_ADDR);

  PZEM(SoftwareSerial& port, uint8_t addr = PZEM_DEFAULT_ADDR);
  PZEM(Stream& port, uint8_t addr = PZEM_DEFAULT_ADDR);

#endif
  PZEM(HardwareSerial& port, uint8_t receivePin, uint8_t transmitPin,
       uint8_t addr = PZEM_DEFAULT_ADDR);

  PZEM(HardwareSerial* port, uint8_t receivePin, uint8_t transmitPin,
       uint8_t addr = PZEM_DEFAULT_ADDR)
      : PZEM(*port, receivePin, transmitPin, addr){};
  // Empty constructor for creating arrays
  PZEM(){};

  ~PZEM();

  void init(Stream* port, bool isSoft,
            uint8_t addr);  // Init common to all constructors
  uint16_t readVoltage();
  uint32_t readCurrent();
  uint32_t readPower();
  uint32_t readEnergy();
  uint16_t readFrequency();
  uint16_t readPowerFactor();

  uint16_t getVoltage();
  uint32_t getCurrent();
  uint32_t getPower();
  uint32_t getEnergy();
  uint16_t getFrequency();
  uint16_t getPowerFactor();

  bool getPowerAlarm();
  bool setPowerAlarm(uint16_t watts);

  bool resetEnergy();
  bool setAddress(uint8_t addr);
  bool checkUpdateStatus();
  uint8_t getAddress();
  uint8_t readAddress(bool update = false);

  void search();

private:
  pMeasureState _state;
  uint8_t _addr;    // Device address
  Stream* _serial;  // Serial interface
#if defined(PZEM004_SOFTSERIAL)
  SoftwareSerial* localSWserial
      = nullptr;  // Pointer to the Local SW serial object
#endif
  bool _isSoft;       // Is serial interface software
  bool _isConnected;  // Flag set on successful communication
  unsigned long _readTimeOut = PZEM_DEFAULT_READ_TIMEOUT;
  powerMeasure _currentValues;  // Measured values
  powerDevice _device;
  uint64_t _lastRead;  // Last time values were updated

  bool updateValues();  // Get most up to date values from device registers and
                        // cache them
  ReceivedResult receive(uint8_t* resp,
                         uint16_t len);  // Receive len bytes into a buffer

  bool sendCmd(FunctionCode cmd, uint16_t rAddr, uint16_t value,
               bool check = false,
               uint16_t slave_addr = 0xFFFF);  // Send 8 byte command

  void setCRC(uint8_t* buf, uint16_t len);          // Set the CRC for a buffer
  bool checkCRC(const uint8_t* buf, uint16_t len);  // Check CRC of buffer

  uint16_t CRC16(const uint8_t* data, uint16_t len);  // Calculate CRC of buffer

  uint32_t extract32BitValue(const uint8_t* response, int startIndex);
  uint16_t extract16BitValue(const uint8_t* response, int startIndex);
  powerMeasure extractAllBits(const uint8_t* response);
  uint32_t readSingleReg(uint16_t rAddr, uint8_t rLength,

                         RegisterType regType = RegisterType::INPUT_REG);
  powerMeasure readInputRegs();
};

#endif