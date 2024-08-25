#include "TestManager.h"
#include "Logger.h"
#include "TestData.h"

using namespace Node_Core;
extern Logger& logger;

// extern TestSync& UPSTestSync;

extern volatile bool mains_triggered;
extern volatile bool ups_triggered;

TestManager* TestManager::instance = nullptr;

TestManager::TestManager()
    : _initialized(false), _newEventTrigger(false), _setupUpdated(false) {

  stateMachine = StateMachine::getInstance();
  if (stateMachine) {
    logger.log(LogLevel::SUCCESS, "State machine is on!");
    _currentstate = stateMachine->getCurrentState();
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
  createTestTasks();

  pauseAllTest();
  createManagerTasks();

  _initialized = true;  // Mark as initialized
}

void TestManager::addTests(RequiredTest testList[], int numTest) {

  if (numTest > MAX_TESTS) {
    logger.log(LogLevel::ERROR, "Maximum Test Limit exeeded", MAX_TESTS);
    return;
  }
  for (int i = 0; i < numTest; ++i) {

    logger.log(LogLevel::INFO, "Test No: ", testList[i].TestNo);
    logger.log(LogLevel::INFO, "Test Type:%s ",
               testTypeToString(testList[i].testtype));
    logger.log(LogLevel::INFO, "Load Level:%s ",
               loadPercentageToString(testList[i].loadlevel));

    if (testList[i].testtype == TestType::SwitchTest) {

      testsSW[_numSwitchTest].testinstance = switchTest;
      testsSW[_numSwitchTest].testRequired = testList[i];
      testsSW[_numSwitchTest].testData = SwitchTestData();
      logger.log(LogLevel::SUCCESS, "Added test",
                 testTypeToString(testList[i].testtype));

      _addedSwitchTest = true;
      _numSwitchTest++;
    } else {
      logger.log(LogLevel::WARNING, "Other Test still not available");
    }
  }
}

void TestManager::pauseAllTest() {
  if (switchTest && switchTestTaskHandle != NULL) {

    logger.log(LogLevel::WARNING, "Pausing Switch test task");
    vTaskSuspend(switchTestTaskHandle);
    logger.log(LogLevel::WARNING, "after pausing");
    eTaskState state = eTaskGetState(switchTestTaskHandle);
    logger.log(LogLevel::INFO, "SwitchTest task state: %s",
               etaskStatetoString(state));
  }
}
void TestManager::triggerEvent(Event event) {

  instance->stateMachine->handleEvent(event);
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

  gpio_isr_handler_add(mainpowerPin, keyISR1, NULL);
  gpio_isr_handler_add(upspowerupPin, keyISR2, NULL);
  gpio_isr_handler_add(upsshutdownPin, keyISR3, NULL);
}
void TestManager::createManagerTasks() {
  logger.log(LogLevel::WARNING, "inside manager createtask function ");

  xTaskCreatePinnedToCore(TestManagerTask, "MainsTestManager", 12000, NULL, 2,
                          &TestManagerTaskHandle, 0);

  eTaskState state = eTaskGetState(TestManagerTaskHandle);
  logger.log(LogLevel::INFO, "manager task state is: %s",
             etaskStatetoString(state));
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

  logger.log(LogLevel::SUCCESS, "All ISR task Created");
}
void TestManager::createTestTasks() {
  if (switchTest) {
    SetupTaskParams taskParam;
    taskParam.task_TestVARating = _cfgTaskParam.task_TestVARating;
    taskParam.task_testDuration_ms = _cfgTaskParam.task_testDuration_ms;
    logger.log(LogLevel::INFO, "Creating Switchtest task ");

    xTaskCreatePinnedToCore(switchTest->MainTestTask, "MainsTestManager",
                            _cfgTask.mainTest_taskStack, &taskParam,
                            _cfgTask.mainTest_taskIdlePriority,
                            &switchTestTaskHandle, _cfgTask.mainTest_taskStack);

    eTaskState state = eTaskGetState(switchTestTaskHandle);
    logger.log(LogLevel::INFO, "SwitchTest task state: %s",
               etaskStatetoString(state));
  }
}

void TestManager::TestManagerTask(void* pvParameters) {

  // managerTaskParam* taskparam = (managerTaskParam*)pvParameters;

  State currentState = instance->stateMachine->getCurrentState();

  logger.log(LogLevel::INFO, "Resuming Test Manger task");
  while (true) {

    currentState = instance->stateMachine->getCurrentState();
    LogLevel logLevel = LogLevel::INFO;

    if (currentState == State::DEVICE_READY) {
      logger.log(LogLevel::INFO, "Device is ready. Logging pending tests...");

      eTaskState state = eTaskGetState(switchTestTaskHandle);
      logger.log(LogLevel::INFO, "SwitchTest task state: %s",
                 etaskStatetoString(state));

      int numswTest = instance->_numSwitchTest;
      for (int i = 0; i < numswTest; ++i) {
        TestManagerStatus managerStatus
            = instance->testsSW[i].testRequired.testStatus.managerStatus;
        TestOperatorStatus operatorStatus
            = instance->testsSW[i].testRequired.testStatus.operatorStatus;
        if (managerStatus == TestManagerStatus::PENDING
            && operatorStatus == TestOperatorStatus::NOT_STARTED) {

          LoadPercentage load = instance->testsSW[i].testRequired.loadlevel;
          TestType type = instance->testsSW[i].testRequired.testtype;

          // Log the pending test information
          logger.log(logLevel, "Pending Test Load level:%s ",
                     loadPercentageToString(load));
          logger.log(logLevel, "Pending Test name:%s ", testTypeToString(type));

          instance->testsSW[i].testRequired.testStatus.operatorStatus
              = TestOperatorStatus::RUNNING;
          logger.log(logLevel, "SwitchTest list is updated now.");
        }
      }

      vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (currentState == State::TEST_START) {
      logger.log(LogLevel::INFO, "starting switch test process");
      if (switchTest) {
        switchTest->_cfgTest.testVARating = 1000;
        switchTest->_cfgTest.testDuration_ms = 5000;
        logger.log(LogLevel::INFO, "starting switch test 25 percent  load");

        eTaskState estate;
        logger.log(LogLevel::INFO, "checking task state");
        estate = eTaskGetState(switchTestTaskHandle);

        if (estate == eSuspended) {
          logger.log(LogLevel::INFO, "resuming blocked task");
          vTaskResume(switchTestTaskHandle);

          vTaskPrioritySet(&switchTestTaskHandle, 3);

          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }

      vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
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

  switchTest = SwitchTest::getInstance();
  if (switchTest) {
    switchTest->init();
  }
}
