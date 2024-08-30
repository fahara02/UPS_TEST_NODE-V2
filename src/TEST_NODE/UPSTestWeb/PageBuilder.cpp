#include "PageBuilder.h"

#define ETAG "\"" __DATE__ " " __TIME__ "\""

void PageBuilder::setupPages(WiFiManager* wm, UPSTesterSetup* testSetup, TestManager* testManager,
							 TestSync* testSync, SetupUPSTest* allSetup, TestData* data)
{
	_server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
		auto* response = request->beginResponseStream("text/html");
		this->sendHeader(response, "UPS TESTING PANEL");
		response->print("<style>");
		this->sendStyle(response);
		response->print("</style>");
		this->sendHeaderTrailer(response);
		const char* routes[] = {"DashBoard", "Settings", "Network", "Modbus", "Report"};

		this->sendNavbar(response, routes, "WebInterface");
		this->sendSidebar(response);
		this->sendDashboard(response, logger);
		this->sendScript(response);
		this->sendPageTrailer(response);
	});

	_server->on("/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
		request->redirect("/");
	});

	_server->on("/favicon.ico", [](AsyncWebServerRequest* request) {
		request->send(204); // TODO add favicon
	});
	_server->on("/style.css", [this](AsyncWebServerRequest* request) {
		if(request->hasHeader("If-None-Match"))
		{
			auto header = request->getHeader("If-None-Match");
			if(header->value() == String(ETAG))
			{
				request->send(304);
				return;
			}
		}
		auto* response = request->beginResponseStream("text/css");
		this->sendStyle(response);
		response->addHeader("ETag", ETAG);
		request->send(response);
	});
	_server->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "text/plain", "404");
	});
}

const char* PageBuilder::copyFromPROGMEM(const char copyFrom[], char sendTo[])
{
	// Copy the string from PROGMEM to SRAM
	strcpy_P(sendTo, copyFrom);
	// Return the destination buffer
	return sendTo;
}

void PageBuilder::sendHeader(AsyncResponseStream* response, const char* title)
{
	char buffer[HEADER_HTML_LENGTH];
	const char* headerHtml = copyFromPROGMEM(HEADER_HTML, buffer);
	response->printf(headerHtml, title);
}

void PageBuilder::sendStyle(AsyncResponseStream* response)
{
	char buffer[CSS_LENGTH];
	const char* CSS = copyFromPROGMEM(STYLE_BLOCK_CSS, buffer);
	response->print(CSS);
}

void PageBuilder::sendHeaderTrailer(AsyncResponseStream* response)
{
	char buffer[HEADER_TRAILER_HTML_LENGTH];
	const char* headerTrailerHtml = copyFromPROGMEM(HEADER_TRAILER_HTML, buffer);
	response->print(headerTrailerHtml);
}
void PageBuilder::sendNavbar(AsyncResponseStream* response, const char* routes[], const char* title,
							 const char* btn1class, const char* btn2class, const char* btn3class)
{
	char buffer[NAVBAR_HTML_LENGTH];
	const char* navbarHtml = copyFromPROGMEM(NAVBAR_HTML, buffer);

	response->printf(navbarHtml,
					 routes[0], // Dashboard route
					 routes[1], // Settings route
					 routes[2], // Network route
					 routes[3], // Modbus route
					 routes[4], // Test Report route
					 title,
					 btn1class, // Action for Shutdown button
					 btn2class, // CSS class for Shutdown button
					 btn3class // Action for Restart button
	);
}

void PageBuilder::sendSidebar(AsyncResponseStream* response, const char* content)
{
	char buffer[SIDEBAR_HTML_LENGTH];
	const char* sidebarHtml = copyFromPROGMEM(SIDEBAR_HTML, buffer);
	response->printf(sidebarHtml, content);
}
void PageBuilder::sendDashboard(AsyncResponseStream* response, Logger& logger,
								const char* classname, const char* paragraph)
{
	response->printf("<div class=\"%s\">", classname);
	response->print("<h1>Dashboard</h1>");
	response->printf("<p>%s</p>", paragraph);

	this->sendLogmonitor(response, logger);

	response->print("</div>");
}
void PageBuilder::sendScript(AsyncResponseStream* response)
{
	char buffer[JSS_LENGTH];
	const char* JSS = copyFromPROGMEM(MAIN_SCRIPT_JSS, buffer);
	response->print(JSS);
}

void PageBuilder::sendPageTrailer(AsyncResponseStream* response)
{
	char buffer[LAST_TRAILER_HTML_LENGTH];
	const char* pageTrailerHtml = copyFromPROGMEM(LAST_TRAILER_HTML, buffer);
	response->print(pageTrailerHtml);
}

void PageBuilder::sendMargin(AsyncResponseStream* response, int pixel, MarginType marginType)
{
	// Open the <div> tag
	response->print("<div");

	// Determine the CSS property based on the MarginType enum
	const char* cssProperty = "margin-top"; // Default to margin-top

	switch(marginType)
	{
		case MarginType::Top:
			cssProperty = "margin-top";
			break;
		case MarginType::Bottom:
			cssProperty = "margin-bottom";
			break;
		case MarginType::Left:
			cssProperty = "margin-left";
			break;
		case MarginType::Right:
			cssProperty = "margin-right";
			break;
	}

	// Add inline style for the specified margin type dynamically based on the input pixel
	response->printf(" style=\"%s: %dpx;\"", cssProperty, pixel);

	// Close the <div> tag
	response->print("></div>");
}

void PageBuilder::sendButton(AsyncResponseStream* response, const char* title, const char* action,
							 const char* css)
{
	response->printf("<form method=\"get\" action=\"%s\">"
					 "<button class=\"%s\">%s</button>"
					 "</form>"
					 "<p></p>",
					 action, css, title);
}

void PageBuilder::sendToggleButton(AsyncResponseStream* response, const char* name, bool state)
{
	response->print("<div>");
	response->printf("<label>%s</label>", name);
	response->printf("<input type='checkbox' %s>", state ? "checked" : "");
	response->print("</div>");
}

void PageBuilder::sendDropdown(AsyncResponseStream* response, const char* name,
							   const std::vector<const char*>& options, const char* selected)
{
	response->print("<div>");
	response->printf("<label>%s</label>", name);
	response->print("<select>");

	for(const auto& option: options)
	{
		response->printf("<option value='%s' %s>%s</option>", option,
						 (selected && strcmp(option, selected) == 0) ? "selected" : "", option);
	}

	response->print("</select>");
	response->print("</div>");
}
void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, String value)
{
	response->printf("<tr>"
					 "<td>%s:</td>"
					 "<td>%s</td>"
					 "</tr>",
					 name, value.c_str());
}

void PageBuilder::sendTableRow(AsyncResponseStream* response, const char* name, uint32_t value)
{
	response->printf("<tr>"
					 "<td>%s:</td>"
					 "<td>%d</td>"
					 "</tr>",
					 name, value);
}

void PageBuilder::sendLogmonitor(AsyncResponseStream* response, Logger& logger)
{
	response->print("<div class=\"log-monitor\" id=\"logMonitor\">");

	// Optionally, you can inject initial log data here
	response->print("<!-- UPS TESTING PANEL LOGS-->");

	// Example: Sending some initial log messages
	response->printf("<p>%s</p>", logger.getBufferedLogs().c_str());

	response->print("</div>");
}
