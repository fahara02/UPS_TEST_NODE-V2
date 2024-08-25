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
  INTR,
};
static const char* etaskStatetoString(eTaskState state) {

  switch (state) {
    case eTaskState::eBlocked:
      return "Blocked";
    case eTaskState::eDeleted:
      return "Deleted";
    case eTaskState::eInvalid:
      return "Invalid";
    case eTaskState::eReady:
      return "Ready";
    case eTaskState::eRunning:
      return "Running";
    case eTaskState::eSuspended:
      return "Suspended";
    default:
      return "Unknown";
  }
}
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

  String formatLogLevel(LogLevel level) {
    static bool toggleColor = false;

    switch (level) {
      case LogLevel::INFO:
        toggleColor = !toggleColor;
        return toggleColor ? "\033[34m[INFO] "
                           : "\033[38;5;94m[INFO] ";  // Alternate between Blue
                                                      // and Brown text
      case LogLevel::WARNING:
        return "\033[33m[WARNING] ";  // Yellow text
      case LogLevel::ERROR:
        return "\033[31m[ERROR] ";  // Red text
      case LogLevel::SUCCESS:
        return "\033[32m[SUCCESS] ";  // Green text
      case LogLevel::TEST:
        return "\033[33m[TEST] ";  // Cyan text
      case LogLevel::INTR:
        return "\033[31m[INTERUUPT] ";  // Red text
      default:
        return "\033[0m";  // Default to no color
    }
  }

  // Log function with formatted output
  void log(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    String output = formatLogLevel(level);
    output += buffer;
    output += "\033[0m";

    Serial.println(output);
  }
  void log(LogLevel level, eTaskState state) {
    log(level, etaskStatetoString(state));
  }
  void log(LogLevel level, int number) { log(level, "%d", number); }

  // Overload for unsigned integers
  void log(LogLevel level, unsigned int number) { log(level, "%u", number); }

  // Overload for long
  void log(LogLevel level, long number) { log(level, "%ld", number); }

  // Overload for unsigned long
  void log(LogLevel level, unsigned long number) { log(level, "%lu", number); }

  // Overload for floats
  void log(LogLevel level, float number) { log(level, "%.2f", number); }

  // Overload for doubles
  void log(LogLevel level, double number) { log(level, "%.2lf", number); }

  // Overloaded log function for combined string and number logging
  void log(LogLevel level, const char* message, int number) {
    log(level, "%s %d", message, number);
  }

  // Overloaded log function for unsigned int
  void log(LogLevel level, const char* message, unsigned int number) {
    log(level, "%s %u", message, number);
  }

  // Overloaded log function for long
  void log(LogLevel level, const char* message, long number) {
    log(level, "%s %ld", message, number);
  }

  // Overloaded log function for unsigned long
  void log(LogLevel level, const char* message, unsigned long number) {
    log(level, "%s %lu", message, number);
  }

  // Overloaded log function for float
  void log(LogLevel level, const char* message, float number) {
    log(level, "%s %.2f", message, number);
  }

  // Overloaded log function for double
  void log(LogLevel level, const char* message, double number) {
    log(level, "%s %.2lf", message, number);
  }

  // Log function for binary output
  void logBinary(LogLevel level, uint32_t result) {
    String output = formatLogLevel(level);
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