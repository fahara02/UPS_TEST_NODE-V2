#ifndef LOGGER_H
#define LOGGER_H
#include "StateMachine.h"
#include "UPSError.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace Node_Core {
enum class LogLevel {
  INFO,
  WARNING,
  ERROR,
  SUCCESS,
  TEST,
};

class Logger {
public:
  // Get the singleton instance
  static Logger& getInstance() {
    static Logger instance;
    return instance;
  }

  // Initialize the logger
  void init() {
    Serial.begin(115200);
    xTaskCreate(logTask, "LoggerTask", 4096, nullptr, 1, nullptr);
    this->log(LogLevel::INFO, "Logtask started");
  }

  // Log function with formatted output
  void log(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    String output;

    switch (level) {
      case LogLevel::INFO:
        output = "\033[34m[INFO] ";  // Blue text
        break;
      case LogLevel::WARNING:
        output = "\033[33m[WARNING] ";  // Yellow text
        break;
      case LogLevel::ERROR:
        output = "\033[31m[ERROR] ";  // Red text
        break;
      case LogLevel::SUCCESS:
        output = "\033[32m[SUCCESS] ";  // Green text
        break;
      case LogLevel::TEST:
        output = "";  // We will handle this case separately below
        break;
      default:
        output = "\033[0m";  // Default to no color
        break;
    }

    if (level == LogLevel::TEST) {
      const char* colors[] = {
          // "\033[31m[TEST]",  // Red
          "\033[33m",  // Yellow
                       // "\033[32m",  // Green
          "\033[36m",  // Cyan
                       //  "\033[34m",  // Blue
          "\033[35m"   // Magenta
      };
      int colorIndex = 0;
      int colorsCount = sizeof(colors) / sizeof(colors[0]);

      for (size_t i = 0; i < strlen(buffer); i++) {
        output += colors[colorIndex];
        output += buffer[i];
        colorIndex = (colorIndex + 1) % colorsCount;
      }
      output += "\033[0m[TEST]";  // Reset color
    } else {
      output += buffer;
      output += "\033[0m[TEST]";  // Reset color
    }

    Serial.println(output);
  }

  // Log function for binary output
  void logBinary(LogLevel level, uint32_t result) {
    String output;
    switch (level) {
      case LogLevel::INFO:
        output = "\033[34m[INFO] ";  // Blue text
        break;
      case LogLevel::WARNING:
        output = "\033[33m[WARNING] ";  // Yellow text
        break;
      case LogLevel::ERROR:
        output = "\033[31m[ERROR] ";  // Red text
        break;
      case LogLevel::SUCCESS:
        output = "\033[32m[SUCCESS] ";  // Green text
        break;
      case LogLevel::TEST:
        output = "\033[36m[TEST] ";  // Green text
        break;
    }
    output += "Result in binary: ";
    Serial.print(output);
    Serial.println(result, BIN);  // Print the result in binary format
    Serial.println("\033[0m");    // Reset color
  }
  // Log error from UPSError
  void logError(UPSError error) {
    switch (error) {
      case UPSError::USER_OK:
        log(LogLevel::INFO, "Sucess!");
        break;
      case UPSError::INIT_FAILED:
        log(LogLevel::ERROR, "Initialisation failed");
        break;
      case UPSError::FS_FAILED:
        log(LogLevel::ERROR, "File System failed");
        break;
      case UPSError::INVALID_DATA:
        log(LogLevel::ERROR, "Test Data Invalid");
        break;
      case UPSError::SENSOR_FAILURE:
        log(LogLevel::ERROR, "Sensor failure detected.");
        break;
      case UPSError::CONNECTION_TIMEOUT:
        log(LogLevel::ERROR, "Connection timeout occurred.");
        break;
      default:
        log(LogLevel::ERROR, "Undefined Error.");
    }
  }

  // Register a callback function for specific errors
  void registerErrorCallback(void (*callback)(UPSError)) {
    errorCallback = callback;
  }

  // Invoke the callback when a specific error occurs
  void triggerErrorCallback(UPSError error) {
    if (errorCallback) {
      errorCallback(error);
    }
  }

private:
  // Private constructor for singleton pattern
  Logger() : errorCallback(nullptr) {}

  // Logger task running in FreeRTOS
  static void logTask(void* pvParameters) {
    while (true) {
      // Log task logic (e.g., handling log queue, etc.)
      vTaskDelay(pdMS_TO_TICKS(1000));  // Example delay
    }
  }

  // Callback function pointer
  void (*errorCallback)(UPSError);
};

}  // namespace Node_Core
#endif