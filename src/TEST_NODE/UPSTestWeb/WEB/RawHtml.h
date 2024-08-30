#ifndef RAWHTML_H
#define RAWHTML_H

#include <pgmspace.h> // Include for PROGMEM on Arduino
const char header_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <header style="display: flex; align-items: center; padding: 10px; background-color: #333; color: white;">
    <img src="%s" alt="Logo" style="height: 50px; margin-right: 20px;">
    <h1 style="margin: 0;">%s</h1>
</header>
</head>
)rawliteral";
const char NAVBAR_HTML[] PROGMEM = R"rawliteral(
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
const char SIDEBAR_HTML[] PROGMEM = R"rawliteral(
<div class="sidebar" id="sidebar">
  <div class="resizer" id="resizer"></div>
  <!-- Dynamic content will be inserted here -->
</div>
)rawliteral";

const char TRAILER_HTML[] PROGMEM = R"rawliteral(
<div class="sidebar" id="sidebar">
  <div class="resizer" id="resizer"></div>
  <!-- Dynamic content will be inserted here -->
</div>
)rawliteral";

#endif // RAWHTML_H
