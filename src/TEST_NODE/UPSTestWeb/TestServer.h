#ifndef TEST_SERVER_H
#define TEST_SERVER_H
#include "PageBuilder.h"
#include "ArduinoJson.h"
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

	void handleGETRequest(const String& uri, AsyncWebServerRequest* request,
						  UPSTesterSetup& _setup);
	void handlePOSTRequest(const String& uri, AsyncWebServerRequest* request,
						   UPSTesterSetup& _setup, TestSync& _sync);
	void handleTestDataRequest(AsyncWebServerRequest* request, JsonVariant& json, TestSync& _sync);
};
#endif