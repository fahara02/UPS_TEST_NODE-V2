#ifndef HPT_SETTINGS_H
#define HPT_SETTINGS_H
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
// ALL CRITICAL TASK STACK SIZE
static const uint32_t AsyncTCP_Stack = 19456;
static const uint32_t wsDataProcessor_Stack = 8192;
static const uint32_t periodicDataSender_Stack = 8192;
static const uint32_t WSCleanup_Stack = 4096;

static const uint32_t userCommand_Stack = 8192;
static const uint32_t userUpdate_Stack = 8192;
static const uint32_t testSync_Stack = 4096;

static const uint32_t TestManager_Stack = 4096;
static const uint32_t MainLossISR_Stack = 1024;
static const uint32_t UPSgainISR_Stack = 1024;
static const uint32_t UPSlossISR_Stack = 1024;
static const uint32_t switchTest_Stack = 4096;
static const uint32_t backupTest_Stack = 4096;

static const uint32_t monitor_Stack = 2048;
static const uint32_t modbus_Stack = 4096;
static const uint32_t timer_Stack = 4096;
// ALL task Priority
static const UBaseType_t AsyncTCP_Priority = 10;
static const UBaseType_t wsDataProcessor_Priority = 6;
static const UBaseType_t periodicDataSender_Priority = 2;
static const UBaseType_t WSCleanup_Priority = 5;

static const UBaseType_t userCommand_Priority = 4;
static const UBaseType_t userUpdate_Priority = 4;
static const UBaseType_t testSync_Priority = 3;

static const UBaseType_t TestManager_Priority = 3;
static const UBaseType_t MainLossISR_Priority = 1;
static const UBaseType_t UPSgainISR_Priority = 1;
static const UBaseType_t UPSloss_Priority = 1;
static const UBaseType_t SwitchTest_Priority = 2;
static const UBaseType_t BackUpTest_Priority = 2;

static const UBaseType_t monitor_Priority = 1;
static const UBaseType_t modbus_Priority = 1;
static const UBaseType_t timer_Priority = 1;
// ALL task Core
static const BaseType_t AsyncTCP_CORE = 1;
static const BaseType_t wsDataProcessor_CORE = tskNO_AFFINITY;
static const BaseType_t periodicDataSender_CORE = tskNO_AFFINITY;
static const BaseType_t WSCleanup_CORE = 0;

static const BaseType_t userCommand_CORE = 0;
static const BaseType_t userUpdate_CORE = 0;
static const BaseType_t testSync_CORE = 0;

static const BaseType_t testManager_CORE = 1;
static const BaseType_t MainLossISR_CORE = 1;
static const BaseType_t UPSgainISR_CORE = 1;
static const BaseType_t UPSloss_CORE = 1;
static const BaseType_t SwitchTest_CORE = 1;
static const BaseType_t BackUpTest_CORE = 1;

static const BaseType_t monitor_CORE = 1;
static const BaseType_t modbus_CORE = tskNO_AFFINITY;
static const BaseType_t timer_CORE = 0;
#endif