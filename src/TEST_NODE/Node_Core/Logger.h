#ifndef LOGGER_H
#define LOGGER_H
#include "StateMachine.h"
#include "UPSError.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <deque>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace Node_Core
{
enum class LogLevel
{
	INFO,
	WARNING,
	ERROR,
	SUCCESS,
	TEST,
	INTR,
};
static const char* etaskStatetoString(eTaskState state)
{
	switch(state)
	{
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
class Logger
{
  public:
	// Get the singleton instance
	static Logger& getInstance()
	{
		static Logger instance;
		return instance;
	}

	// Initialize the logger
	void init(Print* output = &Serial, LogLevel minLevel = LogLevel::INFO, size_t bufferSize = 10,
			  bool timestampsEnabled = false, bool enableEEPROM = false)
	{
		_output = output;
		_minLevel = minLevel; // Set the minimum log level
		_bufferSize = bufferSize;
		_timestampsEnabled = timestampsEnabled;
		_eepromEnabled = enableEEPROM;

		if(!_loggingLock)
		{
			_loggingLock = xSemaphoreCreateMutex();
		}

		if(_eepromEnabled)
		{
			EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM size
		}

		xTaskCreate(logTask, "LoggerTask", 4096, nullptr, 1, nullptr);
		this->log(LogLevel::INFO, "Logtask started");
	}
	void enableEEPROM(bool enable)
	{
		if(_loggingLock && xSemaphoreTake(_loggingLock, portMAX_DELAY) == pdTRUE)
		{
			_eepromEnabled = enable;
			if(_eepromEnabled)
			{
				EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM if enabling
			}
			xSemaphoreGive(_loggingLock);
		}
		log(LogLevel::INFO, "EEPROM storage %s", enable ? "enabled" : "disabled");
	}
	// Set minimum log level
	void setMinLogLevel(LogLevel level)
	{
		if(_loggingLock && xSemaphoreTake(_loggingLock, portMAX_DELAY) == pdTRUE)
		{
			_minLevel = level;
			xSemaphoreGive(_loggingLock);
		}
		log(LogLevel::INFO, "Minimum log level set to: %s", logLevelToString(level));
	}

	// Set output target
	void setOutput(Print* output)
	{
		if(_loggingLock && xSemaphoreTake(_loggingLock, portMAX_DELAY) == pdTRUE)
		{
			_output = output;
			xSemaphoreGive(_loggingLock);
		}
		log(LogLevel::INFO, "Output target changed.");
	}

	// Enable or disable timestamps
	void enableTimestamps(bool enable)
	{
		if(_loggingLock && xSemaphoreTake(_loggingLock, portMAX_DELAY) == pdTRUE)
		{
			_timestampsEnabled = enable;
			xSemaphoreGive(_loggingLock);
		}
	}

	// Log function with formatted output
	void log(LogLevel level, const char* format, ...)
	{
		if(level < _minLevel) // Filter out logs below the minimum level
		{
			return; // Do nothing if log level is too low
		}

		va_list args;
		va_start(args, format);
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);

		String outputStr = formatLogLevel(level);
		if(_timestampsEnabled)
		{
			outputStr += "[" + getTimestamp() + "] ";
		}
		outputStr += buffer;
		outputStr += "\033[0m"; // Reset color

		if(_output)
		{
			_output->println(outputStr);
		}

		addToBuffer(outputStr);

		// Store the log persistently if EEPROM is enabled
		if(_eepromEnabled)
		{
			storeLogToEEPROM(outputStr);
		}
	}

	String getBufferedLogs() const
	{
		String allLogs;
		if(_loggingLock && xSemaphoreTake(_loggingLock, portMAX_DELAY) == pdTRUE)
		{
			for(const String& log: _logBuffer)
			{
				allLogs += log + "\n";
			}
			xSemaphoreGive(_loggingLock);
		}
		return allLogs;
	}

	// Register a callback function for specific errors
	void registerErrorCallback(void (*callback)(UPSError))
	{
		errorCallback = callback;
	}

	// Invoke the callback when a specific error occurs
	void triggerErrorCallback(UPSError error)
	{
		if(errorCallback)
		{
			errorCallback(error);
		}
	}

	/*<---------------------------------------------> */
	/*      All Overloads of log function              */
	/*<---------------------------------------------> */
	void log(LogLevel level, eTaskState state)
	{
		log(level, etaskStatetoString(state));
	}
	void log(LogLevel level, int number)
	{
		log(level, "%d", number);
	}

	// Overload for unsigned integers
	void log(LogLevel level, unsigned int number)
	{
		log(level, "%u", number);
	}

	// Overload for long
	void log(LogLevel level, long number)
	{
		log(level, "%ld", number);
	}

	// Overload for unsigned long
	void log(LogLevel level, unsigned long number)
	{
		log(level, "%lu", number);
	}

	// Overload for floats
	void log(LogLevel level, float number)
	{
		log(level, "%.2f", number);
	}

	// Overload for doubles
	void log(LogLevel level, double number)
	{
		log(level, "%.2lf", number);
	}

	// Overloaded log function for combined string and number logging
	void log(LogLevel level, const char* message, int number)
	{
		log(level, "%s %d", message, number);
	}

	// Overloaded log function for unsigned int
	void log(LogLevel level, const char* message, unsigned int number)
	{
		log(level, "%s %u", message, number);
	}

	// Overloaded log function for long
	void log(LogLevel level, const char* message, long number)
	{
		log(level, "%s %ld", message, number);
	}

	// Overloaded log function for unsigned long
	void log(LogLevel level, const char* message, unsigned long number)
	{
		log(level, "%s %lu", message, number);
	}

	// Overloaded log function for float
	void log(LogLevel level, const char* message, float number)
	{
		log(level, "%s %.2f", message, number);
	}

	// Overloaded log function for double
	void log(LogLevel level, const char* message, double number)
	{
		log(level, "%s %.2lf", message, number);
	}

	// Log function for binary output
	void logBinary(LogLevel level, uint32_t result)
	{
		String output = formatLogLevel(level);
		output += "Result in binary: ";
		Serial.print(output);
		Serial.println(result, BIN); // Print the result in binary format
		Serial.println("\033[0m"); // Reset color
	}
	// Log error from UPSError
	void logError(UPSError error)
	{
		switch(error)
		{
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

  private:
	Print* _output;
	LogLevel _minLevel; // Minimum log level
	size_t _bufferSize;
	static const size_t BUFFER_SIZE = 10;
	bool _timestampsEnabled;
	bool _eepromEnabled; // EEPROM storage flag
	std::deque<String> _logBuffer;
	SemaphoreHandle_t _loggingLock;
	void (*errorCallback)(UPSError);
	static const int EEPROM_SIZE = 512; // Define EEPROM size

	Logger() :
		_output(&Serial), _minLevel(LogLevel::INFO), _bufferSize(BUFFER_SIZE),
		_timestampsEnabled(false), _eepromEnabled(false), errorCallback(nullptr)
	{
	}

	// Logger task running in FreeRTOS
	static void logTask(void* pvParameters)
	{
		while(true)
		{
			// Log task logic (e.g., handling log queue, etc.)
			vTaskDelay(pdMS_TO_TICKS(1000)); // Example delay
		}
	}
	void addToBuffer(const String& logEntry)
	{
		if(_loggingLock && xSemaphoreTake(_loggingLock, portMAX_DELAY) == pdTRUE)
		{
			if(_logBuffer.size() >= _bufferSize)
			{
				_logBuffer.pop_front(); // Remove the oldest entry if buffer is full
			}
			_logBuffer.push_back(logEntry);
			xSemaphoreGive(_loggingLock);
		}
	}
	// Store log entry to EEPROM
	void storeLogToEEPROM(const String& logEntry)
	{
		int address = EEPROM.read(0); // Assume the first byte stores the next write address
		if(address >= EEPROM_SIZE - logEntry.length())
		{
			address = 1; // Reset to the beginning if we exceed EEPROM size
		}

		for(int i = 0; i < logEntry.length(); ++i)
		{
			EEPROM.write(address++, logEntry[i]);
		}
		EEPROM.write(address, '\0'); // Mark the end of the log
		EEPROM.write(0, address); // Save the next write address at the start
		EEPROM.commit(); // Commit changes to EEPROM
	}

	String formatLogLevel(LogLevel level)
	{
		static bool toggleColor = false;

		switch(level)
		{
			case LogLevel::INFO:
				toggleColor = !toggleColor;
				return toggleColor ? "\033[34m[INFO] " :
									 "\033[38;5;94m[INFO] "; // Alternate between Blue
															 // and Brown text
			case LogLevel::WARNING:
				return "\033[33m[WARNING] "; // Yellow text
			case LogLevel::ERROR:
				return "\033[31m[ERROR] "; // Red text
			case LogLevel::SUCCESS:
				return "\033[32m[SUCCESS] "; // Green text
			case LogLevel::TEST:
				return "\033[33m[TEST] "; // Cyan text
			case LogLevel::INTR:
				return "\033[31m[INTERUUPT] "; // Red text
			default:
				return "\033[0m"; // Default to no color
		}
	}
	String getTimestamp()
	{
		// Replace with appropriate timestamp logic for your platform
		return String(millis() / 1000) + "s";
	}
	// Convert log level to string for logging
	const char* logLevelToString(LogLevel level)
	{
		switch(level)
		{
			case LogLevel::INFO:
				return "INFO";
			case LogLevel::WARNING:
				return "WARNING";
			case LogLevel::ERROR:
				return "ERROR";
			case LogLevel::SUCCESS:
				return "SUCCESS";
			case LogLevel::TEST:
				return "TEST";
			case LogLevel::INTR:
				return "INTERRUPT";
			default:
				return "UNKNOWN";
		}
	}
};

} // namespace Node_Core
#endif