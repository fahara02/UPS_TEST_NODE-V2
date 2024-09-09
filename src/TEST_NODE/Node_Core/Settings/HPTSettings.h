#ifndef HPT_SETTINGS_H
#define HPT_SETTINGS_H
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
// ALL CRITICAL TASK STACK SIZE
static const uint32_t AsyncTCP_Stack = 39936;
static const uint32_t wsDataProcessor_Stack = 8192;
static const uint32_t periodicDataSender_Stack = 4096;
static const uint32_t WSCleanup_Stack = 4096;

static const uint32_t userCommand_Stack = 8192;
static const uint32_t userUpdate_Stack = 8192;
static const uint32_t testSync_Stack = 4096;

static const uint32_t TestManager_Stack = 4096;
static const uint32_t MainLossISR_Stack = 1024;
static const uint32_t UPSgainISR_Stack = 1024;
static const uint32_t UPSlossISR_Stack = 1024;

// ALL task Priority
static UBaseType_t AsyncTCP_Priority = 10;
static UBaseType_t wsDataProcessor_Priority = 4;
static UBaseType_t periodicDataSender_Priority = 1;
static UBaseType_t WSCleanup_Priority = 5;

static UBaseType_t userCommand_Priority = 4;
static UBaseType_t userUpdate_Priority = 4;
static UBaseType_t testSync_Priority = 1;

static UBaseType_t TestManager_Priority = 2;
static UBaseType_t MainLossISR_Priority = 5;
static UBaseType_t UPSgainISR_Priority = 5;
static UBaseType_t UPSloss_Priority = 5;

// ALL task Core
static BaseType_t AsyncTCP_CORE = tskNO_AFFINITY;
static BaseType_t wsDataProcessor_CORE = tskNO_AFFINITY;
static BaseType_t periodicDataSender_CORE = 1;
static BaseType_t WSCleanup_CORE = 0;

static BaseType_t userCommand_CORE = 1;
static BaseType_t userUpdate_CORE = 1;
static BaseType_t testSync_CORE = 1;

static BaseType_t testManager_CORE = 1;
static BaseType_t MainLossISR_CORE = 1;
static BaseType_t UPSgainISR_CORE = 1;
static BaseType_t UPSloss_CORE = 1;

#endif