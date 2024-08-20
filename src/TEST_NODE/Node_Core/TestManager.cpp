#include "TestManager.h"
#include "TestData.h"

extern volatile bool mains_triggered;
extern volatile bool ups_triggered;
extern TaskHandle_t ISR_MAINS_POWER_LOSS;
extern TaskHandle_t ISR_UPS_POWER_GAIN;
extern TaskHandle_t ISR_UPS_POWER_LOSS;

TestManager::TestManager() {}

TestManager::~TestManager() {}

void TestManager::init() {
  if (_initialized) {
    return;  // Already initialized, do nothing
  }
  setupPins();
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
                          _cfgTask.mainTest_taskStack, &_cfgTaskParam,
                          _cfgTask.mainTest_taskIdlePriority,
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
void TestManager::TestManagerTask(void* pvParameters) { vTaskDelete(NULL); }
void TestManager::onMainsPowerLossTask(void* pvParameters) {
  // while (true) {
  //   instance[] if (mains_triggered) {
  //     if (true) {
  //       instance->_time_capture_running = true;
  //       instance->startTimeCapture();
  //       Serial.print("\033[31m");  // Start red color
  //       Serial.print("mains Powerloss triggered...");
  //       Serial.print("\033[0m");  // Reset color
  //       mains_triggered = false;
  //     }
  //     Serial.print("mains task High Water Mark: ");
  //     Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack
  //     usage

  //         vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
  //     vTaskSuspend(NULL);
  //   }
  // }
  vTaskDelete(NULL);
}

// // ISR for UPS power gain
void TestManager::onUPSPowerGainTask(void* pvParameters) {

  // while (true) {
  //   if (ups_triggered) {

  //     if (instance) {
  //       instance->stopTimeCapture();
  //       Serial.print("\033[31m");  // Start red color
  //       Serial.print("UPS Powerloss triggered...");
  //       Serial.print("\033[0m");  // Reset color
  //       ups_triggered = false;
  //     }
  //     Serial.print("UPS High Water Mark: ");
  //     Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack
  //     usage

  //         vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
  //     vTaskSuspend(NULL);
  //   }
  // }
  vTaskDelete(NULL);
}

void TestManager::onUPSPowerLossTask(void* pvParameters) { vTaskDelete(NULL); }

void TestManager::initializeTestInstances() {
  // Initialize SwitchTest

  testsSwitch[0].test = SwitchTest::getInstance();
  testsSwitch[0].status = TestStatus::TEST_PENDING;
  testsSwitch[0].data = testsSwitch[0].test->data();
  testsSwitch[0].test->init();

  numTests = MAX_TESTS;  // Set the number of tests
}
