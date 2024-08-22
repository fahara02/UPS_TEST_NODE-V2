#include "TestManager.h"
#include "TestData.h"

extern volatile bool mains_triggered;
extern volatile bool ups_triggered;
extern TaskHandle_t ISR_MAINS_POWER_LOSS;
extern TaskHandle_t ISR_UPS_POWER_GAIN;
extern TaskHandle_t ISR_UPS_POWER_LOSS;
extern SemaphoreHandle_t mainLoss;

TestManager* TestManager::instance = nullptr;

TestManager::TestManager() {

  stateMachine = StateMachine::getInstance();
  if (stateMachine) {
    Serial.println("State machine is on!");
    _currentstate = stateMachine->getCurrentState();

    stateMachine->handleEvent(Event::SELF_CHECK_OK);
  }
}

TestManager::~TestManager() {}

TestManager* TestManager::getInstance() {
  if (instance == nullptr) {
    instance = new TestManager();
  }
  return instance;
}

void TestManager::deleteInstance() {
  delete instance;
  instance = nullptr;
}

void TestManager::init() {
  if (_initialized) {
    return;  // Already initialized, do nothing
  }
  setupPins();
  createISRTasks();
  createManagerTasks();
  initializeTestInstances();

  _initialized = true;  // Mark as initialized
}

void TestManager::setupPins() {
  pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_DOWN, INPUT_PULLDOWN);

  pinMode(UPS_POWER_CUT_PIN, OUTPUT);  // Set power cut pin as output
  pinMode(TEST_END_INT_PIN, OUTPUT);
  pinMode(LOAD_PWM_PIN, OUTPUT);
  pinMode(LOAD25P_ON_PIN, OUTPUT);
  pinMode(LOAD50P_ON_PIN, OUTPUT);
  pinMode(LOAD75P_ON_PIN, OUTPUT);
  pinMode(LOAD_FULL_ON_PIN, OUTPUT);
  pinMode(TEST_END_INT_PIN, OUTPUT);

  ledcSetup(_cfgHardware.pwmchannelNo, _cfgHardware.pwm_frequency,
            _cfgHardware.pwmResolusion_bits);
  ledcWrite(_cfgHardware.pwmchannelNo, 0);
  ledcAttachPin(LOAD_PWM_PIN, 0);
  configureInterrupts();
  Serial.println("After configuring interrupts:");
  pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_DOWN, INPUT_PULLDOWN);
}

void TestManager::configureInterrupts() {
  gpio_num_t mainpowerPin = static_cast<gpio_num_t>(SENSE_MAINS_POWER_PIN);
  gpio_num_t upspowerupPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_PIN);
  gpio_num_t upsshutdownPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_DOWN);
  gpio_install_isr_service(0);

  gpio_set_intr_type(mainpowerPin, GPIO_INTR_NEGEDGE);
  gpio_set_intr_type(upspowerupPin, GPIO_INTR_POSEDGE);
  gpio_set_intr_type(upsshutdownPin, GPIO_INTR_NEGEDGE);

  // Add other cases as needed

  gpio_isr_handler_add(mainpowerPin, keyISR1, NULL);
  gpio_isr_handler_add(upspowerupPin, keyISR2, NULL);
  gpio_isr_handler_add(upsshutdownPin, keyISR3, NULL);

  Serial.print("Interrupt configuration complete\n");
}
void TestManager::createManagerTasks() {
  xTaskCreatePinnedToCore(TestManagerTask, "MainsTestManager",
                          _cfgTask.mainTest_taskStack, &_cfgTaskParam, 3,
                          &TestManagerTaskHandle, _cfgTask.mainsISR_taskCore);
}
void TestManager::createISRTasks() {

  xTaskCreatePinnedToCore(onMainsPowerLossTask, "MainslossISRTask",
                          _cfgTask.mainsISR_taskStack, NULL,
                          _cfgTask.mainsISR_taskIdlePriority,
                          &ISR_MAINS_POWER_LOSS, _cfgTask.mainsISR_taskCore);

  xTaskCreatePinnedToCore(onUPSPowerGainTask, "UPSgainISRTask",
                          _cfgTask.upsISR_taskStack, NULL,
                          _cfgTask.upsISR_taskIdlePriority, &ISR_UPS_POWER_GAIN,
                          _cfgTask.upsISR_taskCore);

  xTaskCreatePinnedToCore(onUPSPowerLossTask, "UPSLossISRTask",
                          _cfgTask.upsISR_taskStack, NULL,
                          _cfgTask.upsISR_taskIdlePriority, &ISR_UPS_POWER_LOSS,
                          _cfgTask.upsISR_taskCore);
}

void TestManager::TestManagerTask(void* pvParameters) {
  // Wait for the DEVICE_READY bit to be set initially
  // uint32_t result
  //     = xEventGroupWaitBits(instance->stateMachine->_EgTestState,
  //                           static_cast<EventBits_t>(State::DEVICE_READY),
  //                           pdFALSE, pdTRUE, portMAX_DELAY);
  uint32_t result
      = xEventGroupGetBits(instance->stateMachine->TestState_EventGroup);
  Serial.print("Eventbit before loop: ");
  Serial.println(result, BIN);  // Print bitmask in binary format for clarity
  int count = 0;
  while (true) {
    // Get the current event bits
    result = xEventGroupGetBits(instance->stateMachine->TestState_EventGroup);

    Serial.print("Test manager task...");

    Serial.print("Eventbit: ");
    Serial.println(result, BIN);  // Print bitmask in binary format for clarity

    if (result & static_cast<EventBits_t>(State::DEVICE_OK)) {
      if (instance->stateMachine->getCurrentState() == State::DEVICE_OK) {
        Serial.println("DEVICE IS ON DEVICE_OK");
        count = count + 1;
        if (count == 3) {
          Serial.println("changing state now");
          instance->stateMachine->handleEvent(Event::SETTING_LOADED);
        }
      }
    }

    if (result & static_cast<EventBits_t>(State::DEVICE_SETUP)) {
      if (instance->stateMachine->getCurrentState() == State::DEVICE_SETUP) {
        Serial.println("DEVICE IS ON DEVICE_SETUP");
      }
      count = count + 1;
      if (count == 7) {
        Serial.println("Making errors now");
        instance->stateMachine->handleEvent(Event::ERROR);
      }
    }

    // Delay to avoid rapid looping
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  Serial.print("Exiting test manager task.....");
  vTaskDelete(NULL);
}

void TestManager::onMainsPowerLossTask(void* pvParameters) {

  while (true) {
    if (xSemaphoreTake(mainLoss, portMAX_DELAY)) {
      vTaskPrioritySet(&ISR_MAINS_POWER_LOSS, 3);
      if (switchTest) {
        switchTest->_dataCaptureRunning = true;
        switchTest->startTestCapture();
        Serial.print("\033[31m");  // Start red color
        Serial.print("mains Powerloss triggered...");
        Serial.print("\033[0m");  // Reset color
        mains_triggered = false;
      }
      Serial.print("mains task High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack
      Serial.println("Mains Loss trigger capture completed.... ");
      vTaskPrioritySet(&ISR_MAINS_POWER_LOSS, 1);
      vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
      vTaskSuspend(NULL);
    }
  }
  vTaskDelete(NULL);
}

// // ISR for UPS power gain
void TestManager::onUPSPowerGainTask(void* pvParameters) {

  while (true) {
    if (ups_triggered) {

      if (switchTest) {
        switchTest->stopTestCapture();
        Serial.print("\033[31m");  // Start red color
        Serial.print("UPS Powerloss triggered...");
        Serial.print("\033[0m");  // Reset color
        ups_triggered = false;
      }
      Serial.print("UPS High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack

      vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
      vTaskSuspend(NULL);
    }
  }
  vTaskDelete(NULL);
}

void TestManager::onUPSPowerLossTask(void* pvParameters) { vTaskDelete(NULL); }

void TestManager::initializeTestInstances() {
  // Initialize SwitchTest
  switchTest = SwitchTest::getInstance();
  testsSwitch[0].test = switchTest;
  testsSwitch[0].status = TestStatus::TEST_PENDING;
  testsSwitch[0].data = testsSwitch[0].test->data();
  switchTest->init();

  numTests = MAX_TESTS;  // Set the number of tests
}
