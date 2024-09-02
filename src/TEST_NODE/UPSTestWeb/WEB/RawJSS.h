#ifndef RAW_JSS_H
#define RAW_JSS_H

#include <pgmspace.h>
const char MAIN_SCRIPT_JSS[] PROGMEM = R"rawliteral(
    <script>
      document.addEventListener('DOMContentLoaded', function () {
        const sidebar = document.getElementById('sidebar');
        const content = document.getElementById('content');
        const toggleButton = document.getElementById('toggleSidebar');

        if (toggleButton && sidebar && content) {
          toggleButton.addEventListener('click', () => {
            sidebar.classList.toggle('hidden');
            content.classList.toggle('full-width');
          });
        } else {
          console.error('One or more elements not found:', {
            toggleButton,
            sidebar,
            content,
          });
        }

        window.tests = []; 

        window.addTest = function () {
          var testSelect = document.getElementById('addTest');
          var loadLevelSelect = document.getElementById('loadLevel');
          var selectedTest = testSelect.options[testSelect.selectedIndex].text;
          var loadLevel =
            loadLevelSelect.options[loadLevelSelect.selectedIndex].value;
          var testDetails = {
            testName: selectedTest,
            loadLevel: loadLevel + '%',
          };
          window.tests.push(testDetails);
          appendLog(
            'All tests: ' +
              JSON.stringify(window.tests) +
              ' at ' +
              new Date().toLocaleString()
          );
        };

        window.deleteTest = function () {
          if (window.tests.length > 0) {
            var deletedTest = window.tests.pop();
            appendLog(
              'Test deleted: ' +
                JSON.stringify(deletedTest) +
                ' at ' +
                new Date().toLocaleString()
            );
            appendLog('Remaining tests: ' + JSON.stringify(window.tests));
            sendTestData(); 
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
          sendTestData(); 
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
          var testCommand = document.getElementById('testCommand');
          if (testCommand) {
            testCommand.innerHTML += message + '\n';
            testCommand.scrollTop = testCommand.scrollHeight;
          } else {
            console.error('Element testCommand not found');
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
        function sendModeJson(mode) {
          fetch('/updateMode', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode: mode }), 
          })
            .then((response) => response.text())
            .then((data) => console.log('Server Response:', data))
            .catch((error) => console.error('Error:', error));
        }

        function sendCommand(command) {
          fetch('/updateCommand', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command: command }), 
          })
            .then((response) => response.text())
            .then((data) => console.log('Server Response:', data))
            .catch((error) => console.error('Error:', error));
        }
      });
    </script>
)rawliteral";

#endif // RAW_JSS_H
