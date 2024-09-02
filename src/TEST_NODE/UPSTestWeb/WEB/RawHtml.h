#ifndef RAWHTML_H
#define RAWHTML_H

#include <pgmspace.h>
const char TOP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>UPS Automatic Testing Interface</title>
)rawliteral";

// CSS PART WILL GO HERE
const char HEADER_TRAILER_HTML[] PROGMEM = R"rawliteral(
 </head>
  <body>
)rawliteral";
const char HEADER_HTML[] PROGMEM = R"rawliteral(
 <header>
      <div class="logo-container">
        <img src="/Logo-Full.svg" alt="Logo" />
      </div>
      <h1>UPS Automatic Testing</h1>
      <div style="width: 70px"></div>
      <!-- Placeholder for alignment -->
    </header>
)rawliteral";

// Navbar HTML
const char NAVBAR_HTML[] PROGMEM = R"rawliteral(
 <div class="navbar">
      <div class="navbar-title">Web Interface</div>
      <div class="buttons">
        <a href="/dashboard">Dashboard</a>

        <div class="dropdown">
          <button class="dropbtn">Settings</button>
          <div class="dropdown-content">
            <a href="#ups-specification">UPS Specification</a>
            <a href="#test-specification">Test Specification</a>
            <a href="#report-specification">Report Specification</a>
            <hr />
            <button id="update-settings-button">Update</button>
          </div>
        </div>

        <a href="/network">Network</a>
        <a href="/modbus">Modbus</a>
        <a href="/test-report">Test Report</a>
        <button id="toggleSidebar" class="button">Toggle Sidebar</button>
        <button class="button">Shutdown</button>
        <button class="button">Restart</button>
      </div>
    </div>
)rawliteral";

// Sidebar HTML
const char SIDEBAR_HTML[] PROGMEM = R"rawliteral(
        <div class="sidebar" id="sidebar">
      <h2>Test Commands</h2>
      <button onclick="startTest()">Start</button>
      <button onclick="stopTest()">Stop</button>
      <button onclick="pauseTest()">Pause</button>

      <div class="dropdown">
        <label for="addTest">UPS Tests:</label>
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
      <button onclick="sendTest()">Send Test</button>
      <button onclick="clearTest()">Clear Test</button>

      <div class="toggler">
        <button class="mode-button" onclick="sendMode()">Mode</button>
        <div class="radio-group">
          <input type="radio" id="auto" name="mode" value="AUTO" checked />
          <label for="auto">Auto</label>
          <input type="radio" id="manual" name="mode" value="MANUAL" />
          <label for="manual">Manual</label>
        </div>
      </div>
    </div>
)rawliteral";

// User Command and Log HTML
const char USER_COMMAND_AND_LOG_HTML[] PROGMEM = R"rawliteral(
 
    <div class="container">
      <!-- User Commands -->
      <div class="user-command">
        <h2>User Commands</h2>
        <pre id="testCommand"></pre>
      </div>

    
      <div class="log-output">
        <h2>Log Output</h2>
        <pre id="logs"></pre>
      </div>
    </div>
)rawliteral";
// JSS PART WILL GO HERE

const char LAST_TRAILER_HTML[] PROGMEM = R"rawliteral(
   
  </body>
</html>
)rawliteral";

#endif // RAWHTML_H
