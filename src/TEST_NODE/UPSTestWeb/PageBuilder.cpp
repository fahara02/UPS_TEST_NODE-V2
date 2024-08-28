#include "PageBuilder.h"

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
	response->print("</div></body></html>");
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
