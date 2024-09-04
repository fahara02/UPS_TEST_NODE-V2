#ifndef RAW_JSS_H
#define RAW_JSS_H

#include <pgmspace.h>
const char MAIN_SCRIPT_JSS[] PROGMEM = R"rawliteral(
    <script>
      document.addEventListener('DOMContentLoaded', function () {
        const sidebar = document.getElementById('sidebar');
        const toggleButton = document.getElementById('toggleSidebar');

        if (toggleButton && sidebar) {
          toggleButton.addEventListener('click', () => {
            sidebar.classList.toggle('hidden');
          });
        } else {
          console.error('Sidebar or Toggle Button not found.');
        }

        window.tests = []; // Store all added tests

        window.addTest = function () {
          const testSelect = document.getElementById('addTest');
          const loadLevelSelect = document.getElementById('loadLevel');
          const selectedTest =
            testSelect.options[testSelect.selectedIndex].text;
          const loadLevel = loadLevelSelect.value;
          const testDetails = {
            testName: selectedTest,
            loadLevel: loadLevel + '%',
          };
          window.tests.push(testDetails);
          appendLog(
            'Added Test: ' +
              JSON.stringify(testDetails) +
              ' at ' +
              new Date().toLocaleString()
          );
        };

        window.deleteTest = function () {
          if (window.tests.length > 0) {
            const deletedTest = window.tests.pop();
            appendLog(
              'Deleted Test: ' +
                JSON.stringify(deletedTest) +
                ' at ' +
                new Date().toLocaleString()
            );
            appendLog('Remaining Tests: ' + JSON.stringify(window.tests));
          } else {
            appendLog('No tests to delete.');
          }
        };

        window.sendTest = function () {
          var sendTest = window.tests;
          appendLog(
            'Sending Test: ' +
              JSON.stringify(sendTest) +
              ' at ' +
              new Date().toLocaleString()
          );
          sendTestData(); // Send the test data to the server whenever a test is added
        };

        window.clearTest = function () {
          var testCommand = document.getElementById('testCommand');
          if (testCommand) {
            testCommand.innerHTML = '';
          } else {
            console.error('Element testCommand not found');
          }
        };

        window.appendLog = function (message) {
          const testCommand = document.getElementById('testCommand');
          const logs = document.getElementById('logs');
          if (testCommand && logs) {
            testCommand.textContent += message + '\n';
            testCommand.scrollTop = testCommand.scrollHeight;
          } else {
            console.error('Log or Test Command element not found.');
          }
        };

        window.startTest = function () {
          alert('Test Started');
          appendLog('Test started at ' + new Date().toLocaleString());
          sendCommand('start');
        };

        window.stopTest = function () {
          alert('Test Stopped');
          appendLog('Test stopped at ' + new Date().toLocaleString());
          sendCommand('stop');
        };

        window.pauseTest = function () {
          alert('Test Paused');
          appendLog('Test paused at ' + new Date().toLocaleString());
          sendCommand('pause');
        };

        window.sendMode = function () {
          // Get the selected radio button
          var modeSelect = document.querySelector('input[name="mode"]:checked');
          var mode = modeSelect ? modeSelect.value : null;
          appendLog(
            'Sending Mode: ' +
              JSON.stringify(mode) +
              ' at ' +
              new Date().toLocaleString()
          );
          sendModeJson(mode);
        };

        function sendModeJson(mode) {
          fetch('/updateMode', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode: mode }), // Send mode as a JSON string
          })
            .then((response) => response.text())
            .then((data) => console.log('Server Response:', data))
            .catch((error) => console.error('Error:', error));
        }

        function sendTestData() {
          fetch('/updateTestData', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(window.tests),
          })
            .then((response) => response.text())
            .then((data) => console.log('Server Response:', data))
            .catch((error) => console.error('Error:', error));
        }

        function sendCommand(command) {
          fetch('/updateCommand', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command: command }), // Send status as a JSON string
          })
            .then((response) => response.text())
            .then((data) => console.log('Server Response:', data))
            .catch((error) => console.error('Error:', error));
        }
        document
          .getElementById('toggleLoadSwitch')
          .addEventListener('change', function () {
            var switchState = this.checked;
            document.getElementById('loadStateLabel').innerText = switchState
              ? 'Load On'
              : 'Load Off';
          });

        document
          .getElementById('togglePowerCutSwitch')
          .addEventListener('change', function () {
            var switchState = this.checked;
            document.getElementById('powerCutLabel').innerText = switchState
              ? 'Mains On'
              : 'Mains Off';
          });

        // Example functions for toggling switches
        function toggleLoad() {
          var switchState = document.getElementById('toggleLoadSwitch').checked;
          document.getElementById('loadStateLabel').innerText = switchState
            ? 'Load On'
            : 'Load Off';
        }

        function togglePowerCut() {
          var switchState = document.getElementById(
            'togglePowerCutSwitch'
          ).checked;
          document.getElementById('powerCutLabel').innerText = switchState
            ? 'Mains On'
            : 'Mains Off';
        }
      });
    </script>
)rawliteral";

#endif // RAW_JSS_H
