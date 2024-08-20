#include "PZEM.h"

#include <stdio.h>

#if defined(PZEM004_SOFTSERIAL)

PZEM::PZEM(uint8_t receivePin, uint8_t transmitPin, uint8_t addr) {
  localSWserial = new SoftwareSerial(
      receivePin, transmitPin);  // We will need to clean up in destructor
  localSWserial->begin(PZEM_BAUD_RATE);

  init((Stream *)localSWserial, true, addr);
}

PZEM::PZEM(SoftwareSerial &port, uint8_t addr) : {
  port.begin(PZEM_BAUD_RATE);
  init((Stream *)&port, true, addr);
}
PZEM::PZEM(Stream &port, uint8_t addr) : { init(&port, true, addr); }
#endif
PZEM::PZEM(HardwareSerial &port, uint8_t receivePin, uint8_t transmitPin,
           uint8_t addr) {
  port.begin(PZEM_BAUD_RATE, SERIAL_8N1, receivePin, transmitPin);
  init((Stream *)&port, false, addr);
}
PZEM::~PZEM() {

#if defined(PZEM004_SOFTSERIAL)
  if (this->localSWserial != nullptr) {
    delete this->localSWserial;
  }
#endif
}

void PZEM::init(Stream *port, bool isSoft, uint8_t addr) {
  _addr = (addr >= MIN_SLAVE_ADDR && addr <= MAX_SLAVE_ADDR)
              ? addr
              : PZEM_DEFAULT_ADDR;
  _serial = port;
  _isSoft = isSoft;
  _lastRead = -1;
  _isConnected = false;
}

uint16_t PZEM::readVoltage() { return readSingleReg(Registers::VOLTAGE, 0x01); }

uint32_t PZEM::readCurrent() {
  return readSingleReg(Registers::CURRENT_L, 0x02);
}

uint32_t PZEM::readPower() { return readSingleReg(Registers::POWER_L, 0x02); }

uint32_t PZEM::readEnergy() { return readSingleReg(Registers::ENERGY_L, 0x02); }

uint16_t PZEM::readFrequency() {
  return readSingleReg(Registers::FREQUENCY, 0x01);
}

uint16_t PZEM::readPowerFactor() { return readSingleReg(Registers::PF, 0x01); }

bool PZEM::updateValues() {
  powerMeasure newValues = readInputRegs();
  if (!newValues.isValid) {
    return false;  // Return false if reading input registers fails
  }

  // Update the _currentValues with the new data
  _currentValues = newValues;

  return true;  // Return true if everything is successful
}

bool PZEM::checkUpdateStatus() {
  if (!updateValues()) {
    return 0;
  }

  if ((millis() - _lastRead) > UPDATE_TIME) {
    if (!updateValues()) {
      return 0;  // Update did not work, return 0
    }
  }
  return true;
}

uint16_t PZEM::getVoltage() {

  if (!checkUpdateStatus()) {
    return NAN;
    ;
  }
  return _currentValues.voltage;
}

uint32_t PZEM::getCurrent() {

  if (!checkUpdateStatus()) {
    return NAN;
  }
  return _currentValues.current;
}

uint32_t PZEM::getPower() {

  if (!checkUpdateStatus()) {
    return NAN;
  }
  return _currentValues.power;
}

uint32_t PZEM::getEnergy() {

  if (!checkUpdateStatus()) {
    return NAN;
  }
  return _currentValues.energy;
}

uint16_t PZEM::getFrequency() {

  if (!checkUpdateStatus()) {
    return NAN;
  }
  return _currentValues.frequency;
}

uint16_t PZEM::getPowerFactor() {

  if (!checkUpdateStatus()) {
    return NAN;
  }
  return _currentValues.pf;
}

uint8_t PZEM::readAddress(bool update) {

  if (update) {
    return readSingleReg(Registers::SLAVE_ADDR, LENGTH_16bit_REG);
  }
  return _addr;
}
bool PZEM::setAddress(uint8_t newaddr) {
  if (newaddr < 0x01 || newaddr > 0xF7)  // sanity check
    return false;

  // Write the new address to the address register
  if (!sendCmd(FunctionCode::WRITE_SINGLE_REG, Registers::SLAVE_ADDR, newaddr,
               true))
    return false;

  _addr = newaddr;  // If successful, update the current slave address

  return true;
}

uint8_t PZEM::getAddress() {
  return readAddress();
  ;
}

bool PZEM::setPowerAlarm(uint16_t watts) {
  if (watts > 25000) {  // Sanitych check
    watts = 25000;
  }

  return sendCmd(FunctionCode::WRITE_SINGLE_REG, Registers::ALARM_THR, watts);
}
bool PZEM::getPowerAlarm() {
  if (!updateValues())  // Update vales if necessary
    return NAN;         // Update did not work, return NAN

  return _currentValues.alarms != 0x0000;
}

bool PZEM::sendCmd(FunctionCode cmd, uint16_t rAddr, uint16_t value, bool check,
                   uint16_t slave_addr) {
  uint8_t sendBuffer[FRAME_BUFFER_SIZE];  // Send buffer
  uint8_t respBuffer[FRAME_BUFFER_SIZE];  // Response buffer (only used when
                                          // check is true)

  // Validate and set slave address
  if ((slave_addr == 0xFFFF) || (slave_addr < MIN_SLAVE_ADDR)
      || (slave_addr > MAX_SLAVE_ADDR)) {
    slave_addr = _addr;
  }

  sendBuffer[0] = static_cast<uint8_t>(slave_addr);  // Set slave address
  sendBuffer[1] = static_cast<uint8_t>(cmd);         // Set command
  sendBuffer[2] = static_cast<uint8_t>(rAddr >> 8);  // High byte
  sendBuffer[3] = static_cast<uint8_t>(rAddr);       // Low byte
  sendBuffer[4] = static_cast<uint8_t>(value >> 8);
  sendBuffer[5] = static_cast<uint8_t>(value);

  // Calculate and append CRC
  setCRC(sendBuffer, FRAME_BUFFER_SIZE);
  _serial->write(sendBuffer, FRAME_BUFFER_SIZE);
  _serial->flush();  // Ensure the data is sent

  // Optionally verify response
  if (check) {
    ReceiveStatus received = receive(respBuffer, 8).status;
    if (received != ReceiveStatus::SUCCESS) {
      return false;  // Failed to receive response
    }

    // Check if the response matches the sent data
    for (uint8_t i = 0; i < 8; i++) {
      if (sendBuffer[i] != respBuffer[i]) {
        return false;  // Response did not match
      }
    }
  }

  return true;
}

ReceivedResult PZEM::receive(uint8_t *resp, uint16_t len) {
  uint32_t startTime = millis();
  uint16_t index = 0;
  while (millis() - startTime < _readTimeOut && index < len) {
    if (_serial->available()) {
      resp[index++] = _serial->read();
    }
  }
  if (index != len) {
    return {ReceiveStatus::TIMEOUT, index};
  }
  return {
      checkCRC(resp, len) ? ReceiveStatus::SUCCESS : ReceiveStatus::CRC_ERROR,
      index};
}

powerMeasure PZEM::readInputRegs() {
  uint8_t buf[25];
  if (sendCmd(FunctionCode::READ_INPUT_REG, Registers::VOLTAGE, 0x0A, true)) {
    ReceivedResult result = receive(buf, RESPONSE_SIZE);
    if (result.status == ReceiveStatus::SUCCESS) {
      return extractAllBits(buf);
    }
  }
  return powerMeasure();
}

uint32_t PZEM::readSingleReg(uint16_t rAddr, uint8_t rLength,
                             RegisterType regType) {
  uint8_t respBuffer[SINGLE_REG_RESPONSE];
  FunctionCode cmd = (regType == RegisterType::HOLDING_REG)
                         ? FunctionCode::READ_HOLDING_REG
                         : FunctionCode::READ_INPUT_REG;

  if (!sendCmd(cmd, rAddr, rLength, true)) {
    return PZEM_ERROR_VALUE;
  }
  ReceivedResult result = receive(respBuffer, rLength + 5);
  if (result.status == ReceiveStatus::SUCCESS) {
    return (rLength == LENGTH_32bit_REG) ? extract32BitValue(respBuffer, 3)
                                         : extract16BitValue(respBuffer, 3);
  }
  return PZEM_ERROR_VALUE;
}

uint16_t PZEM::CRC16(const uint8_t *data, uint16_t len) {
  uint8_t nTemp;          // CRC table index
  uint16_t crc = 0xFFFF;  // Default value

  while (len--) {
    nTemp = *data++ ^ crc;
    crc >>= 8;
    crc ^= (uint16_t)pgm_read_word(&crcTable[nTemp]);
  }
  return crc;
}

bool PZEM::checkCRC(const uint8_t *buf, uint16_t len) {
  if (len <= 2) {
    return false;  // Sanity check
  }

  uint16_t crc = CRC16(buf, len - 2);  // Calculate CRC of data
  return (buf[len - 2] == (crc & 0xFF))
         && (buf[len - 1] == (crc >> 8));  // Check CRC
}

void PZEM::setCRC(uint8_t *buf, uint16_t len) {
  if (len <= 2) {
    return;  // Sanity check
  }

  uint16_t crc = CRC16(buf, len - 2);  // Calculate CRC of data
  buf[len - 2] = crc & 0xFF;           // Set low byte
  buf[len - 1] = (crc >> 8);           // Set high byte
}

uint16_t PZEM::extract16BitValue(const uint8_t *response, int startIndex) {
  return (static_cast<uint16_t>(response[startIndex]) << 8)
         | response[startIndex + 1];
}

uint32_t PZEM::extract32BitValue(const uint8_t *response, int startIndex) {

  uint16_t raw_low = extract16BitValue(response, startIndex);
  uint16_t raw_high = extract16BitValue(response, startIndex + 2);

  return (static_cast<uint32_t>(raw_high) << 16) | raw_low;
}

powerMeasure PZEM::extractAllBits(const uint8_t *response) {
  powerMeasure power;

  power.voltage = extract16BitValue(response, 0) / 10.0;
  power.current = extract32BitValue(response, 2) / 1000.0;
  power.power = extract32BitValue(response, 6) / 10.0;
  power.energy = extract32BitValue(response, 10) / 1000.0;
  power.frequency = extract16BitValue(response, 14) / 10.0;
  power.pf = extract16BitValue(response, 16) / 100.0;
  power.alarms = extract16BitValue(response, 18);

  return power;
}

bool PZEM::resetEnergy() {
  uint8_t buffer[]
      = {0x00, static_cast<uint8_t>(FunctionCode::RESET), 0x00, 0x00};
  uint8_t reply[5];
  buffer[0] = _addr;

  setCRC(buffer, 4);
  _serial->write(buffer, 4);

  uint16_t length = receive(reply, 5).bytes_received;

  if (length == 0 || length == 5) {
    return false;
  }

  return true;
}
void PZEM::search() {
  uint8_t respBuffer[SINGLE_REG_RESPONSE];  // Response buffer

  for (uint8_t i = MIN_SLAVE_ADDR; i <= PZEM_DEFAULT_ADDR; i++) {
    if (sendCmd(FunctionCode::READ_INPUT_REG, Registers::VOLTAGE, 0x01, true,
                i)) {
      receive(respBuffer, SINGLE_REG_RESPONSE);  // Check response

      if (respBuffer[0] == i) {  // If response matches address
        Serial.print("Found Device @: 0x");
        Serial.println(i, HEX);
      }
    }
  }
}
