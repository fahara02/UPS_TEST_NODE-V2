#ifndef RAWHTML_H
#define RAWHTML_H

#include <pgmspace.h> // Include for PROGMEM on Arduino

const char* const NAVBAR_HTML PROGMEM = R"rawliteral(
<div class="navbar">
  <div class="navbar-title">%s</div>
  <div>
    <a href="%s">Dashboard</a>
    <a href="%s">Settings</a>
    <a href="%s">Network</a>
    <a href="%s">Modbus</a>
    <a href="%s">Test Report</a>
    <form method="get" action="%s">
      <button class="%s">Shutdown</button>
    </form>
    <form method="get" action="%s">
      <button class="%s">Restart</button>
    </form>
  </div>
</div>
)rawliteral";

// Sidebar HTML
const char* const SIDEBAR_HTML_PROGMEM = R"rawliteral(
<div class="sidebar" id="sidebar">
  <div class="resizer" id="resizer"></div>
  <!-- Dynamic content will be inserted here -->
</div>
)rawliteral";

#endif // RAWHTML_H
