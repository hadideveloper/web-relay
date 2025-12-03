# ESP32 Web Relay Firmware

A smart relay controller firmware for ESP32 that enables remote control of relays through HTTP requests, web interface, and UART commands.

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Features](#features)
- [Code Structure](#code-structure)
- [Building the Firmware](#building-the-firmware)
- [Programming the ESP32](#programming-the-esp32)
- [Configuration](#configuration)
- [UART Commands](#uart-commands)
- [Web Server API](#web-server-api)
- [HTTP Polling & JSON Protocol](#http-polling--json-protocol)
- [How It Works](#how-it-works)
- [Troubleshooting](#troubleshooting)

## Overview

This firmware turns an ESP32 into a smart relay controller that can:
- Control 2 relays via GPIO pins
- Connect to WiFi networks
- Poll a remote server for commands via HTTP GET requests
- Provide a web interface for local control
- Accept commands via UART serial interface
- Send acknowledgment (ACK) messages back to the server via HTTP POST

The device operates in a polling mode, checking the configured server URL every 2 seconds for new commands when WiFi is connected.

## Hardware Requirements

- **ESP32 Development Board** (ESP32-WROOM-32 or compatible)
- **2 Relays** connected to:
  - Relay 1: GPIO 16
  - Relay 2: GPIO 17
- **LED** (optional, for WiFi status indication)
- **USB Cable** for programming and serial communication
- **Power Supply** appropriate for your relay module

### GPIO Pinout

| Function | GPIO Pin |
|----------|----------|
| Relay 1  | GPIO 16  |
| Relay 2  | GPIO 17  |
| UART TX  | GPIO 1   |
| UART RX  | GPIO 3   |

## Features

- ✅ WiFi Station mode with automatic reconnection
- ✅ Persistent storage of WiFi credentials and server URL (NVS)
- ✅ HTTP client with polling mechanism (every 2 seconds)
- ✅ Embedded web server for local control
- ✅ UART command interface
- ✅ JSON-based command protocol
- ✅ Automatic relay timer (duration-based control)
- ✅ Command acknowledgment (ACK) via HTTP POST
- ✅ LED status indicator for WiFi connection

## Code Structure

```
firmware/
├── main/
│   ├── inc/              # Header files
│   │   ├── com.h         # UART command parsing
│   │   ├── http.h        # HTTP client functions
│   │   ├── led.h         # LED control
│   │   ├── relay.h       # Relay control
│   │   ├── server.h       # JSON response processing
│   │   ├── uart.h        # UART communication
│   │   ├── webserver.h   # Web server functions
│   │   └── wifi.h        # WiFi management
│   └── src/              # Source files
│       ├── main.c        # Main application entry point
│       ├── com.c         # Command parsing and queue
│       ├── http.c        # HTTP client implementation
│       ├── led.c         # LED GPIO control
│       ├── relay.c       # Relay GPIO control
│       ├── server.c      # JSON parsing and command execution
│       ├── uart.c        # UART driver
│       ├── webserver.c   # HTTP server implementation
│       └── wifi.c        # WiFi connection management
├── CMakeLists.txt        # Main CMake configuration
├── sdkconfig            # ESP-IDF configuration
└── sdkconfig.defaults   # Default configuration values
```

### Module Descriptions

- **main.c**: Application entry point, initializes all modules and main event loop
- **wifi.c**: WiFi station mode, connection management, credential storage
- **http.c**: HTTP client for polling server and sending POST requests
- **webserver.c**: Embedded HTTP server for local web interface
- **server.c**: JSON parsing, command execution, relay timer management
- **relay.c**: GPIO control for relay outputs
- **com.c**: UART command parsing and queue management
- **uart.c**: Low-level UART communication

## Building the Firmware

### Prerequisites

1. **ESP-IDF Framework** (v5.0 or later recommended)
   ```bash
   # Install ESP-IDF (see official documentation)
   # https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   ```

2. **Python 3.6+** (required by ESP-IDF)

3. **CMake 3.16+**

### Build Steps

1. **Set up ESP-IDF environment**:
   ```bash
   # On Linux/macOS:
   . $HOME/esp/esp-idf/export.sh
   
   # On Windows:
   # Run ESP-IDF Command Prompt or:
   %userprofile%\esp\esp-idf\export.bat
   ```

2. **Navigate to firmware directory**:
   ```bash
   cd ESP32/firmware
   ```

3. **Configure the project** (optional):
   ```bash
   idf.py menuconfig
   ```
   - Navigate to `Component config` → `ESP32-specific` to adjust CPU frequency
   - Default is 240 MHz

4. **Build the project**:
   ```bash
   idf.py build
   ```

5. **Build output**:
   - Binary file: `build/esp32-hello-world.bin`
   - ELF file: `build/esp32-hello-world.elf`

## Programming the ESP32

### Method 1: Using idf.py (Recommended)

1. **Connect ESP32** to your computer via USB

2. **Flash the firmware**:
   ```bash
   idf.py -p COM3 flash
   ```
   Replace `COM3` with your serial port:
   - Windows: `COM3`, `COM4`, etc.
   - Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.
   - macOS: `/dev/cu.usbserial-*`, `/dev/cu.SLAB_USBtoUART`, etc.

3. **Monitor serial output**:
   ```bash
   idf.py -p COM3 monitor
   ```
   Or combine flash and monitor:
   ```bash
   idf.py -p COM3 flash monitor
   ```

4. **Exit monitor**: Press `Ctrl+]`

### Method 2: Using esptool.py Directly

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/esp32-hello-world.bin
```

### Finding Your Serial Port

**Windows:**
```powershell
# PowerShell
Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description
```

**Linux:**
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

**macOS:**
```bash
ls /dev/cu.*
```

## Configuration

### Initial Setup via UART

1. **Connect to serial port** (115200 baud, 8N1)
2. **Set WiFi credentials**:
   ```
   SSID=YourWiFiNetwork
   WIFIPASS=YourPassword
   ```
3. **Set server URL**:
   ```
   URL=https://your-server.com/api/relay
   ```

### Configuration Storage

All configuration is stored in **NVS (Non-Volatile Storage)**:
- WiFi SSID and password: Namespace `wifi`
- Server URL: Namespace `http`
- Settings persist across reboots

### Default Configuration

- **CPU Frequency**: 240 MHz
- **UART Baud Rate**: 115200
- **Web Server Port**: 80
- **HTTP Polling Interval**: 2000 ms (2 seconds)
- **HTTP Timeout**: 10000 ms (10 seconds)

## UART Commands

The firmware accepts commands via UART (115200 baud). Commands must end with `<CR>` (carriage return, `\r`) or `<LF>` (line feed, `\n`).

### Relay Control Commands

| Command | Description |
|---------|-------------|
| `relay1 on` | Turn Relay 1 ON |
| `relay1 off` | Turn Relay 1 OFF |
| `relay2 on` | Turn Relay 2 ON |
| `relay2 off` | Turn Relay 2 OFF |
| `led on` | Turn LED ON |
| `led off` | Turn LED OFF |

### WiFi Configuration Commands

| Command | Description | Response |
|---------|-------------|----------|
| `SSID=<ssid>` | Set WiFi SSID | `OK` or `ERROR` |
| `SSID?` | Query stored SSID | SSID string or `NOT_SET` |
| `WIFIPASS=<password>` | Set WiFi password | `OK` or `ERROR` |
| `WIFIPASS?` | Query stored password | Masked password or `NOT_SET` |

**Note**: Password query returns a masked version (e.g., `abc***xy`).

### Server Configuration Commands

| Command | Description | Response |
|---------|-------------|----------|
| `URL=<url>` | Set server URL | `OK` or `ERROR` |
| `URL?` | Query stored URL | URL string or `NOT_SET` |
| `IP?` | Query current IP address | IP address or `NOT_CONNECTED` |

### Command Examples

```
relay1 on
relay2 off
SSID=MyWiFi
WIFIPASS=mypassword123
URL=https://api.example.com/relay
IP?
```

## Web Server API

The firmware includes an embedded HTTP server running on port 80 (default). Access it via the ESP32's IP address.

### Endpoints

#### GET `/`
Returns the main control page (HTML).

**Response**: HTML page with:
- Current IP address
- Server URL configuration form
- Relay 1 control buttons and status
- Relay 2 control buttons and status

#### GET `/relay1/on`
Turns Relay 1 ON.

**Response**: HTTP 303 redirect to `/`

#### GET `/relay1/off`
Turns Relay 1 OFF.

**Response**: HTTP 303 redirect to `/`

#### GET `/relay2/on`
Turns Relay 2 ON.

**Response**: HTTP 303 redirect to `/`

#### GET `/relay2/off`
Turns Relay 2 OFF.

**Response**: HTTP 303 redirect to `/`

#### POST `/seturl`
Sets the server URL for HTTP polling.

**Content-Type**: `application/x-www-form-urlencoded`

**Body**: `url=<encoded_url>`

**Example**:
```bash
curl -X POST http://192.168.1.100/seturl -d "url=https://api.example.com/relay"
```

**Response**: HTTP 303 redirect to `/` on success, HTTP 400 on error

### Web Interface Usage

1. Connect to the ESP32's WiFi network or ensure it's on your local network
2. Open a browser and navigate to `http://<esp32_ip_address>`
3. Use the web interface to:
   - View current IP address
   - Configure server URL
   - Control relays with ON/OFF buttons
   - View relay status

## HTTP Polling & JSON Protocol

### Polling Mechanism

The firmware polls the configured server URL every **2 seconds** when:
- WiFi is connected
- URL is configured

The polling uses **HTTP GET** requests to fetch commands.

### JSON Command Format

The server should return JSON in the following format:

```json
{
  "command_id": "unique-command-id-123",
  "relay1": {
    "state": 1,
    "duration": 5000
  },
  "relay2": {
    "state": 0
  }
}
```

#### JSON Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `command_id` | string | Optional | Unique identifier for acknowledgment |
| `relay1` | object | Optional | Command for Relay 1 |
| `relay2` | object | Optional | Command for Relay 2 |

#### Relay Object Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `state` | integer | Yes | `1` = ON, `0` = OFF |
| `duration` | integer | No | Auto-off duration in milliseconds (only used when `state` is `1`) |

#### Examples

**Turn Relay 1 ON for 5 seconds:**
```json
{
  "command_id": "cmd-001",
  "relay1": {
    "state": 1,
    "duration": 5000
  }
}
```

**Turn both relays ON (Relay 2 permanently):**
```json
{
  "command_id": "cmd-002",
  "relay1": {
    "state": 1,
    "duration": 10000
  },
  "relay2": {
    "state": 1
  }
}
```

**Turn Relay 1 OFF:**
```json
{
  "command_id": "cmd-003",
  "relay1": {
    "state": 0
  }
}
```

**No commands (empty response):**
```json
{}
```

### Acknowledgment (ACK) via POST

When a command includes a `command_id`, the firmware sends an acknowledgment via **HTTP POST** to the same URL:

**Request**:
- Method: `POST`
- Content-Type: `application/json`
- Body:
```json
{
  "command_id": "unique-command-id-123",
  "status": "received"
}
```

**Response**: Server should return HTTP 200-299 for success.

### Backward Compatibility

For backward compatibility, the firmware also supports simple string responses:
- `"0"` → Turn Relay 1 OFF
- `"1"` → Turn Relay 1 ON

## How It Works

### Startup Sequence

1. **Initialization**:
   - UART, LED, and Relays are initialized
   - Communication module (COM) starts UART reading task
   - WiFi module initializes and loads credentials from NVS
   - HTTP client initializes and loads URL from NVS
   - Web server starts on port 80

2. **WiFi Connection**:
   - If credentials exist in NVS, automatically connects
   - LED turns ON when connected
   - LED turns OFF when disconnected

3. **HTTP Polling**:
   - HTTP polling task starts after 2 seconds
   - Polls server URL every 2 seconds when WiFi is connected
   - Processes JSON responses and executes relay commands
   - Sends ACK via POST if `command_id` is present

### Main Event Loop

The main loop in `main.c`:
- Monitors WiFi connection status and updates LED
- Processes commands from UART queue
- Executes relay/LED commands
- Handles WiFi and URL configuration commands

### Command Flow

```
┌─────────────┐
│   Server    │
│  (Internet) │
└──────┬──────┘
       │ HTTP GET (every 2s)
       ▼
┌─────────────┐
│  ESP32 HTTP │
│   Client    │
└──────┬──────┘
       │ JSON Response
       ▼
┌─────────────┐
│   Server    │
│  (server.c) │
└──────┬──────┘
       │ Parse & Execute
       ▼
┌─────────────┐
│   Relay     │
│  Control    │
└─────────────┘
       │
       │ HTTP POST (ACK)
       ▼
┌─────────────┐
│   Server    │
│  (Internet) │
└─────────────┘
```

### Relay Timer Feature

When a relay command includes a `duration` field:
1. Relay is turned ON
2. A FreeRTOS task is created with the specified duration
3. After the duration expires, the relay is automatically turned OFF
4. The task is deleted

This allows for timed operations like "turn on for 5 seconds".

## Troubleshooting

### WiFi Connection Issues

**Problem**: ESP32 doesn't connect to WiFi

**Solutions**:
- Verify SSID and password are correct: `SSID?` and `WIFIPASS?`
- Check WiFi signal strength
- Ensure 2.4 GHz network (ESP32 doesn't support 5 GHz)
- Check serial monitor for error messages
- Try resetting NVS: `idf.py erase-flash` (will erase all stored data)

### HTTP Polling Not Working

**Problem**: No HTTP requests are being made

**Solutions**:
- Verify URL is set: `URL?`
- Check WiFi connection: `IP?`
- Verify server URL is accessible (test with browser/curl)
- Check serial monitor for HTTP error messages
- Ensure server returns valid JSON or empty object `{}`

### Web Server Not Accessible

**Problem**: Can't access web interface

**Solutions**:
- Verify ESP32 IP address: `IP?`
- Ensure device is on the same network
- Check firewall settings
- Try accessing via IP directly: `http://192.168.1.100`
- Check serial monitor for web server errors

### Relay Not Responding

**Problem**: Relays don't turn ON/OFF

**Solutions**:
- Verify GPIO connections (GPIO 16 for Relay 1, GPIO 17 for Relay 2)
- Check relay module power supply
- Test with UART commands: `relay1 on`, `relay1 off`
- Check serial monitor for relay control messages
- Verify relay module is compatible (active HIGH/LOW)

### Build Errors

**Problem**: `idf.py build` fails

**Solutions**:
- Ensure ESP-IDF is properly installed and sourced
- Check CMake version: `cmake --version` (need 3.16+)
- Clean build: `idf.py fullclean && idf.py build`
- Verify all source files are present
- Check for syntax errors in code

### Serial Monitor Issues

**Problem**: No output in serial monitor

**Solutions**:
- Verify correct serial port: `idf.py -p COM3 monitor`
- Check baud rate (default: 115200)
- Try different USB cable/port
- Close other programs using the serial port
- On Windows: Check Device Manager for COM port conflicts

## Additional Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [ESP32 Hardware Reference](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [cJSON Library](https://github.com/DaveGamble/cJSON)

## License

See the main project LICENSE file.

## Contributing

Contributions are welcome! Please ensure code follows ESP-IDF coding standards and includes appropriate error handling and logging.

