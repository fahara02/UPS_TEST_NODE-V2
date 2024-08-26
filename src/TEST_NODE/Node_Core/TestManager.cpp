#include "TestManager.h"
#include "Logger.h"
#include "TestData.h"
#include "TestSync.h"

using namespace Node_Core;
extern Logger& logger;
extern TestSync& SyncTest;

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
  instance->UpdateSettings();
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
  createManagerTasks();
  createTestTasks();

  // pauseAllTest();

  _initialized = true;  // Mark as initialized
}

void TestManager::UpdateSettings() {
  if (TesterSetup) {
    _cfgSpec = TesterSetup->specSetup();
    _cfgTest = TesterSetup->testSetup();
    _cfgTask = TesterSetup->taskSetup();
    _cfgTaskParam = TesterSetup->paramSetup();
    _cfgHardware = TesterSetup->hardwareSetup();
  };
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

  xTaskCreatePinnedToCore(TestManagerTask, "MainsTestManager",
                          _cfgTask.mainTest_taskStack, NULL, 2,
                          &TestManagerTaskHandle, _cfgTask.mainTest_taskCore);
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

    logger.log(LogLevel::INFO, "Creating Switchtest task ");

    xQueueSend(TestManageQueue, &_cfgTaskParam, 100);

    xTaskCreatePinnedToCore(switchTest->MainTestTask, "MainsTestManager", 12000,
                            NULL, _cfgTask.mainTest_taskIdlePriority,
                            &switchTestTaskHandle, 0);

    eTaskState state = eTaskGetState(switchTestTaskHandle);
    logger.log(LogLevel::INFO, "SwitchTest task state: %s",
               etaskStatetoString(state));
  }
}
// void TestManager::TestManagerTask(void* pvParameters) {
//   State currentState = instance->stateMachine->getCurrentState();
//   logger.log(LogLevel::INFO, "Resuming Test Manager task");

//   while (true) {
//     currentState = instance->stateMachine->getCurrentState();

//     if (currentState == State::DEVICE_READY) {
//       logger.log(LogLevel::INFO, "Device is ready. Checking pending
//       tests...");

//       for (int i = 0; i < instance->_numSwitchTest; ++i) {
//         if (instance->isTestPendingAndNotStarted(instance->testsSW[i])) {
//           instance->logPendingSwitchTest(instance->testsSW[i]);

//           // Trigger AUTO_TEST_CMD and INPUT_OUTPUT_READY sequentially
//           instance->triggerEvent(Event::AUTO_TEST_CMD);
//           vTaskDelay(pdMS_TO_TICKS(100));
//           instance->triggerEvent(Event::INPUT_OUTPUT_READY);
//         }
//       }
//     }

//     if (currentState == State::TEST_START) {

//       logger.log(LogLevel::INFO, "Manager Task under test start phase");

//       eTaskState estate = eTaskGetState(switchTestTaskHandle);
//       logger.log(LogLevel::INFO, "SwitchTest task state: %s",
//                  etaskStatetoString(estate));

//       for (int i = 0; i < instance->_numSwitchTest; ++i) {
//         if (instance->isTestPendingAndNotStarted(instance->testsSW[i])) {
//           LoadPercentage load = instance->testsSW[i].testRequired.loadlevel;

//           instance->configureSwitchTest(load);
//           // vTaskDelay(pdMS_TO_TICKS(200));

//           eTaskState estate = eTaskGetState(switchTestTaskHandle);
//           logger.log(LogLevel::INFO, "SwitchTest task state: %s",
//                      etaskStatetoString(estate));

//           SyncTest.startTest(TestType::SwitchTest);
//           vTaskPrioritySet(switchTestTaskHandle,
//                            instance->_cfgTask.mainTest_taskIdlePriority + 2);

//           if (switchTest->_dataCaptureOk) {
//             logger.log(LogLevel::SUCCESS,
//                        "Successfull data capture , updating status");

//             instance->testsSW[i].testRequired.testStatus.managerStatus
//                 = TestManagerStatus::DONE;
//             instance->testsSW[i].testRequired.testStatus.operatorStatus
//                 = TestOperatorStatus::SUCCESS;

//             logger.log(LogLevel::WARNING, "Pausing SwitchTest task");

//             SyncTest.stopTest(TestType::SwitchTest);
//             eTaskState estate = eTaskGetState(switchTestTaskHandle);
//             logger.log(LogLevel::INFO, "SwitchTest task state: %s",
//                        etaskStatetoString(estate));
//           }
//           vTaskDelay(pdMS_TO_TICKS(instance->_cfgTaskParam.task_testDuration_ms
//                                    + 100));
//         }
//       }
//     }
//     if (currentState == State::TEST_IN_PROGRESS) {
//     }

//     if (currentState == State::CURRENT_TEST_OK) {
//     }

//     if (currentState == State::READY_NEXT_TEST) {
//     }
//     vTaskDelay(pdMS_TO_TICKS(100));
//   }

//   vTaskDelete(NULL);
// }

void TestManager::TestManagerTask(void* pvParameters) {
  logger.log(LogLevel::INFO, "Resuming Test Manager task");

  while (true) {
    State currentState = instance->stateMachine->getCurrentState();

    if (currentState == State::DEVICE_READY) {
      logger.log(LogLevel::INFO, "Device is ready. Checking pending tests...");
    }

    for (int i = 0; i < instance->_numSwitchTest; ++i) {
      if (instance->isTestPendingAndNotStarted(instance->testsSW[i])) {
        currentState = instance->stateMachine->getCurrentState();

        if (currentState == State::DEVICE_READY) {
          instance->logPendingSwitchTest(instance->testsSW[i]);
          instance->triggerEvent(Event::AUTO_TEST_CMD);
          vTaskDelay(pdMS_TO_TICKS(50));

        } else if (currentState == State::AUTO_MODE) {
          vTaskDelay(pdMS_TO_TICKS(50));
          instance->triggerEvent(Event::PENDING_TEST_FOUND);
          vTaskDelay(pdMS_TO_TICKS(50));
        }

        else if (currentState == State::TEST_START) {
          logger.log(LogLevel::INFO, "Manager Task under test start phase");

          LoadPercentage load = instance->testsSW[i].testRequired.loadlevel;
          instance->configureSwitchTest(load);
          eTaskState estate = eTaskGetState(switchTestTaskHandle);
          logger.log(LogLevel::INFO, "SwitchTest task state: %s",
                     etaskStatetoString(estate));
          vTaskPrioritySet(switchTestTaskHandle,
                           instance->_cfgTask.mainTest_taskIdlePriority + 2);

          logger.log(LogLevel::INFO, "Starting SwitchTest...");
          SyncTest.startTest(TestType::SwitchTest);
          vTaskDelay(pdMS_TO_TICKS(100));
        }

        else if (currentState == State::TEST_IN_PROGRESS) {

          logger.log(LogLevel::INFO, "Manager observing running test");

          if (switchTest->_dataCaptureOk) {
            logger.log(LogLevel::SUCCESS, "Successful data capture.");
          }

          vTaskDelay(pdMS_TO_TICKS(instance->_cfgTaskParam.task_testDuration_ms
                                   + 100));
        } else if (currentState == State::CURRENT_TEST_CHECK) {
          logger.log(
              LogLevel::INFO,
              "In check State  checking either test ended or data captured");
          if (switchTest->_triggerTestEndEvent) {
            logger.log(LogLevel::INFO, "test Cycle ended ");
          }
          if (switchTest->_dataCaptureOk) {
            logger.log(LogLevel::SUCCESS, "Successful data capture.");
          }

          vTaskDelay(pdMS_TO_TICKS(instance->_cfgTaskParam.task_testDuration_ms
                                   + 100));
        } else if (currentState == State::CURRENT_TEST_OK) {
          logger.log(LogLevel::INFO,
                     "Test completed successfully. Stopping SwitchTest...");
          SyncTest.stopTest(TestType::SwitchTest);
          instance->testsSW[i].testRequired.testStatus.managerStatus
              = TestManagerStatus::DONE;
          instance->testsSW[i].testRequired.testStatus.operatorStatus
              = TestOperatorStatus::SUCCESS;
          logger.log(LogLevel::WARNING, "Triggering SAVE event from manager");
          instance->triggerEvent(Event::SAVE);

        }

        else if (currentState == State::READY_NEXT_TEST) {
          logger.log(LogLevel::INFO, "Checking for next pending test...");

          bool pendingTestFound = false;

          // Check if there are any more pending tests
          for (int j = 0; j < instance->_numSwitchTest; ++j) {
            if (instance->isTestPendingAndNotStarted(instance->testsSW[j])) {
              pendingTestFound = true;
              break;
            }
          }

          if (pendingTestFound) {
            logger.log(LogLevel::INFO,
                       "Pending test found. Preparing to start next test...");
            instance->triggerEvent(Event::PENDING_TEST_FOUND);
          } else {
            logger.log(LogLevel::INFO, "No more pending tests.");
          }
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // General delay for task
  }

  vTaskDelete(NULL);  // Delete the task when finished
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

bool TestManager::isTestPendingAndNotStarted(
    const UPSTestRun<SwitchTest, SwitchTestData>& test) {
  return test.testRequired.testStatus.managerStatus
             == TestManagerStatus::PENDING
         && test.testRequired.testStatus.operatorStatus
                == TestOperatorStatus::NOT_STARTED;
}

void TestManager::configureSwitchTest(LoadPercentage load) {
  SetupTaskParams task_Param;
  task_Param = TesterSetup->paramSetup();
  switch (load) {
    case LoadPercentage::LOAD_0P:

      task_Param.task_TestVARating = 0;
      logger.log(LogLevel::INFO, "Setup switch test for Noload");
      break;
    case LoadPercentage::LOAD_25P:

      task_Param.task_TestVARating = _cfgTest.testVARating * 25 / 100;
      logger.log(LogLevel::INFO, "Setup switch test at 25 percent load");
      break;
    case LoadPercentage::LOAD_50P:

      task_Param.task_TestVARating = _cfgTest.testVARating * 50 / 100;
      logger.log(LogLevel::INFO, "Setup switch test at 50 percent load");
      break;
    case LoadPercentage::LOAD_75P:

      task_Param.task_TestVARating = _cfgTest.testVARating * 75 / 100;
      logger.log(LogLevel::INFO, "Setup switch test at 75 percent load");
      break;
    case LoadPercentage::LOAD_100P:

      task_Param.task_TestVARating = _cfgTest.testVARating;
      logger.log(LogLevel::INFO, "Setup switch test at 100 percent load");
      break;
    default:
      logger.log(LogLevel::ERROR, "Unknown load percentage");
      break;
  }

  // Send the task parameters to the queue
  xQueueSend(TestManageQueue, &task_Param, 100);
}

void TestManager::logPendingSwitchTest(
    const UPSTestRun<SwitchTest, SwitchTestData>& test) {
  LoadPercentage load = test.testRequired.loadlevel;
  TestType type = test.testRequired.testtype;

  logger.log(LogLevel::INFO, "Pending Test Load level: %s",
             loadPercentageToString(load));
  logger.log(LogLevel::INFO, "Pending Test name: %s", testTypeToString(type));
  logger.log(LogLevel::INFO, "SwitchTest list is updated now.");
}