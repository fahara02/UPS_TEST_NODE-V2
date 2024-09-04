#ifndef TEST_SERVER_H
#define TEST_SERVER_H
#include "PageBuilder.h"
#include "ArduinoJson.h"
#include <Update.h>
#include <vector>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "NodeConstants.h"
#include "Settings.h"
#include "TestData.h"
#include "UPSdebug.h"

#include "TestManager.h"
#include "UPSTesterSetup.h"
#include "Filehandler.h"
class TestServer
{
  public:
	TestServer(AsyncWebServer* server, UPSTesterSetup& _setup, TestSync& _sync) :
		_server(server), webPage(new PageBuilder()), _setup(UPSTesterSetup::getInstance()),
		_sync(TestSync::getInstance())
	{
	}
	~TestServer()
	{
		delete webPage; // Clean up dynamically allocated memory
	}
	void begin()

	{
		if(!_fileHandler.begin())
		{
			Serial.println("Failed to initialize the filesystem");
			return;
		}
		_fileHandler.serveFile(*_server, "/Logo-Full.svg", "/Logo-Full.svg", "image/svg+xml");
		servePages(_setup, _sync);
	}
	void servePages(UPSTesterSetup& _setup, TestSync& _sync);

  private:
	AsyncWebServer* _server;
	PageBuilder* webPage;
	Node_Utility::FileHandler _fileHandler;
	UPSTesterSetup& _setup;
	TestSync& _sync;

	// HTTP_GET
	void handleRootRequest(AsyncWebServerRequest* request);
	void handleLogRequest(AsyncWebServerRequest* request);
	void handleDashboardRequest(AsyncWebServerRequest* request);

	void handleSettingRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
							  const char* caption, SettingType type, const char* redirect_uri);
	// HTTP_POST
	void handleUpdateSettingRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
									SettingType type);
	void handleUpdateModeRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
								 TestSync& _sync);
	void handleUserUpdateRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
								 TestSync& _sync);
	void handleUserCommandRequest(AsyncWebServerRequest* request, UPSTesterSetup& _setup,
								  TestSync& _sync);

	void handleTestDataRequest(AsyncWebServerRequest* request, JsonVariant& json, TestSync& _sync);
};
#endif