#ifndef PAGE_H
#define PAGE_H

const char* page = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>UPS Testing Panel</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    background-color: #f0f0f0;
                    margin: 0;
                }
                .navbar {
                    background-color: black;
                    padding: 1em;
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                    color: white;
                    position: relative;
                }
                .navbar-title {
                    font-size: 1.5em;
                    margin-left: 20px;
                }
                .navbar a {
                    color: white;
                    text-decoration: none;
                    margin: 0 15px;
                }
                .sidebar {
                    height: 100vh;
                    width: 200px;
                    background-color: #333;
                    color: white;
                    padding: 15px;
                    position: fixed;
                    top: 0;
                    left: 0;
                }
                .sidebar button {
                    display: block;
                    width: 100%;
                    margin: 10px 0;
                    padding: 10px;
                    background-color: blue;
                    border: none;
                    color: white;
                    cursor: pointer;
                }
                .sidebar button:hover {
                    background-color: red;
                }
                .content {
                    margin-left: 220px;
                    padding: 20px;
                }
                .dropdown {
                    background-color: white;
                    color: black;
                    padding: 10px;
                    width: 100%;
                }
                .toggler {
                    margin-top: 20px;
                    padding: 10px;
                }
                .button {
                    background-color: red;
                    color: white;
                    padding: 10px 20px;
                    border: none;
                    cursor: pointer;
                }
                .button:hover {
                    background-color: blue;
                }
                .log-monitor {
                    margin-top: 20px;
                    width: 100%;
                    height: 300px;
                    background-color: #fff;
                    border: 1px solid #ccc;
                    padding: 10px;
                    overflow-y: scroll;
                    font-family: monospace;
                    white-space: pre-wrap;
                    background-color: #f5f5f5;
                }
            </style>
        </head>
        <body>

            <div class="navbar">
                <div class="navbar-title">UPS Testing Panel</div>
                <div>
                    <a href="#">Dashboard</a>
                    <a href="#">Settings</a>
                    <a href="#">Network</a>
                    <a href="#">Modbus</a>
                    <a href="#">Test Report</a>
                    <button class="button">Shutdown</button>
                    <button class="button">Restart</button>
                </div>
            </div>

            <div class="sidebar">
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
                    <label for="loadLevel">Add Load Level:</label>
                    <select id="loadLevel">
                        <option value="0">0%</option>
                        <option value="25">25%</option>
                        <option value="50">50%</option>
                        <option value="75">75%</option>
                        <option value="100">100%</option>
                    </select>
                </div>
                
                <div class="toggler">
                    <label for="mode">Mode:</label>
                    <input type="radio" id="auto" name="mode" value="AUTO">
                    <label for="auto">Auto</label>
                    <input type="radio" id="manual" name="mode" value="MANUAL">
                    <label for="manual">Manual</label>
                </div>
            </div>

            <div class="content">
                <h1>Dashboard</h1>
                <p>This is where test reports and status will be shown.</p>

                <div class="log-monitor" id="logMonitor">
                    <!-- Log data will be appended here -->
                </div>
            </div>

            <script>
                function startTest() {
                    alert("Test Started");
                    appendLog("Test started at " + new Date().toLocaleString());
                }
                function stopTest() {
                    alert("Test Stopped");
                    appendLog("Test stopped at " + new Date().toLocaleString());
                }
                function pauseTest() {
                    alert("Test Paused");
                    appendLog("Test paused at " + new Date().toLocaleString());
                }
                function appendLog(message) {
                    var logMonitor = document.getElementById('logMonitor');
                    logMonitor.innerHTML += message + "\n";
                    logMonitor.scrollTop = logMonitor.scrollHeight; // Auto-scroll to the bottom
                }
            </script>

        </body>
        </html>
        )rawliteral";

#endif // PAGE_H
