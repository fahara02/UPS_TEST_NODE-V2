#include <Arduino.h>
#include "Adafruit_MAX31855.h"
#include "SPI.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "Logger.h"
#include "ModbusManager.h"
#include "TestSync.h"
#include "TestManager.h"
#include "UPSTest.h"
#include "SwitchTest.h"
#include "BackupTest.h"
#include "UPSdebug.h"
#include "PageBuilder.h"

using namespace Node_Core;

// Global Logger Instance
Logger& logger = Logger::getInstance();
TestSync& SyncTest = TestSync::getInstance();
SwitchTest* switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();
BackupTest* backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();

volatile unsigned long lastMainsTriggerTime = 0;
volatile unsigned long lastUPSTriggerTime = 0;
volatile bool check_ups_shutdown = false;
const unsigned long debounceDelay = 100;

SemaphoreHandle_t mainLoss = NULL;
SemaphoreHandle_t upsLoss = NULL;
SemaphoreHandle_t upsGain = NULL;

xSemaphoreHandle state_mutex = NULL;

TaskHandle_t modbusRTUTaskHandle = NULL;
TaskHandle_t switchTestTaskHandle = NULL;
TaskHandle_t backupTestTaskHandle = NULL;
TaskHandle_t efficiencyTestTaskHandle = NULL;
TaskHandle_t inputvoltageTestTaskHandle = NULL;
TaskHandle_t waveformTestTaskHandle = NULL;
TaskHandle_t tunepwmTestTaskHandle = NULL;

TaskHandle_t TestManagerTaskHandle = NULL;

TaskHandle_t ISR_MAINS_POWER_LOSS = NULL;
TaskHandle_t ISR_UPS_POWER_GAIN = NULL;
TaskHandle_t ISR_UPS_POWER_LOSS = NULL;

QueueHandle_t TestManageQueue = NULL;
QueueHandle_t SwitchTestDataQueue = NULL;
QueueHandle_t BackupTestDataQueue = NULL;
static const uint8_t messageQueueLength = 10;

EventGroupHandle_t eventGroupTest = NULL;
EventGroupHandle_t eventGroupSwitchTestData = NULL;
EventGroupHandle_t eventGroupBackupTestData = NULL;

// Define the SwitchTest instance

UPSTesterSetup* TesterSetup = nullptr;
TestManager* Manager = nullptr;

// Task handles

SemaphoreHandle_t xSemaphore;

Modbus::ResultCode err;

ModbusRTU mb;
#define ESP_LITTLEFS_TAG = "LFS"

void IRAM_ATTR keyISR1(void* pvParameters)
{
	unsigned long currentTime = millis();
	if(currentTime - lastMainsTriggerTime > debounceDelay)
	{
		BaseType_t urgentTask = pdFALSE;
		lastMainsTriggerTime = currentTime;
		xSemaphoreGiveFromISR(mainLoss, &urgentTask);
		if(urgentTask)
		{
			vPortEvaluateYieldFromISR(urgentTask);
		}
	}
}
void IRAM_ATTR keyISR2(void* pvParameters)
{
	unsigned long currentTime = millis();
	if(currentTime - lastUPSTriggerTime > debounceDelay)
	{
		BaseType_t urgentTask = pdFALSE;
		lastMainsTriggerTime = currentTime;
		// xTaskResumeFromISR(ISR_MAINS_POWER_LOSS);
		xSemaphoreGiveFromISR(upsGain, &urgentTask);
		if(urgentTask)
		{
			vPortEvaluateYieldFromISR(urgentTask);
		}
	}
}
void IRAM_ATTR keyISR3(void* pvParameters)
{
	unsigned long currentTime = millis();
	if(currentTime - lastUPSTriggerTime > debounceDelay)
	{
		BaseType_t urgentTask = pdFALSE;
		lastMainsTriggerTime = currentTime;
		// xTaskResumeFromISR(ISR_MAINS_POWER_LOSS);
		xSemaphoreGiveFromISR(upsLoss, &urgentTask);
		if(urgentTask)
		{
			vPortEvaluateYieldFromISR(urgentTask);
		}
	}
}

void modbusRTUTask(void* pvParameters)
{
	while(true)
	{
		logger.log(LogLevel::INFO, "Resuming modbus task");
		mb.task();
		vTaskDelay(pdMS_TO_TICKS(500)); // Task delay
	}
	vTaskDelete(NULL);
}

void setup()
{
	mainLoss = xSemaphoreCreateBinary();
	upsGain = xSemaphoreCreateBinary();
	upsLoss = xSemaphoreCreateBinary();

	TestManageQueue = xQueueCreate(messageQueueLength, sizeof(SetupTaskParams));
	SwitchTestDataQueue = xQueueCreate(messageQueueLength, sizeof(SwitchTestData));
	BackupTestDataQueue = xQueueCreate(messageQueueLength, sizeof(BackupTestData));

	// Initialize Serial for debugging
	logger.init();
	logger.log(LogLevel::INFO, "Serial started........");

	if(mainLoss == NULL || upsGain == NULL || upsLoss == NULL)
	{
		logger.log(LogLevel::ERROR, "Failed to create one or more ISR semaphores");
		// Handle error
	}
	else
	{
		logger.log(LogLevel::SUCCESS, "Successfully created ISR semaphores");
	}

	SyncTest.init();
	modbusRTU_Init();
	Serial2.begin(9600, SERIAL_8N1);
	mb.begin(&Serial2);
	mb.slave(1);
	logger.log(LogLevel::INFO, "modbus slave configured");

	logger.log(LogLevel::INFO, "creating semaphores..");

	logger.log(LogLevel::INFO, "creating queue");

	logger.log(LogLevel::INFO, "getting TesterSetup  instance");
	TesterSetup = UPSTesterSetup::getInstance();

	if(TesterSetup)
	{
		logger.log(LogLevel::SUCCESS, "TesterSetup instance created!");
	}
	else
	{
		logger.log(LogLevel::ERROR, "TesterSetup instance creation failed");
	}
	logger.log(LogLevel::INFO, "getting manager instance");
	Manager = TestManager::getInstance();

	if(Manager)
	{
		logger.log(LogLevel::SUCCESS, "Manager instance created!");
		Manager->init();
	}
	else
	{
		logger.log(LogLevel::ERROR, "Manager instance creation failed");
	}

	if(Manager)
	{
		RequiredTest testlist[] = {
			{1, TestType::BackupTest, LoadPercentage::LOAD_50P, true},
			{2, TestType::SwitchTest, LoadPercentage::LOAD_75P, true},

		};
		logger.log(LogLevel::INFO, "adding Tests");
		Manager->addTests(testlist, sizeof(testlist) / sizeof(testlist[0]));
		logger.log(LogLevel::INFO, "changing states");

		Manager->triggerEvent(Event::SELF_CHECK_OK);
		vTaskDelay(pdTICKS_TO_MS(100));
		Manager->triggerEvent(Event::SETTING_LOADED);
		vTaskDelay(pdTICKS_TO_MS(100));
		Manager->triggerEvent(Event::LOAD_BANK_CHECKED);
		vTaskDelay(pdTICKS_TO_MS(100));

		xTaskCreatePinnedToCore(modbusRTUTask, "ModbusRTUTask", 10000, NULL, 1,
								&modbusRTUTaskHandle, 0);
	}

	else
	{
		logger.log(LogLevel::ERROR, "Cant create manager instance");
	}
}
void loop()
{
	// The scheduler will handle tasks; loop should remain empty
}
