#ifndef RAWHTML_H
#define RAWHTML_H

#include <pgmspace.h>
const char HEADER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
   )rawliteral";
//-------------here CSS PART WILL GO----------------//
const char HEADER_TRAILER_HTML[] PROGMEM = R"rawliteral(
 </head>
  <body>
)rawliteral";

// Navbar HTML
const char NAVBAR_HTML[] PROGMEM = R"rawliteral(
<header style="display: flex; align-items: center; padding: 10px; background-color: #333; color: white;">
    <img src="/Logo-Full.svg" alt="Logo" style="height: 50px; margin-right: 20px;">
    <h1 style="margin: 0;">"%s"</h1>
</header>
<div class="navbar">
    <div class="navbar-title">WebInterface</div>
    <div>
        <a href="%s">Dashboard</a>
        <a href="%s">Settings</a>
        <a href="%s">Network</a>
        <a href="%s">Modbus</a>
        <a href="%s">Test Report</a>
        <button id="toggleSidebar" class="button">Toggle Sidebar</button>
        <button class="button">Shutdown</button>
        <button class="button">Restart</button>
    </div>
</div>
)rawliteral";

// Sidebar HTML
const char SIDEBAR_HTML[] PROGMEM = R"rawliteral(
<div class="sidebar" id="sidebar">
    <p>Sidebar content here</p>
    <button onclick="startTest()">Start</button>
    <button onclick="stopTest()">Stop</button>
    <button onclick="pauseTest()">Pause</button>
    <div class="dropdown">
        <label for="addTest">Add Test:</label>
        <select id="addTest">
            <option value="1">Switch Test</option>
            <option value="2">Backup Test</option>
            <option value="3">Efficiency Test</option>
            <option value="4">Input Voltage Test</option>
            <option value="5">Waveform Test</option>
            <option value="6">Tune PWM Test</option>
        </select>
    </div>
    <div class="dropdown">
        <label for="loadLevel">Load Level:</label>
        <select id="loadLevel">
            <option value="0">0%</option>
            <option value="25">25%</option>
            <option value="50">50%</option>
            <option value="75">75%</option>
            <option value="100">100%</option>
        </select>
    </div>
    <button onclick="addTest()">Add Test</button>
    <button onclick="deleteTest()">Delete Test</button>
    <div class="toggler">
        <label for="mode">Mode:</label>
        <input type="radio" id="auto" name="mode" value="AUTO">
        <label for="auto">Auto</label>
        <input type="radio" id="manual" name="mode" value="MANUAL">
        <label for="manual">Manual</label>
    </div>
</div>
)rawliteral";
const char USER_COMMAND_AND_LOG_HTML[] PROGMEM = R"rawliteral(
<div class="container">
    <!-- Left side: User commands -->
    <div class="user-command">
        <h2>User Commands</h2>
        <pre id="testCommand"></pre>
    </div>

    <!-- Right side: Log output -->
    <div class="log-output">
        <h2>Log Output</h2>
        <pre id="logs"></pre>
    </div>
</div>
)rawliteral";

// Main Content and Footer HTML
const char USER_COMMAND_HTML[] PROGMEM = R"rawliteral(
<pre id="testCommand"></pre>
</body>
</html>
)rawliteral";
const char LAST_TRAILER_HTML[] PROGMEM = R"rawliteral(
   
  </body>
</html>
)rawliteral";

#endif