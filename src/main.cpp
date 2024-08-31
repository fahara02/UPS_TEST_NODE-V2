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

#include "PageBuilder.h"
#include "UPSdebug.h"

using namespace Node_Core;

// Global Logger Instance
Logger& logger = Logger::getInstance();
TestSync& SyncTest = TestSync::getInstance();
StateMachine& stateMachine = StateMachine::getInstance();
UPSTesterSetup& TesterSetup = UPSTesterSetup::getInstance();
SwitchTest& switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();
BackupTest& backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();

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

// Task handles
const char* PARAM_MESSAGE = "message";
SemaphoreHandle_t xSemaphore;
AsyncWebServer server(80);

WiFiManager wm;
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
		logger.log(LogLevel::WARNING, "Resuming modbus task");
		// mb.task();
		vTaskDelay(pdMS_TO_TICKS(2000)); // Task delay
	}
	vTaskDelete(NULL);
}

void setup()

{
	Serial.begin(115200);

	WiFi.mode(WIFI_STA);
	wm.setClass("invert");
	auto reboot = false;
	wm.setAPCallback([&reboot](WiFiManager* wifiManager) {
		reboot = true;
	});
	wm.autoConnect();
	if(reboot)
		// {
		ESP.restart();

	PageBuilder builder(&server);
	builder.setupPages();

	// Initialize Serial for debugging
	logger.init(&Serial, LogLevel::INFO, 10);
	logger.log(LogLevel::INFO, "Serial started........");
	server.begin();
	logger.log(LogLevel::INFO, "creating semaphores..");
	mainLoss = xSemaphoreCreateBinary();
	upsGain = xSemaphoreCreateBinary();
	upsLoss = xSemaphoreCreateBinary();
	logger.log(LogLevel::INFO, "creating queue");
	TestManageQueue = xQueueCreate(messageQueueLength, sizeof(SetupTaskParams));
	SwitchTestDataQueue = xQueueCreate(messageQueueLength, sizeof(SwitchTestData));
	BackupTestDataQueue = xQueueCreate(messageQueueLength, sizeof(BackupTestData));

	logger.log(LogLevel::INFO, "initiating test sync");
	SyncTest.init();
	logger.log(LogLevel::INFO, "initiating modbus");
	modbusRTU_Init();
	Serial2.begin(9600, SERIAL_8N1);
	mb.begin(&Serial2);
	mb.slave(1);
	xTaskCreate(modbusRTUTask, "ModbusRTUTask", 4096, NULL, 1, &modbusRTUTaskHandle);
	logger.log(LogLevel::INFO, "modbus slave configured");

	logger.log(LogLevel::INFO, "getting manager instance");
	TestManager& Manager = TestManager::getInstance();
	logger.log(LogLevel::INFO, "getting manager init");
	Manager.init();

	RequiredTest testlist[] = {
		{1, TestType::BackupTest, LoadPercentage::LOAD_50P, true},
		{2, TestType::SwitchTest, LoadPercentage::LOAD_75P, true},

	};
	logger.log(LogLevel::INFO, "adding Tests");
	Manager.addTests(testlist, sizeof(testlist) / sizeof(testlist[0]));
	logger.log(LogLevel::INFO, "changing states");

	// Manager.triggerEvent(Event::SELF_CHECK_OK);
	vTaskDelay(pdTICKS_TO_MS(100));
	Manager.triggerEvent(Event::SETTING_LOADED);
	vTaskDelay(pdTICKS_TO_MS(100));
	Manager.triggerEvent(Event::LOAD_BANK_CHECKED);
	vTaskDelay(pdTICKS_TO_MS(100));
}
void loop()
{
	vTaskDelete(NULL);
}
