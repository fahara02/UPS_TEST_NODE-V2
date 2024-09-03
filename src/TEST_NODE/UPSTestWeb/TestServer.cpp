#include "TestServer.h"
#include "AsyncJson.h"

// Other includes as necessary

#define ETAG "\"" __DATE__ " " __TIME__ "\""

void TestServer::setupPages(UPSTesterSetup& _setup, TestSync& _sync)
{
	// Handle GET requests
	std::vector<String> getUris = {"/", "/dashboard", "/settings/ups-specification",
								   "/settings/test-specification", "/log"};
	for(const auto& uri: getUris)
	{
		_server->on(uri.c_str(), HTTP_GET, [this, uri](AsyncWebServerRequest* request) {
			this->handleGETRequest(uri, request);
		});
	}

	// Handle POST requests
	std::vector<String> postUris = {"/updateMode", "/updateCommand"};
	for(const auto& uri: postUris)
	{
		_server->on(uri.c_str(), HTTP_POST,
					[this, uri, &_setup, &_sync](AsyncWebServerRequest* request) {
						this->handlePOSTRequest(uri, request, _setup, _sync);
					});
	}

	auto* testDataHandler = new AsyncCallbackJsonWebHandler(
		"/updateTestData", [this, &_sync](AsyncWebServerRequest* request, JsonVariant& json) {
			this->handleTestDataRequest(request, json, _sync);
		});
	_server->addHandler(testDataHandler);

	// Handle 404 errors
	_server->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "text/plain", "404 Not Found");
	});
}
