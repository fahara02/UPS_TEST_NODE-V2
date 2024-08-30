#ifndef RAW_CSS_H
#define RAW_CSS_H
#include <pgmspace.h>
const char STYLE_BLOCK_CSS[] PROGMEM = R"rawliteral(
   <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #f0f0f0;
        margin: 0;
      }
      .navbar {
        background-color: black;
        padding: 1em;
        display: flex;
        justify-content: space-between;
        align-items: center;
        color: white;
        position: relative;
      }
      .navbar-title {
        font-size: 1.5em;
        margin-left: 20px;
      }
      .navbar a {
        color: white;
        text-decoration: none;
        margin: 0 15px;
      }
      .sidebar {
        height: 100vh;
        width: 200px;
        background-color: #333;
        color: white;
        padding: 15px;
        position: fixed;
        top: 0;
        left: 0;
        transition: width 0.3s;
        overflow: hidden;
      }

      .resizer {
        width: 10px;
        height: 100%;
        background-color: #aaa;
        position: fixed;
        left: 200px; /* Initial position right after the sidebar */
        top: 0;
        cursor: ew-resize; /* Left-right drag cursor */
      }

      .content {
        margin-left: 210px; /* Adjusted to match resizer position */
        padding: 20px;
        transition: margin-left 0.3s;
      }

      .sidebar button {
        display: block;
        width: 100%;
        margin: 10px 0;
        padding: 10px;
        background-color: blue;
        border: none;
        color: white;
        cursor: pointer;
      }
      .sidebar button:hover {
        background-color: red;
      }

      .dropdown {
        background-color: white;
        color: black;
        padding: 10px;
        width: 100%;
      }
      .toggler {
        margin-top: 20px;
        padding: 10px;
      }
      .button {
        background-color: red;
        color: white;
        padding: 10px 20px;
        border: none;
        cursor: pointer;
      }
      .button:hover {
        background-color: blue;
      }
      .log-monitor {
        margin-top: 20px;
        width: 100%;
        height: 300px;
        background-color: #fff;
        border: 1px solid #ccc;
        padding: 10px;
        overflow-y: scroll;
        font-family: monospace;
        white-space: pre-wrap;
        background-color: #f5f5f5;
      }
)rawliteral";
#endif