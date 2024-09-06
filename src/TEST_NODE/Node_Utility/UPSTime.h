#ifndef UPS_TIME_H
#define UPS_TIME_H

#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"

// NTP server configurations
const char* NTP_SERVER = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const char* TZ_INFO = "<+06>-6"; // Timezone info for GMT+6
const long gmtOffset_sec = 21600;
const long daylightOffset_sec = 0;

tm timeinfo;
time_t now;
unsigned long lastNTPtime;
unsigned long lastEntryTime;
unsigned long show_time_period_ms = 60000;

// Function to configure NTP and time zone
void setupTime()
{
	// Configure the NTP server and time zone
	configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER, ntpServer2);
	setenv("TZ", TZ_INFO, 1);
	tzset(); // Apply the timezone settings
}

// Function to get NTP time, retry for the given number of seconds
bool getNTPtime(int sec)
{
	unsigned long start = millis();
	while((millis() - start) <= (1000 * sec))
	{
		time(&now); // Get the current time
		localtime_r(&now, &timeinfo); // Convert to local time
		if(timeinfo.tm_year >= (2016 - 1900)) // Check if NTP sync was successful
		{
			lastNTPtime = now; // Store the time for reference
			Serial.println("NTP sync successful.");
			return true;
		}
		Serial.print(".");
		delay(500); // Use smaller delay to avoid blocking for too long
	}

	Serial.println("NTP sync failed.");
	return false;
}

// Function to display the time
void showTime(const tm& localTime)
{
	char time_output[30];
	strftime(time_output, sizeof(time_output), "%a  %d-%m-%y %T", &localTime);
	Serial.println(time_output);
}

// Task to continuously keep time and display it
void TimeKeeperTask(void* pvParameters)
{
	while(true)
	{
		unsigned long start_show_time = millis();
		// Only sync NTP every hour to avoid unnecessary network load
		if(millis() - lastNTPtime > 3600000UL || lastNTPtime == 0) // 1 hour
		{
			if(getNTPtime(10))
			{
				showTime(timeinfo); // Show the time after successful sync
			}
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
	}

	vTaskDelete(NULL);
}

#endif
