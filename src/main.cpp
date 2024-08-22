#include "Adafruit_MAX31855.h"
#include "FS.h"
#include "Logger.h"
#include "ModbusManager.h"
#include "SwitchTest.h"
#include "TestManager.h"
#include "UPSTest.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Wire.h>
#include <esp32-hal-log.h>

using namespace Node_Core;

// Global Logger Instance
Logger& logger = Logger::getInstance();

volatile unsigned long lastMainsTriggerTime = 0;
volatile unsigned long lastUPSTriggerTime = 0;
volatile bool mains_triggered = false;
volatile bool ups_triggered = false;
volatile bool check_ups_shutdown = false;
const unsigned long debounceDelay = 100;

SemaphoreHandle_t mainLoss = NULL;
SemaphoreHandle_t upsLoss = NULL;
SemaphoreHandle_t upsGain = NULL;

TaskHandle_t modbusRTUTaskHandle = NULL;
TaskHandle_t switchTestTaskHandle = NULL;
TaskHandle_t backupTimeTestTaskHandle = NULL;
TaskHandle_t efficiencyTestTaskHandle = NULL;
TaskHandle_t inputvoltageTestTaskHandle = NULL;
TaskHandle_t waveformTestTaskHandle = NULL;
TaskHandle_t tunepwmTestTaskHandle = NULL;

TaskHandle_t ISR_MAINS_POWER_LOSS = NULL;
TaskHandle_t ISR_UPS_POWER_GAIN = NULL;
TaskHandle_t ISR_UPS_POWER_LOSS = NULL;

// Define the SwitchTest instance

UPSTesterSetup* TesterSetup = nullptr;

TestManager* Manager = nullptr;
// Task handles

SemaphoreHandle_t xSemaphore;

Modbus::ResultCode err;

ModbusRTU mb;
#define ESP_LITTLEFS_TAG = "LFS"

void IRAM_ATTR keyISR1(void* pvParameters) {
  unsigned long currentTime = millis();
  if (currentTime - lastMainsTriggerTime > debounceDelay) {
    BaseType_t urgentTask = pdFALSE;
    lastMainsTriggerTime = currentTime;
    // xTaskResumeFromISR(ISR_MAINS_POWER_LOSS);
    xSemaphoreGiveFromISR(mainLoss, &urgentTask);
    if (urgentTask) {
      vPortEvaluateYieldFromISR(urgentTask);
    }
  }
}
void IRAM_ATTR keyISR2(void* pvParameters) {
  unsigned long currentTime = millis();
  if (currentTime - lastUPSTriggerTime > debounceDelay) {
    BaseType_t urgentTask = pdFALSE;
    lastMainsTriggerTime = currentTime;
    // xTaskResumeFromISR(ISR_MAINS_POWER_LOSS);
    xSemaphoreGiveFromISR(upsGain, &urgentTask);
    if (urgentTask) {
      vPortEvaluateYieldFromISR(urgentTask);
    }
  }
}
void IRAM_ATTR keyISR3(void* pvParameters) {
  unsigned long currentTime = millis();
  if (currentTime - lastUPSTriggerTime > debounceDelay) {
    BaseType_t urgentTask = pdFALSE;
    lastMainsTriggerTime = currentTime;
    // xTaskResumeFromISR(ISR_MAINS_POWER_LOSS);
    xSemaphoreGiveFromISR(upsLoss, &urgentTask);
    if (urgentTask) {
      vPortEvaluateYieldFromISR(urgentTask);
    }
  }
}

void modbusRTUTask(void* pvParameters) {

  while (true) {
    // Serial.print("Resuming modbus task... ");
    mb.task();
    // Serial.print("Modbus Stack High Water Mark: ");
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack
    // usage

    vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
  }
  vTaskDelete(NULL);
}

void setup() {
  mainLoss = xSemaphoreCreateBinary();
  upsGain = xSemaphoreCreateBinary();
  upsLoss = xSemaphoreCreateBinary();
  // Initialize Serial for debugging
  logger.init();
  logger.log(LogLevel::INFO, "Serial started........");

  TesterSetup = UPSTesterSetup::getInstance();
  Manager = TestManager::getInstance();

  if (Manager) {
    Manager->init();
    logger.log(LogLevel::INFO, "Testmanager  initialised........");
  }
  modbusRTU_Init();
  Serial2.begin(9600, SERIAL_8N1);
  mb.begin(&Serial2);
  mb.slave(1);
  Serial.print("modbus slave configured");

  xTaskCreatePinnedToCore(modbusRTUTask, "ModbusRTUTask", 10000, NULL, 1,
                          &modbusRTUTaskHandle, 0);
}

void loop() {

  // The scheduler will handle tasks; loop should remain empty
}
