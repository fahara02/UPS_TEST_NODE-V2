#include "PageBuilder.h"
const char* PageBuilder::copyFromPROGMEM(const char copyFrom[], char sendTo[])
{
	// Copy the string from PROGMEM to SRAM
	strcpy_P(sendTo, copyFrom);
	// Return the destination buffer
	return sendTo;
}

void PageBuilder::sendResponseHeader(AsyncResponseStream* response, const char* title,
									 bool inlineStyle)
{
	response->print("<!DOCTYPE html>"
					"<html lang=\"en\" class=\"\">"
					"<head>"
					"<meta charset='utf-8'>"
					"<meta name=\"viewport\" "
					"content=\"width=device-width,initial-scale=1,user-scalable=no\"/>");
	response->printf("<title>UPS Testing Panel - %s</title>", title);
	if(inlineStyle)
	{
		response->print("<style>");
		sendMinCss(response);
		response->print("</style>");
	}
	else
	{
		response->print("<link rel=\"stylesheet\" href=\"style.css\">");
	}
	response->print("</head>"
					"<body>"
					"<h2>UPS Testing Panel</h2>");
	response->printf("<h3>%s</h3>", title);
	response->print("<div id=\"content\">");
}
void PageBuilder::sendResponseTrailer(AsyncResponseStream* response)
{
	response->print("</body></html>");
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
void PageBuilder::sendDashboard(AsyncResponseStream* response, Logger* logger,
								const char* classname, const char* paragraph)
{
	response->printf("<div class=\"%s\">", classname);
	response->print("<h1>Dashboard</h1>");
	response->printf("<p>%s</p>", paragraph);

	this->sendLogmonitor(response, logger);

	response->print("</div>");
}

void PageBuilder::sendLogmonitor(AsyncResponseStream* response, Logger* logger)
{
	response->print("<div class=\"log-monitor\" id=\"logMonitor\">");

	// Optionally, you can inject initial log data here
	response->print("<!-- UPS TESTING PANEL LOGS-->");

	// Example: Sending some initial log messages
	response->printf("<p>%s</p>", logger->getBufferedLogs().c_str());

	response->print("</div>");
}
void PageBuilder::sendNavbar(AsyncResponseStream* response, const char* title, const char* action,
							 const char* css, const char* routes[])
{
	// Assuming routes array contains:
	// [0] - Dashboard route
	// [1] - Settings route
	// [2] - Network route
	// [3] - Modbus route
	// [4] - Test Report route
	char buffer[NAVBAR_HTML_LENGTH];
	const char* navbarHtml = copyFromPROGMEM(NAVBAR_HTML, buffer);

	response->printf(NAVBAR_HTML, title,
					 routes[0], // Dashboard route
					 routes[1], // Settings route
					 routes[2], // Network route
					 routes[3], // Modbus route
					 routes[4], // Test Report route
					 action, // Action for Shutdown button
					 css, // CSS class for Shutdown button
					 action, // Action for Restart button
					 css // CSS class for Restart button
	);
}
void PageBuilder::sendSidebar(AsyncResponseStream* response)
{
	// Begin the sidebar HTML
	char buffer[SIDEBAR_HTML_LENGTH];
	const char* sidebarHtml = copyFromPROGMEM(SIDEBAR_HTML, buffer);
	response->printf(sidebarHtml);

	// Start, Stop, Pause buttons using sendButton
	this->sendButton(response, "Start", "startTest()", "button");
	this->sendButton(response, "Stop", "stopTest()", "button");
	this->sendButton(response, "Pause", "pauseTest()", "button");

	// Add Test dropdown using sendDropdown
	std::vector<const char*> addTestOptions = {"Switch Test",	  "Backup Test",
											   "Efficiency Test", "Input Voltage Test",
											   "Waveform Test",	  "Tune PWM Test"};
	this->sendDropdown(response, "Add Test:", addTestOptions, "1");

	// Load Level dropdown using sendDropdown
	std::vector<const char*> loadLevelOptions = {"0%", "25%", "50%", "75%", "100%"};
	this->sendDropdown(response, "Load Level:", loadLevelOptions, "0");
	this->sendMargin(response, 10, MarginType::Top);
	// Add Test and Delete Test buttons using sendButton
	this->sendButton(response, "Add Test", "addTest()", "button");
	this->sendButton(response, "Delete Test", "deleteTest()", "button");
	this->sendMargin(response, 10, MarginType::Top);
	// Mode toggler using custom HTML
	response->print("<div class=\"toggler\">");
	response->print("<label for=\"mode\">Mode:</label>");
	response->print("<input type=\"radio\" id=\"auto\" name=\"mode\" value=\"AUTO\" />");
	response->print("<label for=\"auto\">Auto</label>");
	response->print("<input type=\"radio\" id=\"manual\" name=\"mode\" value=\"MANUAL\" />");
	response->print("<label for=\"manual\">Manual</label>");
	response->print("</div>");

	// Close the sidebar HTML
	response->print("</div>");
}
