#ifndef PAGE_BUILDER_H
#define PAGE_BUILDER_H

#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include "Settings.h"
#include "TestData.h"
#include "UPSdebug.h"
#include "TestSync.h"
#include "TestManager.h"
#include "UPSTesterSetup.h"
#include "vector"

static const String WiFiQuality(int rssiValue)
{
	switch(rssiValue)
	{
		case -30 ... 0:
			return "Amazing";
		case -67 ... - 31:
			return "Very Good";
		case -70 ... - 68:
			return "Okay";
		case -80 ... - 71:
			return "Not Good";
		default:
			return "Unusable";
	}
}

class PageBuilder
{
  public:
	PageBuilder(AsyncWebServer* server) : _server(server)
	{
	}

	void setupPages(WiFiManager* wm, TestManager* testManager, TestSync* testSync,
					UPSTesterSetup* testSetup);

	void setupPages(WiFiManager* wm, Node_Core::SetupUPSTest* setup);

	void setupPages(WiFiManager* wm, TestData* data);

	void sendResponseHeader(AsyncResponseStream* response, const char* title,
							bool inlineStyle = false);

	void sendResponseTrailer(AsyncResponseStream* response);

	void sendButton(AsyncResponseStream* response, const char* title, const char* action,
					const char* css = "");

	void sendTableRow(AsyncResponseStream* response, const char* name, uint32_t value);

	void sendTableRow(AsyncResponseStream* response, const char* name, String value);

	void sendDebugForm(AsyncResponseStream* response, String slaveId, String reg, String function,
					   String count);

	void sendMinCss(AsyncResponseStream* response);

	void sendToggleButton(AsyncResponseStream* response, const char* name, bool state);

	void sendDropdown(AsyncResponseStream* response, const char* name,
					  const std::vector<const char*>& options, const char* selected = nullptr);

	// Utility function
	String getWiFiQuality(int rssiValue) const;

  private:
	AsyncWebServer* _server;
};

#endif // PAGE_BUILDER_H