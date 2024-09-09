#ifndef HPT_SETTINGS_H
#define HPT_SETTINGS_H
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
// ALL CRITICAL TASK STACK SIZE

static constexpr int wsDataProcessor_Stack = 8192;
static constexpr int periodicDataSender_Stack = 4096;
static const uint32_t userCommand_Stack = 8192;
static const uint32_t userUpdate_Stack = 8192;
static const uint32_t testSync_Stack = 4096;

// ALL task Priority

UBaseType_t userCommand_Priority = 4;
UBaseType_t userUpdate_Priority = 4;
UBaseType_t testSync_Priority = 1;
// ALL task Core
BaseType_t userCommand_CORE = 1;
BaseType_t userUpdate_CORE = 1;
BaseType_t testSync_CORE = 1;

#endif