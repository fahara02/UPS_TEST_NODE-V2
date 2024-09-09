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

// ALL task Priority
static UBaseType_t AsyncTCP_Priority = 10;
static UBaseType_t wsDataProcessor_Priority = 4;
static UBaseType_t periodicDataSender_Priority = 1;
static UBaseType_t WSCleanup_Priority = 5;
static UBaseType_t userCommand_Priority = 4;
static UBaseType_t userUpdate_Priority = 4;
static UBaseType_t testSync_Priority = 1;

// ALL task Core
static BaseType_t AsyncTCP_CORE = tskNO_AFFINITY;
static BaseType_t wsDataProcessor_CORE = tskNO_AFFINITY;
static BaseType_t periodicDataSender_CORE = 1;
static BaseType_t WSCleanup_CORE = 0;
static BaseType_t userCommand_CORE = 1;
static BaseType_t userUpdate_CORE = 1;
static BaseType_t testSync_CORE = 1;

#endif