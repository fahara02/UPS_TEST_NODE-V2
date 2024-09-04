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
class TestServer
{
  public:
	TestServer(AsyncWebServer* server) : _server(server), webPage(new PageBuilder())
	{
	}
	~TestServer()
	{
		delete webPage; // Clean up dynamically allocated memory
	}

	void servePages(UPSTesterSetup& _setup, TestSync& _sync);

  private:
	AsyncWebServer* _server;
	PageBuilder* webPage;

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