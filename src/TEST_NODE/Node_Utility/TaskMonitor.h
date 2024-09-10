#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "HPTSettings.h"

namespace Node_Utility
{

// Define your logging tags
#define TAG "TaskMonitor"

// Logging macros
#define LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
#define LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
#define LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
#define LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)

static constexpr int MAX_TASK = 15;
static constexpr UBaseType_t TASK_MONITOR_STACK_FREE_MIN = 1024; // Define as needed

class TaskMonitor
{
  private:
	struct TaskInfo
	{
		const char* name; // Task name
		TaskHandle_t handle; // FreeRTOS task handle
		UBaseType_t stackHighWaterMark; // Stack high-water mark
		eTaskState state; // Task state
	};

	TaskInfo tasks[MAX_TASK]; // Static array to store task info
	size_t taskCount; // Number of tasks added

	uint32_t printDelay; // Delay in milliseconds for printing
	SemaphoreHandle_t monitorMutex; // Mutex for thread safety
	TaskHandle_t monitorTaskHandle; // Handle for the monitor task

	// Private constructor for singleton
	TaskMonitor() : taskCount(0), printDelay(1000)
	{
		monitorMutex = xSemaphoreCreateMutex(); // Create a mutex
		xTaskCreatePinnedToCore(monitorTask, // Task function
								"MonitorTask", // Name of the task
								monitor_Stack, // Stack size in bytes
								this, // Parameter to pass (this instance)
								monitor_Priority, // Task priority
								&monitorTaskHandle, // Task handle
								monitor_CORE // Core to run the task on (1 for APP CPU on ESP32)
		);
	}

	// Background task function
	static void monitorTask(void* pvParameters)
	{
		TaskMonitor* self = static_cast<TaskMonitor*>(pvParameters);
		for(;;)
		{
			self->updateTaskInfo();
			self->printTaskInfo();
			vTaskDelay(1);
			vTaskDelay(self->printDelay / portTICK_PERIOD_MS); // Delay based on print interval
		}
	}

	// Update the high-water mark and state for all tasks
	void updateTaskInfo()
	{
		xSemaphoreTake(monitorMutex, portMAX_DELAY); // Lock
		for(size_t i = 0; i < taskCount; i++)
		{
			TaskHandle_t handle = xTaskGetHandle(tasks[i].name);
			if(handle != nullptr)
			{
				tasks[i].handle = handle;
				tasks[i].stackHighWaterMark = uxTaskGetStackHighWaterMark(handle);
				tasks[i].state = eTaskGetState(handle);
			}
		}
		xSemaphoreGive(monitorMutex); // Unlock
	}

	void printTaskInfo()
	{
		for(size_t i = 0; i < taskCount; i++)
		{
			const TaskInfo& task = tasks[i];
			if(task.handle)
			{
				const UBaseType_t size = task.stackHighWaterMark;
				const eTaskState state = task.state;
				const char* stateStr = getTaskStateString(state);
				const UBaseType_t priority = uxTaskPriorityGet(task.handle);

				if(size < TASK_MONITOR_STACK_FREE_MIN)
				{
					LOGW(TAG, "%-15s (p=%u) %s %u bytes", task.name, priority, stateStr, size);
				}
				else
				{
					LOGI(TAG, "%-15s (p=%u) %s %u bytes", task.name, priority, stateStr, size);
				}
			}
			else
			{
				LOGE(TAG, "%-15s Handle not found", task.name);
			}
		}
	}

	// Convert task state to a human-readable string
	static const char* getTaskStateString(eTaskState state)
	{
		switch(state)
		{
			case eRunning:
				return "Running";
			case eReady:
				return "Ready";
			case eBlocked:
				return "Blocked";
			case eSuspended:
				return "Suspended";
			case eDeleted:
				return "Deleted";
			default:
				return "Unknown";
		}
	}

  public:
	// Get the singleton instance
	static TaskMonitor& getInstance()
	{
		static TaskMonitor
			instance; // Meyers' singleton - instantiated on first use, guaranteed to be destroyed
		return instance;
	}

	// Add a task by name
	void addTask(const char* name)
	{
		xSemaphoreTake(monitorMutex, portMAX_DELAY); // Lock
		if(taskCount < MAX_TASK)
		{ // Prevent overflow
			tasks[taskCount++] = {name, nullptr, 0, eInvalid};
		}
		xSemaphoreGive(monitorMutex); // Unlock
	}

	// Set the print delay
	void setPrintDelay(uint32_t delayMs)
	{
		printDelay = delayMs;
	}
};

} // namespace Node_Utility

#endif // TASK_MONITOR_H
