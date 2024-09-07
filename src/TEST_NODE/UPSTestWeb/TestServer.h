#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "PageBuilder.h"
#include "ArduinoJson.h"
#include <Update.h>
#include <vector>

#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "NodeConstants.h"
#include "EventHelper.h"
#include "Settings.h"
#include "TestData.h"
#include "UPSdebug.h"
#include "TestManager.h"
#include "UPSTesterSetup.h"
#include "Filehandler.h"
#include <deque>
#include "DataHandler.h"

class TestServer
{
  public:
	TestServer(AsyncWebServer* server, AsyncWebSocket* ws, UPSTesterSetup& _setup,
			   TestSync& _sync) :
		_server(server),
		_ws(ws), webPage(new PageBuilder()), _setup(UPSTesterSetup::getInstance()),
		_sync(TestSync::getInstance())
	{
		initWebSocket();
		createServerTask();
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
		_fileHandler.serveFile(*_server, "/Logo-Full.svg", "/Logo-Full.svg", "image/svg+xml",
							   "max-age=2592000"); // Cache for 30 days
		_fileHandler.serveFile(*_server, "/favicon.png", "/favicon.png", "image/png",
							   "max-age=2592000"); // Cache for 30 days
		_fileHandler.serveFile(*_server, "/style.css", "/style.css", "text/css",
							   "max-age=86400"); // Cache for 24 hours
		_fileHandler.serveFile(*_server, "/script.js", "/script.js", "application/javascript",
							   "max-age=86400"); // Cache for 24 hours

		servePages(_setup, _sync);
	}

	void initWebSocket();

	void servePages(UPSTesterSetup& _setup, TestSync& _sync);

  private:
	AsyncWebServer* _server;
	AsyncWebSocket* _ws;

	PageBuilder* webPage;
	Node_Utility::FileHandler _fileHandler;
	UPSTesterSetup& _setup;
	TestSync& _sync;
	bool isClientConnected = false;
	int _lastClientId = 0;
	// std::deque<AsyncWebSocketMessageBuffer*> sensorDataQueue;
	bool _sendData = false;

	// GUI buttons/LED state
	bool _startBTN = false;
	bool _stopBTN = false;
	bool _pauseBTN = false;
	bool _modeBTN = false;
	bool _loadBTN = false;
	bool _mainBTN = false;
	bool _blinkSetupLED = false;
	bool _blinkReadyLED = false;
	bool _blinkTestLED = false;

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
	// WebSocket-related functions
	void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
				   void* arg, uint8_t* data, size_t len);

	void createServerTask();
	static void wsClientCleanup(void* pvParameters);
};

#endif
