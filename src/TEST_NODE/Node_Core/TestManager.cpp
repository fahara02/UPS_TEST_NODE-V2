#include "TestManager.h"
#include "Logger.h"
#include "TestData.h"

using namespace Node_Core;
extern Logger& logger;

extern volatile bool mains_triggered;
extern volatile bool ups_triggered;
extern TaskHandle_t ISR_MAINS_POWER_LOSS;
extern TaskHandle_t ISR_UPS_POWER_GAIN;
extern TaskHandle_t ISR_UPS_POWER_LOSS;

extern SemaphoreHandle_t mainLoss;
extern SemaphoreHandle_t upsLoss;
extern SemaphoreHandle_t upsGain;

TestManager* TestManager::instance = nullptr;

TestManager::TestManager() {

  stateMachine = StateMachine::getInstance();
  if (stateMachine) {
    logger.log(LogLevel::SUCCESS, "State machine is on!");
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
  initializeTestInstances();
  pauseallTestTask();

  createManagerTasks();

  _initialized = true;  // Mark as initialized
}

void TestManager::pauseallTestTask() {
  vTaskSuspend(switchTestTaskHandle);
  logger.log(LogLevel::WARNING, "SwitchTest task is paused");
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

  logger.log(LogLevel::SUCCESS, "Interrupts configured");
  pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_DOWN, INPUT_PULLDOWN);
}

void TestManager::configureInterrupts() {
  logger.log(LogLevel::INFO, "configuring Interrupts ");
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

  uint32_t result
      = xEventGroupGetBits(instance->stateMachine->TestState_EventGroup);
  State currentState = instance->stateMachine->getCurrentState();

  while (true) {

    while (currentState == State::DEVICE_OK) {
      logger.log(LogLevel::TEST, "starting switch test process");
      if (switchTest) {
        switchTest->_cfgTest.testVARating = 1000;
        switchTest->_cfgTest.testDuration_ms = 5000;
        logger.log(LogLevel::INFO, "checking task state");
        eTaskState estate;
        estate = eTaskGetState(switchTestTaskHandle);

        if (estate == eSuspended) {
          logger.log(LogLevel::INFO, "resuming blocked task");
          vTaskResume(switchTestTaskHandle);

          vTaskPrioritySet(&switchTestTaskHandle, 3);
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      logger.log(LogLevel::INFO, "starting switch test 25 percent  load");
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }

  vTaskDelete(NULL);
}

void TestManager::onMainsPowerLossTask(void* pvParameters) {

  while (true) {
    if (xSemaphoreTake(mainLoss, portMAX_DELAY)) {
      vTaskPrioritySet(&ISR_MAINS_POWER_LOSS, 3);
      if (switchTest) {
        switchTest->_dataCaptureRunning = true;
        switchTest->startTestCapture();
        logger.log(LogLevel::INTR, "mains Powerloss triggered...");
        mains_triggered = false;
      }

      logger.log(LogLevel::INFO, "mains task High Water Mark:",
                 uxTaskGetStackHighWaterMark(NULL));
      vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
      vTaskSuspend(NULL);
    }
  }
  vTaskDelete(NULL);
}

// // ISR for UPS power gain
void TestManager::onUPSPowerGainTask(void* pvParameters) {

  while (true) {
    if (xSemaphoreTake(upsGain, portMAX_DELAY)) {

      if (switchTest) {
        switchTest->stopTestCapture();
        logger.log(LogLevel::INTR, "UPS Powerloss triggered...");
        ups_triggered = false;
      }
      logger.log(LogLevel::INFO,
                 "UPS High Water Mark:", uxTaskGetStackHighWaterMark(NULL));

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
