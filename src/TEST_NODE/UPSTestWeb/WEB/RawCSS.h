#ifndef RAW_CSS_H
#define RAW_CSS_H
#include <pgmspace.h>

const char STYLE_BLOCK_CSS[] PROGMEM = R"rawliteral(
      <style>
      /* Reset and base styles */
      * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
      }
      body {
        font-family: Arial, sans-serif;
        background-color: #f4f4f9;
        color: #333;
        display: flex;
        flex-direction: column;
        min-height: 100vh;
      }

      /* Header Styles */
      header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 10px 20px;
        background-color: #333;
        color: white;
      }
      header img {
        height: 50px;
        margin-right: 20px;
      }
      header h1 {
        flex-grow: 1;
        text-align: center;
        font-size: 24px;
      }

      /* Navbar Styles */
      .navbar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 10px 20px;
        background-color: #0077cc;
        color: white;
      }
      .navbar a {
        color: white;
        margin: 0 10px;
        text-decoration: none;
        font-weight: bold;
        transition: color 0.3s;
      }
      .navbar a:hover {
        color: #ffdd57;
      }
      .navbar .buttons {
        display: flex;
        align-items: center;
      }
      .navbar .button {
        padding: 8px 15px;
        margin-left: 10px;
        background-color: #0066b3;
        border: none;
        color: white;
        border-radius: 5px;
        cursor: pointer;
        font-weight: bold;
        transition: background-color 0.3s;
      }
      .navbar .button:hover {
        background-color: #005a99;
      }

      /* Sidebar Styles */
      .sidebar {
        width: 220px;
        background-color: #2c3e50;
        color: white;
        padding: 20px;
        position: fixed;
        top: 100px; /* Adjusted to be below header and navbar */
        left: 0;
        height: calc(100% - 100px); /* Adjust height accordingly */
        transition: transform 0.3s ease, visibility 0.3s ease;
        transform: translateX(0);
        visibility: visible;
        box-shadow: 2px 0 10px rgba(0, 0, 0, 0.1);
        overflow-y: auto;
      }
      .sidebar.hidden {
        transform: translateX(-100%);
        visibility: hidden;
      }
      .sidebar h2 {
        margin-bottom: 15px;
        font-size: 20px;
        border-bottom: 1px solid #34495e;
        padding-bottom: 5px;
      }
      .sidebar button,
      .sidebar label,
      .sidebar select,
      .sidebar p {
        color: white;
        background-color: #34495e;
        border: none;
        border-radius: 5px;
        padding: 10px;
        margin-bottom: 10px;
        width: 100%;
        font-size: 16px;
      }
      .sidebar button:hover,
      .sidebar select:hover {
        background-color: #3b5998;
      }
      .sidebar .dropdown {
        margin-bottom: 20px;
      }
      .sidebar button:nth-child(4) {
        margin-bottom: 20px; /* Adjust the margin as needed */
      }
      .sidebar .toggler {
        margin-top: 20px;
      }

      .sidebar .mode-button {
        display: block;
        margin-bottom: 10px; /* Similar to the original label's margin */
        font-weight: bold;
        background-color: #34495e;
        color: white;
        border: none;
        border-radius: 5px;
        padding: 10px 15px;
        font-size: 16px;
        cursor: pointer;
        transition: background-color 0.3s;
      }

      .sidebar .mode-button:hover {
        background-color: #3b5998;
      }

      .sidebar .radio-group {
        display: flex;
        align-items: center;
        margin-top: 10px; /* Similar margin to the previous structure */
      }

      .sidebar .radio-group label {
        display: flex;
        align-items: center;
        margin-right: 20px; /* Ensures spacing between radio buttons */
        font-weight: normal;
      }

      .sidebar .radio-group input {
        margin-right: 5px;
      }

      /* Main Content Styles */
      .container {
        display: flex;
        flex: 1;
        margin: 20px;
        margin-left: 250px; /* Adjusted to accommodate sidebar width */
        transition: margin-left 0.3s ease;
      }
      .sidebar.hidden ~ .container {
        margin-left: 50px;
      }
      .user-command,
      .log-output {
        width: 50%;
        padding: 20px;
        border-radius: 8px;
        box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
        overflow-y: auto;
        background-color: #fff;
      }
      .user-command {
        margin-right: 20px;
      }
      .user-command h2,
      .log-output h2 {
        margin-bottom: 10px;
        font-size: 18px;
        border-bottom: 1px solid #ddd;
        padding-bottom: 5px;
      }
      .log-output {
        background-color: #1e1e1e;
        color: #00ff00;
        font-family: monospace;
        white-space: pre-wrap;
      }

      /* Responsive Adjustments */
      @media (max-width: 768px) {
        .navbar,
        header {
          flex-direction: column;
          align-items: flex-start;
        }
        .navbar .buttons {
          margin-top: 10px;
        }
        .container {
          flex-direction: column;
          margin-left: 0;
        }
        .user-command,
        .log-output {
          width: 100%;
          margin-right: 0;
          margin-bottom: 20px;
        }
        .sidebar {
          top: 150px; /* Adjust if navbar height changes */
          height: calc(100% - 150px);
        }
      }
    </style>
)rawliteral";

#endif
