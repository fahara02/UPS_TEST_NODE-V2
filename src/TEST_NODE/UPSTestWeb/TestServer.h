#ifndef TEST_SERVER_H
#define TEST_SERVER_H
#include "PageBuilder.h"
#include "ArduinoJson.h"
class TestServer
{
  public:
	TestServer(AsyncWebServer* server) : _server(server), _builder()
	{
		UPSTesterSetup& _setup = UPSTesterSetup::getInstance();
		TestSync& _sync = TestSync::getInstance();
	}
	void setupPages(UPSTesterSetup& _setup, TestSync& _sync);

  private:
	AsyncWebServer* _server;
	PageBuilder _builder;

	void handleGETRequest(const String& uri, AsyncWebServerRequest* request);
	void handlePOSTRequest(const String& uri, AsyncWebServerRequest* request,
						   UPSTesterSetup& _setup, TestSync& _sync);
	void handleTestDataRequest(AsyncWebServerRequest* request, JsonVariant& json, TestSync& _sync);
};
#endif