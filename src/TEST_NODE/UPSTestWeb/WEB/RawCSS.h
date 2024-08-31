#ifndef RAW_CSS_H
#define RAW_CSS_H
#include <pgmspace.h>
const char STYLE_BLOCK_CSS[] PROGMEM = R"rawliteral(
   <style>
    body {
        margin: 0;
        font-family: Arial, sans-serif;
        background-color: #f4f4f9;
        color: #333;
    }
    .navbar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 10px 20px;
        background-color: #0077cc; /* Primary blue color */
        color: white;
    }
    .navbar a {
        color: white;
        margin: 0 15px;
        text-decoration: none;
        font-weight: bold;
    }
    .navbar a:hover {
        text-decoration: underline;
    }
    .button {
        padding: 8px 15px;
        margin-left: 10px;
        background-color: #0066b3; /* Slightly darker blue */
        border: none;
        color: white;
        border-radius: 5px;
        cursor: pointer;
        font-weight: bold;
    }
    .button:hover {
        background-color: #005a99; /* Darker blue on hover */
    }
    .sidebar {
        width: 220px;
        background-color: #2c3e50; /* Darker blue-gray for sidebar */
        color: white;
        padding: 20px;
        position: fixed;
        top: 0;
        left: 0;
        height: 100%;
        transition: transform 0.3s ease;
        transform: translateX(0);
        box-shadow: 2px 0 10px rgba(0, 0, 0, 0.1);
    }
    .sidebar.hidden {
        transform: translateX(-100%);
    }
    .sidebar button,
    .sidebar label,
    .sidebar select,
    .sidebar p {
        color: white;
        background-color: #34495e; /* Dark blue for sidebar elements */
        border: none;
        border-radius: 5px;
        padding: 10px;
        margin-bottom: 10px;
        width: 100%;
    }
    .sidebar button:hover,
    .sidebar select:hover {
        background-color: #3b5998; /* Lighter blue on hover */
    }
    .sidebar select {
        width: 100%;
        padding: 8px;
        border-radius: 5px;
    }
    .dropdown {
        margin-bottom: 10px;
    }
    .dropdown label {
        margin-bottom: 5px; /* Small margin between label and select */
        display: block;
    }
    .dropdown:first-of-type {
        margin-top: 10px; /* Margin between the first dropdown and the element above it */
    }
    .toggler {
        margin-bottom: 10px;
    }
    .toggler label {
        display: block;
        margin-bottom: 5px;
        font-weight: bold;
    }
    .toggler input[type="radio"] {
        margin-right: 10px;
    }
    .container {
        display: flex;
        justify-content: flex-start;
        gap: 20px;
        margin: 30px;
        margin-left: 250px;
        transition: margin-left 0.3s ease;
    }
     .sidebar.hidden {
        transform: translateX(-100%);
    }
    .sidebar.hidden ~ .container {
        margin-left: 50px;
    }
    .user-command, .log-output {
        width: 45%;
        padding: 20px;
        border-radius: 8px;
        box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
        overflow-y: auto;
    }
    .user-command {
        background-color: #f9f9f9;
        border: 1px solid #ddd;
    }
    .log-output {
        background-color: #1e1e1e;
        color: #00ff00; /* Green text for logs */
        font-family: monospace;
        white-space: pre-wrap;
    }
</style>
)rawliteral";

#endif
