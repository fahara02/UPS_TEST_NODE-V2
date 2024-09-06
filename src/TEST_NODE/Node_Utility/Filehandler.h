// FileHandler.h
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "FS.h"
#include "SPIFFS.h"
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>

namespace Node_Utility
{
class FileHandler
{
  public:
	FileHandler()
	{
	}

	// Initialize the filesystem
	bool begin()
	{
		if(!SPIFFS.begin(true))
		{
			Serial.println("Failed to mount SPIFFS");
			return false;
		}
		Serial.println("SPIFFS mounted successfully");
		return true;
	}

	// List all files in the filesystem
	void listFiles()
	{
		File root = SPIFFS.open("/");
		File file = root.openNextFile();

		Serial.println("Files in SPIFFS:");
		while(file)
		{
			Serial.print("  FILE: ");
			Serial.println(file.name());
			file = root.openNextFile();
		}
	}

	// Serve a specific file via the web server with cache control
	void serveFile(AsyncWebServer& server, const char* urlPath, const char* filePath,
				   const char* mimeType = "text/plain", const char* cacheControl = "max-age=86400")
	{
		server.on(
			urlPath, HTTP_GET, [filePath, mimeType, cacheControl](AsyncWebServerRequest* request) {
				AsyncWebServerResponse* response =
					request->beginResponse(SPIFFS, filePath, mimeType);
				response->addHeader("Cache-Control", cacheControl); // Add Cache-Control header
				request->send(response);
			});
	}
};

} // namespace Node_Utility

#endif // FILE_HANDLER_H
