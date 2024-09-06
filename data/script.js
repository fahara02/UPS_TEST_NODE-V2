let websocket;
const gateway = 'ws://192.168.0.108:80/ws';

 //const gateway = `ws://${window.location.hostname}/ws`;
// Initialize variables
function initAllVariables() {
  window.tests = [];
}

// Initialize WebSocket connection
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = onWebSocketOpen;
  websocket.onclose = onWebSocketClose;
  websocket.onmessage = onWebSocketMessage;
}

// WebSocket open event
function onWebSocketOpen(event) {
  console.log('WebSocket connection opened');
}

// WebSocket close event
function onWebSocketClose(event) {
  console.log('WebSocket connection closed');
  setTimeout(initWebSocket, 2000); // Try to reconnect after 2 seconds
}

// WebSocket message handler
function onWebSocketMessage(event) {
  console.log('Message from server:', event.data);
  appendLog('Message received: ' + event.data);

  try {
    const data = JSON.parse(event.data);

    // Handle specific message types
    switch (data.messageType) {
      case 'testStarted':
        appendLog('Server: Test has started');
        break;
      case 'testStopped':
        appendLog('Server: Test has stopped');
        break;
      default:
        appendLog('Server: Unknown message received');
        break;
    }

    // Update DOM elements based on message data
    updateDOMElements(data);
  } catch (error) {
    console.error('Error parsing WebSocket message:', error);
  }
}

// Update DOM elements based on WebSocket message data
function updateDOMElements(data) {
  if (data.inputPowerFactor !== undefined) {
    document.getElementById('inputPowerFactor').innerText = data.inputPowerFactor;
  }
  if (data.outputPowerFactor !== undefined) {
    document.getElementById('outputPowerFactor').innerText = data.outputPowerFactor;
  }
  if (data.inputVoltage !== undefined) {
    document.getElementById('inputVoltage').innerText = data.inputVoltage;
  }
  if (data.outputVoltage !== undefined) {
    document.getElementById('outputVoltage').innerText = data.outputVoltage;
  }
  if (data.inputCurrent !== undefined) {
    document.getElementById('inputCurrent').innerText = data.inputCurrent;
  }
  if (data.outputCurrent !== undefined) {
    document.getElementById('outputCurrent').innerText = data.outputCurrent;
  }
  if (data.inputWattage !== undefined) {
    document.getElementById('inputWattage').innerText = data.inputWattage;
  }
  if (data.outputWattage !== undefined) {
    document.getElementById('outputWattage').innerText = data.outputWattage;
  }
}

// Initialize WebSocket and set up event listeners on DOMContentLoaded
document.addEventListener('DOMContentLoaded', () => {
  initAllVariables();
  initWebSocket(); // Initialize WebSocket here to avoid undefined errors

  // Event delegation for test management
  document.addEventListener('click', (event) => {
    const targetId = event.target.id;
    switch (targetId) {
      case 'addTestButton':
        addTest();
        break;
      case 'deleteTestButton':
        deleteTest();
        break;
      case 'clearTestButton':
        clearTest();
        break;
      case 'sendTestButton':
        sendTest();
        break;
      case 'startTestButton':
        startTest();
        break;
      case 'stopTestButton':
        stopTest();
        break;
      case 'pauseTestButton':
        pauseTest();
        break;
      case 'sendModeButton':
        sendMode();
        break;
      case 'toggleSidebar':
        toggleSidebar();
        break;
      default:
        break;
    }
  });

  // Event listeners for switches
  document.addEventListener('change', (event) => {
    if (event.target.matches('#toggleLoadSwitch')) {
      updateLoadSwitchLabel(event.target.checked);
    } else if (event.target.matches('#togglePowerCutSwitch')) {
      updatePowerCutSwitchLabel(event.target.checked);
    }
  });
});

// Toggle sidebar visibility
function toggleSidebar() {
  const sidebar = document.getElementById('sidebar');
  if (sidebar) {
    sidebar.classList.toggle('hidden');
  } else {
    console.error('Sidebar element not found.');
  }
}

// Update label for load switch
function updateLoadSwitchLabel(isChecked) {
  document.getElementById('loadStateLabel').innerText = isChecked ? 'Load On' : 'Load Off';
}

// Update label for power cut switch
function updatePowerCutSwitchLabel(isChecked) {
  document.getElementById('powerCutLabel').innerText = isChecked ? 'Mains On' : 'Mains Off';
}

// Add test to the list
function addTest() {
  if (!window.tests) {
    console.error('Tests array is not initialized.');
    window.tests = [];
  }

  const testSelect = document.getElementById('addTest');
  const loadLevelSelect = document.getElementById('loadLevel');

  if (testSelect && loadLevelSelect) {
    const selectedTest = testSelect.options[testSelect.selectedIndex].text;
    const loadLevel = loadLevelSelect.value;
    const testDetails = {
      testName: selectedTest,
      loadLevel: loadLevel + '%',
    };
    window.tests.push(testDetails);
    appendLog('Added Test: ' + JSON.stringify(testDetails) + ' at ' + new Date().toLocaleString());
  } else {
    console.error('Test select or load level select not found.');
  }
}

// Delete last test
function deleteTest() {
  if (window.tests && window.tests.length > 0) {
    const deletedTest = window.tests.pop();
    appendLog('Deleted Test: ' + JSON.stringify(deletedTest) + ' at ' + new Date().toLocaleString());
    appendLog('Remaining Tests: ' + JSON.stringify(window.tests));
  } else {
    appendLog('No tests to delete.');
  }
}

// Clear the test log
function clearTest() {
  const testCommand = document.getElementById('testCommand');
  if (testCommand) {
    testCommand.innerHTML = '';
  } else {
    console.error('Test Command element not found');
  }
}

function sendTest() {
  var sendTest = window.tests;
  appendLog('Sending Test: ' + JSON.stringify(sendTest) + ' at ' + new Date().toLocaleString());
  sendTestData(); // Send the test data to the server whenever a test is added
}

// Append logs to the UI
function appendLog(message) {
  const testCommand = document.getElementById('testCommand');
  if (testCommand) {
    testCommand.textContent += message + '\n';
    testCommand.scrollTop = testCommand.scrollHeight;
  } else {
    console.error('Log element not found.');
  }
}

// Start test via WebSocket
function startTest() {
  appendLog('Test started at ' + new Date().toLocaleString());
  sendWebSocketCommand('start');
}

// Stop test via WebSocket
function stopTest() {
  appendLog('Test stopped at ' + new Date().toLocaleString());
  sendWebSocketCommand('stop');
}

// Pause test via WebSocket
function pauseTest() {
  appendLog('Test paused at ' + new Date().toLocaleString());
  sendWebSocketCommand('pause');
}

// Send mode via WebSocket
function sendMode() {
  const modeSelect = document.querySelector('input[name="mode"]:checked');
  const mode = modeSelect ? modeSelect.value : null;
  appendLog('Sending Mode: ' + mode + ' at ' + new Date().toLocaleString());
  sendWebSocketCommand(mode);
}

// Send command via WebSocket
function sendWebSocketCommand(command) {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    websocket.send(command); // Send the command as a string
  } else {
    console.error('WebSocket connection is not open.');
  }
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