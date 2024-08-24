#include "Adafruit_MAX31855.h"
#include "FS.h"
#include "Logger.h"
#include "ModbusManager.h"
#include "SwitchTest.h"
#include "TestManager.h"
#include "TestReq.h"

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
TestSync& UPSTestSync = TestSync::getInstance();

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

  // Initialize Serial for debugging
  logger.init();
  logger.log(LogLevel::INFO, "Serial started........");

  modbusRTU_Init();
  Serial2.begin(9600, SERIAL_8N1);
  mb.begin(&Serial2);
  mb.slave(1);
  logger.log(LogLevel::INFO, "modbus slave configured");
  xTaskCreatePinnedToCore(modbusRTUTask, "ModbusRTUTask", 10000, NULL, 1,
                          &modbusRTUTaskHandle, 0);

  logger.log(LogLevel::INFO, "creating semaphores..");

  mainLoss = xSemaphoreCreateBinary();
  upsGain = xSemaphoreCreateBinary();
  upsLoss = xSemaphoreCreateBinary();

  // AllTest[0].testName = TestType::SwitchTest;
  // AllTest[1].testName = TestType::SwitchTest;
  // AllTest[2].testName = TestType::SwitchTest;
  // AllTest[0].level = LoadLevel::LEVEL_25;
  // AllTest[1].level = LoadLevel::LEVEL_50;
  // AllTest[2].level = LoadLevel::LEVEL_75;
  // UPSTestSync.addTests(AllTest, 3);

  // UPSTestSync.testBitEncoding();

  TesterSetup = UPSTesterSetup::getInstance();
  Manager = TestManager::getInstance();

  if (Manager) {
    Manager->init();
    logger.log(LogLevel::INFO, "Testmanager  initialised........");
    logger.log(LogLevel::INFO, "Artificially getting to AUTO MODE");
    Manager->triggerEvent(Event::SELF_CHECK_OK);
    vTaskDelay(pdTICKS_TO_MS(100));
    Manager->triggerEvent(Event::SETTING_LOADED);
    vTaskDelay(pdTICKS_TO_MS(100));
    Manager->triggerEvent(Event::LOAD_BANK_CHECKED);
    vTaskDelay(pdTICKS_TO_MS(100));
    logger.log(LogLevel::INFO, "Triggering Auto test cmd event........");
    Manager->triggerEvent(Event::AUTO_TEST_CMD);
    vTaskDelay(pdTICKS_TO_MS(100));
    logger.log(LogLevel::INFO, "Triggering input output ready event........");
    Manager->triggerEvent(Event::INPUT_OUTPUT_READY);
    vTaskDelay(pdTICKS_TO_MS(100));
  }
}
void loop() {

  // The scheduler will handle tasks; loop should remain empty
}
