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
    <link rel="icon" href="/favicon.png" type="image/png" />
    <link rel="stylesheet" href="/style.css?v=2.0.2">
)rawliteral";

// CSS PART WILL GO HERE
const char HEAD_TRAILER_HTML[] PROGMEM = R"rawliteral(
 </head>
  <body>
  <script src="/script.js?v=2.0.3"></script>
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
            <a href="/settings/ups-specification">UPS Specification</a>
            <a href="/settings/test-specification">Test Specification</a>
            <a href="/settings/report-specification">Report Specification</a>
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

// User Command and Log HTML Will be here from function
// Power monitor right after it
const char POWER_MONITOR_HTML[] PROGMEM = R"rawliteral(
    
       <div class="power-monitor">
        <h2>Power Monitor</h2>

        <!-- Power Factor Displays -->
        <div class="card-container">
          <div class="card">
            <div class="card-title">Input Power Factor</div>
            <div class="card-value" id="inputPowerFactor">0.0</div>
          </div>
          <div class="card">
            <div class="card-title">Output Power Factor</div>
            <div class="card-value" id="outputPowerFactor">0.0</div>
          </div>
        </div>

        <!-- Voltage Displays -->
        <div class="card-container">
          <div class="card">
            <div class="card-title">Input Voltage</div>
            <div class="card-value" id="inputVoltage">0.0 V</div>
          </div>
          <div class="card">
            <div class="card-title">Output Voltage</div>
            <div class="card-value" id="outputVoltage">0.0 V</div>
          </div>
        </div>

        <!-- Current Displays -->
        <div class="card-container">
          <div class="card">
            <div class="card-title">Input Current</div>
            <div class="card-value" id="inputCurrent">0.0 A</div>
          </div>
          <div class="card">
            <div class="card-title">Output Current</div>
            <div class="card-value" id="outputCurrent">0.0 A</div>
          </div>
        </div>

        <!-- Wattage Displays -->
        <div class="card-container">
          <div class="card">
            <div class="card-title">Input Wattage</div>
            <div class="card-value" id="inputWattage">0.0 W</div>
          </div>
          <div class="card">
            <div class="card-title">Output Wattage</div>
            <div class="card-value" id="outputWattage">0.0 W</div>
          </div>
        </div>
        <div class="bottom-bar">
          <div class="switches">
            <!-- Load On/Off Switch -->
            <div class="toggle-container">
              <span id="loadStateLabel">Load Off</span>
              <label class="toggle-switch">
                <input type="checkbox" id="toggleLoadSwitch" />
                <span class="slider"></span>
              </label>
            </div>

            <!-- Simulate Power Cut/Restore Power Switch -->
            <div class="toggle-container">
              <span id="powerCutLabel">Mains Off </span>
              <label class="toggle-switch">
                <input type="checkbox" id="togglePowerCutSwitch" />
                <span class="slider"></span>
              </label>
            </div>
            <!-- LED Indicators -->
            <div class="led-indicators">
              <div class="led blue" id="ledLoadOn" title="Load On"></div>
              <div class="led green" id="ledReady" title="Ready"></div>
              <div
                class="led red"
                id="ledTestRunning"
                title="Test Running"
              ></div>
            </div>
          </div>
        </div>
      </div>
    </div>
)rawliteral";
// JSS PART WILL GO HERE

const char LAST_TRAILER_HTML[] PROGMEM = R"rawliteral(
   
  </body>
</html>
)rawliteral";

#endif // RAWHTML_H
