#ifndef RAW_JSS_H
#define RAW_JSS_H
#include <pgmspace.h>
const char MAIN_SCRIPT_JSS[] PROGMEM = R"rawliteral(
       <script>
      const resizer = document.getElementById('resizer');
      const sidebar = document.getElementById('sidebar');
      const content = document.getElementById('content');
      var tests = []; // Store all added tests

      function addTest() {
        var testSelect = document.getElementById('addTest');
        var loadLevelSelect = document.getElementById('loadLevel');

        var selectedTest = testSelect.options[testSelect.selectedIndex].text;
        var loadLevel =
          loadLevelSelect.options[loadLevelSelect.selectedIndex].value;

        // Create a JSON object with the test name and load level
        var testDetails = {
          testName: selectedTest,
          loadLevel: loadLevel + '%',
        };

        // Add the new test to the tests array
        tests.push(testDetails);

        // Log the updated JSON with all tests
        appendLog(
          'All tests: ' +
            JSON.stringify(tests) +
            ' at ' +
            new Date().toLocaleString()
        );
      }

      function deleteTest() {
        if (tests.length > 0) {
          var deletedTest = tests.pop(); // Remove the last test added
          appendLog(
            'Test deleted: ' +
              JSON.stringify(deletedTest) +
              ' at ' +
              new Date().toLocaleString()
          );
          appendLog('Remaining tests: ' + JSON.stringify(tests));
        } else {
          appendLog('No tests to delete.');
        }
      }

      function appendLog(message) {
        var logMonitor = document.getElementById('logMonitor');
        logMonitor.innerHTML += message + '\n';
        logMonitor.scrollTop = logMonitor.scrollHeight; // Auto-scroll to the bottom
      }
      function appendLog(message) {
        var logMonitor = document.getElementById('logMonitor');
        logMonitor.innerHTML += message + '\n';
        logMonitor.scrollTop = logMonitor.scrollHeight; // Auto-scroll to the bottom
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
      function appendLog(message) {
        var logMonitor = document.getElementById('logMonitor');
        logMonitor.innerHTML += message + '\n';
        logMonitor.scrollTop = logMonitor.scrollHeight; // Auto-scroll to the bottom
      }

      resizer.addEventListener('mousedown', function (e) {
        document.addEventListener('mousemove', resizeSidebar);
        document.addEventListener('mouseup', stopResizing);
      });

      function resizeSidebar(e) {
        const newWidth = e.clientX;
        if (newWidth > 50 && newWidth < 400) {
          // Limit the width range
          sidebar.style.width = newWidth + 'px';
          resizer.style.left = newWidth + 'px';
          content.style.marginLeft = newWidth + 10 + 'px';
        }
      }

      function stopResizing() {
        document.removeEventListener('mousemove', resizeSidebar);
        document.removeEventListener('mouseup', stopResizing);
      }
    </script>
)rawliteral";
#endif