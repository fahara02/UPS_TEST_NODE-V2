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

    window.addTest = function() {
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
        sendTestData(); // Send the test data to the server whenever a test is added
    }

    window.deleteTest = function() {
        if (window.tests.length > 0) {
            var deletedTest = window.tests.pop();
            appendLog('Test deleted: ' + JSON.stringify(deletedTest) + ' at ' + new Date().toLocaleString());
            appendLog('Remaining tests: ' + JSON.stringify(window.tests));
            sendTestData(); // Send the updated test data to the server whenever a test is deleted
        } else {
            appendLog('No tests to delete.');
        }
    }

    window.appendLog = function(message) {
        var testCommand = document.getElementById('testCommand');
        if (testCommand) {
            testCommand.innerHTML += message + '\\n';
            testCommand.scrollTop = testCommand.scrollHeight; // Corrected to use testCommand
        } else {
            console.error('Element testCommand not found');
        }
    }

    window.startTest = function() {
        alert('Test Started');
        appendLog('Test started at ' + new Date().toLocaleString());
        sendStatus('start'); // Send start status to the server
    }

    window.stopTest = function() {
        alert('Test Stopped');
        appendLog('Test stopped at ' + new Date().toLocaleString());
        sendStatus('stop'); // Send stop status to the server
    }

    window.pauseTest = function() {
        alert('Test Paused');
        appendLog('Test paused at ' + new Date().toLocaleString());
        sendStatus('pause'); // Send pause status to the server
    }

    function sendTestData() {
        fetch('/updateTestData', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(window.tests)
        })
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.error('Error:', error));
    }

    function sendStatus(status) {
        fetch('/updateStatus', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({status: status})
        })
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.error('Error:', error));
    }
});
</script>
)rawliteral";

#endif // RAW_JSS_H
