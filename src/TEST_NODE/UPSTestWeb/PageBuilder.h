#ifndef PAGE_BUILDER_H
#define PAGE_BUILDER_H

#include <pgmspace.h>
#include <Update.h>
#include <vector>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "NodeConstants.h"
#include "Settings.h"
#include "TestData.h"
#include "UPSdebug.h"
#include "TestSync.h"
#include "TestManager.h"
#include "UPSTesterSetup.h"

#include "Logger.h"
#include "RawHTML.h"
#include "RawCSS.h"
#include "RawJSS.h"

using namespace Node_Core;
extern Logger& logger;
constexpr size_t TOP_HTML_LENGTH = sizeof(TOP_HTML) - 1;
constexpr size_t HEADER_HTML_LENGTH = sizeof(HEADER_HTML) - 1;
constexpr size_t SIDEBAR_HTML_LENGTH = sizeof(SIDEBAR_HTML) - 1;
constexpr size_t NAVBAR_HTML_LENGTH = sizeof(NAVBAR_HTML) - 1;
constexpr size_t HEADER_TRAILER_HTML_LENGTH = sizeof(HEADER_TRAILER_HTML) - 1;
constexpr size_t LAST_TRAILER_HTML_LENGTH = sizeof(LAST_TRAILER_HTML) - 1;
constexpr size_t CONTENT_HTML_LENGTH = sizeof(USER_COMMAND_AND_LOG_HTML) - 1;
constexpr size_t CSS_LENGTH = sizeof(STYLE_BLOCK_CSS) - 1;
constexpr size_t JSS_LENGTH = sizeof(MAIN_SCRIPT_JSS) - 1;

enum class MarginType
{
	Top,
	Bottom,
	Left,
	Right
};

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
	void setupPages(TestSync& testSync);

	void sendHtmlHead(AsyncResponseStream* response);
	void sendHeader(AsyncResponseStream* response);
	void sendHeaderTrailer(AsyncResponseStream* response);
	void sendPageTrailer(AsyncResponseStream* response);
	void sendUserCommand(AsyncResponseStream* response, const char* content = "");
	void sendStyle(AsyncResponseStream* response);
	void sendScript(AsyncResponseStream* response);
	void sendNavbar(AsyncResponseStream* response);
	void sendSidebar(AsyncResponseStream* response, const char* content = "");
	void sendSettingTable(AsyncResponseStream* response, UPSTesterSetup& testerSetup);

	// Utility function
	String getWiFiQuality(int rssiValue) const;

  private:
	AsyncWebServer* _server;
	const char* copyFromPROGMEM(const char copyFrom[], char sendTo[]);

	void sendButton(AsyncResponseStream* response, const char* title, const char* action,
					const char* css = "");

	void sendDebugForm(AsyncResponseStream* response, String slaveId, String reg, String function,
					   String count);
	void sendMinCss(AsyncResponseStream* response);
	void sendToggleButton(AsyncResponseStream* response, const char* name, bool state);

	void sendMargin(AsyncResponseStream* response, int pixel, MarginType marginType);

	void sendInputField(AsyncResponseStream* response, const char* name, uint32_t value);
	void sendInputField(AsyncResponseStream* response, const char* name, uint32_t value,
						uint32_t minValue, uint32_t maxValue);
	void sendInputField(AsyncResponseStream* response, const char* name, const char* value);
	void sendInputField(AsyncResponseStream* response, const char* name, const char* value,
						const std::vector<const char*>& options);
	void sendDropdown(AsyncResponseStream* response, const char* name,
					  const std::vector<const char*>& options, const char* selected = nullptr);

	//--------------All Table related ------------------------//
	void sendTableCaption(AsyncResponseStream* response, const char* caption,
						  const char* style = "text-align:middle");
	void sendTableRow(AsyncResponseStream* response, const char* name, int value,
					  const char* cssClass = nullptr);
	void sendTableRow(AsyncResponseStream* response, const char* name, double value,
					  const char* cssClass = nullptr);
	void sendTableRow(AsyncResponseStream* response, const char* name, uint32_t value,
					  const char* cssClass = nullptr);
	void sendTableRow(AsyncResponseStream* response, const char* name, String value,
					  const char* cssClass = nullptr);
	void sendTableRow(AsyncResponseStream* response, const char* name, const char* value,
					  const char* cssClass = nullptr);

	template<typename... Args>
	void sendColorGroup(AsyncResponseStream* response, int span, const char* backgound_color,
						Args&&... args);
	void sendTableStyle(AsyncResponseStream* response);
};

#endif // PAGE_BUILDER_H
