#ifndef RAW_JSS_H
#define RAW_JSS_H

#include <pgmspace.h>
const char MAIN_SCRIPT_JSS[] PROGMEM = R"rawliteral(
<script>
document.addEventListener('DOMContentLoaded', function() {
    const sidebar = document.getElementById('sidebar');
    const content = document.getElementById('content');
    const toggleButton = document.getElementById('toggleSidebar');
 

    if (toggleButton && sidebar && content) {
        toggleButton.addEventListener('click', () => {
            sidebar.classList.toggle('hidden');
            content.classList.toggle('full-width');
        });
    } else {
        console.error('One or more elements not found:', { toggleButton, sidebar, content });
    }

    window.tests = []; // Store all added tests in the global window object

    window.addTest = function() { // Define addTest globally
        var testSelect = document.getElementById('addTest');
        var loadLevelSelect = document.getElementById('loadLevel');
        var selectedTest = testSelect.options[testSelect.selectedIndex].text;
        var loadLevel = loadLevelSelect.options[loadLevelSelect.selectedIndex].value;
        var testDetails = {
            testName: selectedTest,
            loadLevel: loadLevel + '%',
        };
        window.tests.push(testDetails);
        appendLog('All tests: ' + JSON.stringify(window.tests) + ' at ' + new Date().toLocaleString());
    }

    window.deleteTest = function() { // Define deleteTest globally
        if (window.tests.length > 0) {
            var deletedTest = window.tests.pop();
            appendLog('Test deleted: ' + JSON.stringify(deletedTest) + ' at ' + new Date().toLocaleString());
            appendLog('Remaining tests: ' + JSON.stringify(window.tests));
        } else {
            appendLog('No tests to delete.');
        }
    }

    window.appendLog = function(message) { // Define appendLog globally
        var testCommand = document.getElementById('testCommand');
        testCommand.innerHTML += message + '\\n';
        testCommand.scrollTop = logMonitor.scrollHeight;
    }

    window.startTest = function() { // Define startTest globally
        alert('Test Started');
        appendLog('Test started at ' + new Date().toLocaleString());
    }

    window.stopTest = function() { // Define stopTest globally
        alert('Test Stopped');
        appendLog('Test stopped at ' + new Date().toLocaleString());
    }

    window.pauseTest = function() { // Define pauseTest globally
        alert('Test Paused');
        appendLog('Test paused at ' + new Date().toLocaleString());
    }
});
</script>
)rawliteral";

#endif // RAW_JSS_H
