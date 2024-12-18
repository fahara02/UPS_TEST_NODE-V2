#include <Arduino.h>
#include "Adafruit_MAX31855.h"
#include "SPI.h"
#include <WiFi.h>
#include <WifiClient.h>
#include <AsyncTCP.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "esp_heap_caps.h"
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "Logger.h"

#include "UPSTestNode.h"
#include "EventHelper.h"
#include <nvs_flash.h>
#include "TestSync.h"
#include "TestServer.h"
#include "UPSTime.h"
#include "DataHandler.h"
#include "TaskMonitor.h"
#include "PZEM_Modbus.hpp"
#include <memory>
#include "ModbusClientTCP.h"

using namespace Node_Core;
using namespace Node_Utility;
// Global Logger Instance
Logger& logger = Logger::getInstance();
// TaskMonitor& monitor = TaskMonitor::getInstance();
StateMachine& stateMachine = StateMachine::getInstance();
TestSync& SyncTest = TestSync::getInstance();
UPSTesterSetup& TesterSetup = UPSTesterSetup::getInstance();
DataHandler& wsDataHandler = DataHandler::getInstance();
SwitchTest& switchTest = UPSTest<SwitchTest, SwitchTestData>::getInstance();
BackupTest& backupTest = UPSTest<BackupTest, BackupTestData>::getInstance();

volatile unsigned long lastMainsTriggerTime = 0;
volatile unsigned long lastUPSTriggerTime = 0;
volatile bool check_ups_shutdown = false;
const unsigned long debounceDelay = 100;

SemaphoreHandle_t mainLoss = NULL;
SemaphoreHandle_t upsLoss = NULL;
SemaphoreHandle_t upsGain = NULL;

TaskHandle_t ISR_MAINS_POWER_LOSS = NULL;
TaskHandle_t ISR_UPS_POWER_GAIN = NULL;
TaskHandle_t ISR_UPS_POWER_LOSS = NULL;

TaskHandle_t switchTestTaskHandle = NULL;
TaskHandle_t backupTestTaskHandle = NULL;
TaskHandle_t efficiencyTestTaskHandle = NULL;
TaskHandle_t inputvoltageTestTaskHandle = NULL;
TaskHandle_t waveformTestTaskHandle = NULL;
TaskHandle_t tunepwmTestTaskHandle = NULL;
TaskHandle_t TestManagerTaskHandle = NULL;

QueueHandle_t TestManageQueue = NULL;
QueueHandle_t SwitchTestDataQueue = NULL;
QueueHandle_t BackupTestDataQueue = NULL;

static const uint8_t messageQueueLength = 10;

EventGroupHandle_t eventGroupSwitchTestData = NULL;
EventGroupHandle_t eventGroupBackupTestData = NULL;

// Define the SwitchTest instance

// Task handles
const char* PARAM_MESSAGE = "message";
SemaphoreHandle_t xSemaphore;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

WiFiManager wm;
// WiFiClient theClient;
// auto MClient = std::make_unique<ModbusClientTCP>(theClient);
Node_Utility::ModbusManager& MBManager = Node_Utility::ModbusManager::getInstance();

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
		// Monitor and log the available free heap memory
		size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT); // For standard heap (DRAM)

		logger.log(LogLevel::INFO, "Free heap byets: ", freeHeap);
		if(heap_caps_check_integrity_all(true))
		{
			logger.log(LogLevel::SUCCESS, "No fragmentation in heap");
		}
		else
		{
			logger.log(LogLevel::ERROR, "heap fragmented");
		};
		// mb.task();  // Uncomment when Modbus task needs to run
		vTaskDelay(1);
		vTaskDelay(pdMS_TO_TICKS(2000)); // Task delay
	}

	vTaskDelete(NULL);
}

void setup()

{
	setupTime();
	xTaskCreatePinnedToCore(TimeKeeperTask, "TimeKeeperTask", timer_Stack, NULL, timer_Priority,
							NULL, timer_CORE);
	Serial.begin(115200);
	// Initialize NVS
	esp_err_t err = nvs_flash_init();

	// Handle NVS flash errors (re-init if necessary)
	if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		Serial.println("Erasing NVS and reinitializing...");
		nvs_flash_erase(); // Erase NVS partition and retry init
		err = nvs_flash_init();
	}

	// Check if NVS initialization succeeded
	if(err != ESP_OK)
	{
		Serial.println("NVS initialization failed!");
	}
	else
	{
		Serial.println("NVS initialized successfully!");
	}

	// Now you can begin using Preferences
	if(!preferences.begin("state_store", false))
	{
		Serial.println("Failed to open preferences!");
	}
	else
	{
		Serial.println("Preferences opened successfully!");
		// Add your preferences handling logic here, for example:
		int lastState = preferences.getUInt("last_state", (uint32_t)State::DEVICE_ON);
		Serial.printf("Last saved state: %d\n", lastState);
	}
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

	// Initialize Serial for debugging
	logger.init(&Serial, LogLevel::INFO, 20);
	logger.log(LogLevel::INFO, "Serial started........");

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

	// Issue a request

	Node_Utility::ModbusManager::Target target1 = {
		TargetType::INPUT_POWER, IPAddress(192, 168, 0, 172), 1, READ_HOLD_REGISTER, 761, 0, 21};
	Node_Utility::ModbusManager::Target target2 = {
		TargetType::OUTPUT_POWER, IPAddress(192, 168, 0, 172), 2, READ_HOLD_REGISTER, 123, 0, 21};

	MBManager.autopoll(true, target1, target2);

	logger.log(LogLevel::INFO, "modbus client configured");

	logger.log(LogLevel::INFO, "getting manager instance");
	TestManager& Manager = TestManager::getInstance();
	logger.log(LogLevel::INFO, "getting manager init");
	Manager.init();

	logger.log(LogLevel::INFO, "getting datahandler init");
	wsDataHandler.init();

	logger.log(LogLevel::INFO, "changing states");
	Manager.passEvent(Event::SELF_CHECK_OK);
	vTaskDelay(pdTICKS_TO_MS(100));
	Manager.passEvent(Event::SETTING_LOADED);
	vTaskDelay(pdTICKS_TO_MS(100));
	Manager.passEvent(Event::LOAD_BANK_CHECKED);
	vTaskDelay(pdTICKS_TO_MS(100));

	TesterSetup.addObserver(&Manager);
	TesterSetup.addObserver(&switchTest);
	TesterSetup.addObserver(&backupTest);

	TestServer testServer(&server, &ws, TesterSetup, SyncTest);
	testServer.servePages(TesterSetup, SyncTest);
	testServer.begin();

	server.begin();
	if(ws.enabled())
	{
		logger.log(LogLevel::SUCCESS, "Websocket is enabled");
	}

	// monitor.setPrintDelay(5000);

	// monitor.addTask("async_tcp");
	// monitor.addTask("ProcessWsData");
	// monitor.addTask("wsDataSender");
	// monitor.addTask("userCommand");
	// monitor.addTask("userUpdate");
	// monitor.addTask("testSync");
	// monitor.addTask("MainTestManager");
	// monitor.addTask("SwitchTestTask");
	// monitor.addTask("BackUpTestTask");
	// monitor.addTask("WSCleanupTask");
}
void loop()
{
	vTaskDelete(NULL);
}
