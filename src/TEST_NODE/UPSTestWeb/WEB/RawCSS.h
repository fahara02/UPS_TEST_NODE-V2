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
  align-items: center;
  justify-content: space-between;
  background-color: #333;
  color: white;
  padding: 10px 20px; /* This controls the top bar's height */
}

header .logo-container {
  background-color: white;
  display: flex;
  align-items: center;
  padding: 0 5px;
  margin-right: 20px;
  height: 50px; /* Set a fixed height to match the top bar */
}

header img {
  height: 100%;
  margin: 0;
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

      /* Dropdown Styles */
      .dropdown {
        position: relative;
        display: inline-block;
      }

      .dropbtn {
        background-color: #0077cc; /* Match navbar background */
        color: white;
        padding: 10px 20px;
        font-size: 16px;
        border: none;
        cursor: pointer;
        font-weight: bold;
        border-radius: 5px;
        transition: background-color 0.3s;
      }

      .dropbtn:hover {
        background-color: #005a99;
      }

      .dropdown-content {
        display: none;
        position: absolute;
        background-color: #f4f4f4; /* Match side panel background */
        min-width: 220px;
        box-shadow: 0px 8px 16px 0px rgba(0, 0, 0, 0.2);
        z-index: 1;
        border-radius: 5px;
      }

      .dropdown-content a,
      .dropdown-content button {
        color: #333; /* Match side panel text color */
        padding: 12px 20px;
        text-decoration: none;
        display: block;
        text-align: left;
        font-weight: bold;
        font-size: 14px;
        border: none;
        background: none;
        width: 100%;
        box-sizing: border-box;
        transition: background-color 0.3s;
      }

      .dropdown-content a:hover,
      .dropdown-content button:hover {
        background-color: #e0e0e0; /* Hover color for options */
      }

      .dropdown:hover .dropdown-content {
        display: block;
      }

      /* Update button styling */
      #update-settings-button {
        cursor: pointer;
        background-color: #0066b3; /* Button color matching the navbar */
        color: white;
        font-weight: bold;
        border-radius: 5px;
        padding: 10px 20px;
        font-size: 16px; /* Font size increased */
        text-align: center;
        width: 100%;
        box-sizing: border-box;
        transition: background-color 0.3s;
      }

      #update-settings-button:hover {
        background-color: #005a99; /* Darken button on hover */
      }

      #update-settings-button:hover {
        background-color: #275a8e; /* Darken button on hover */
      }

      /* Sidebar Styles */
      .sidebar {
        width: 220px;
        background-color: #2c3e50;
        color: white;
        padding: 20px;
        position: fixed;
        top: 129px; /* Adjusted to be below header and navbar */
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
