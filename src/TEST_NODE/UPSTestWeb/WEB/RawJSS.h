#ifndef RAW_JSS_H
#define RAW_JSS_H
#include <pgmspace.h>
const char MAIN_SCRIPT_JSS[] PROGMEM = R"rawliteral(
      <script>
      const sidebar = document.getElementById('sidebar');
      const content = document.getElementById('content');
      const toggleButton = document.getElementById('toggleSidebar');

      toggleButton.addEventListener('click', () => {
        sidebar.classList.toggle('hidden');
        content.classList.toggle('full-width');
      });

      var tests = []; // Store all added tests

      function addTest() {
        var testSelect = document.getElementById('addTest');
        var loadLevelSelect = document.getElementById('loadLevel');
        var selectedTest = testSelect.options[testSelect.selectedIndex].text;
        var loadLevel = loadLevelSelect.options[loadLevelSelect.selectedIndex].value;
        var testDetails = {
          testName: selectedTest,
          loadLevel: loadLevel + '%',
        };
        tests.push(testDetails);
        appendLog('All tests: ' + JSON.stringify(tests) + ' at ' + new Date().toLocaleString());
      }

      function deleteTest() {
        if (tests.length > 0) {
          var deletedTest = tests.pop();
          appendLog('Test deleted: ' + JSON.stringify(deletedTest) + ' at ' + new Date().toLocaleString());
          appendLog('Remaining tests: ' + JSON.stringify(tests));
        } else {
          appendLog('No tests to delete.');
        }
      }

      function appendLog(message) {
        var logMonitor = document.getElementById('logMonitor');
        logMonitor.innerHTML += message + '\n';
        logMonitor.scrollTop = logMonitor.scrollHeight;
      }

      function startTest() {
        alert('Test Started');
        appendLog('Test started at ' + new Date().toLocaleString());
      }
      function stopTest() {
        alert('Test Stopped');
        appendLog('Test stopped at ' + new Date().toLocaleString());
      }
      function pauseTest() {
        alert('Test Paused');
        appendLog('Test paused at ' + new Date().toLocaleString());
      }
    </script>
)rawliteral";
#endif