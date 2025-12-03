# Web Relay - Smart IoT Relay Controller

A complete IoT solution for remote relay control, consisting of ESP32 firmware and an example web server. The ESP32 device polls a server for commands via HTTP GET requests and sends acknowledgments via HTTP POST, enabling reliable remote control of relays over the internet.

<img width="879" height="838" alt="image" src="https://github.com/user-attachments/assets/0d59ddf8-f34f-43c7-a871-425bf5f8dae6" />

<img width="655" height="884" alt="image" src="https://github.com/user-attachments/assets/7efa3a93-3a08-45f2-9ac6-931242227a7f" />



## 🌟 Features

- **ESP32 Firmware**

  - WiFi Station mode with automatic reconnection
  - HTTP polling mechanism (every 2 seconds)
  - Embedded web server for local control
  - UART command interface
  - JSON-based command protocol
  - Automatic relay timer (duration-based control)
  - Command acknowledgment (ACK) via HTTP POST
  - Persistent storage (NVS) for WiFi credentials and server URL
  - LED status indicator for WiFi connection

- **Example Server (Blazor)**
  - Modern web interface for relay control
  - RESTful API endpoints for ESP32 communication
  - Command queuing and acknowledgment tracking
  - Real-time state updates
  - Responsive UI with dark theme

## 📋 Table of Contents

- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Components](#components)
- [API Documentation](#api-documentation)
- [JSON Protocol](#json-protocol)
- [Hardware Requirements](#hardware-requirements)
- [Documentation](#documentation)
- [License](#license)
- [Contributing](#contributing)

## 🏗️ Architecture

```
┌─────────────────┐
│  Web Browser    │
│  (Blazor UI)    │
└────────┬────────┘
         │ HTTP
         ▼
┌─────────────────┐      HTTP GET (poll every 2s)      ┌─────────────┐
│  Example Server │ ◄───────────────────────────────── │    ESP32     │
│  (Blazor/.NET)  │                                    │   Firmware   │
└────────┬────────┘      HTTP POST (ACK)               └──────┬───────┘
         │                                                    │
         │ Queue Commands                                     │ GPIO
         │                                                    ▼
         │                                            ┌─────────────┐
         │                                            │   Relays    │
         │                                            │ (GPIO 16/17)│
         │                                            └─────────────┘
         │
         └─── Command Acknowledgment ────┘
```

### How It Works

1. **User Action**: User clicks a button in the web interface
2. **Command Queue**: Server queues the command with a unique `command_id`
3. **ESP32 Polling**: ESP32 polls `/api/relay` every 2 seconds via HTTP GET
4. **Command Delivery**: Server returns the queued command as JSON
5. **Command Execution**: ESP32 parses JSON and controls relays via GPIO
6. **Acknowledgment**: ESP32 sends ACK via HTTP POST with `command_id`
7. **State Update**: Server updates UI state upon receiving ACK

## 🚀 Quick Start

### Prerequisites

**For ESP32 Firmware:**

- ESP-IDF v5.0 or later
- Python 3.6+
- CMake 3.16+
- ESP32 development board

**For Example Server:**

- .NET 10.0 SDK or later
- Visual Studio 2022 / VS Code / Rider (optional)

### ESP32 Setup

1. **Build and flash the firmware**:

   ```bash
   cd ESP32/firmware
   idf.py build
   idf.py -p COM3 flash monitor
   ```

2. **Configure WiFi via UART** (115200 baud):

   ```
   SSID=YourWiFiNetwork
   WIFIPASS=YourPassword
   ```

3. **Get IP address**:

   ```
   IP?
   ```

4. **Configure server URL**:
   ```
   URL=http://your-server-ip:5000/api/relay
   ```

For detailed instructions, see [ESP32 Firmware README](ESP32/firmware/README.md).

### Example Server Setup

1. **Navigate to server directory**:

   ```bash
   cd ExampleServer/WebRelay.Server.Example.Blazor
   ```

2. **Run the server**:

   ```bash
   dotnet run
   ```

3. **Access the web interface**:

   - Open browser: `http://localhost:5000` (or the port shown in console)

4. **Configure ESP32**:
   - Set the ESP32 URL to: `http://<your-computer-ip>:5000/api/relay`
   - Ensure ESP32 and server are on the same network (or use port forwarding)

## 📁 Project Structure

```
web-relay/
├── ESP32/
│   └── firmware/              # ESP32 firmware source code
│       ├── main/
│       │   ├── inc/          # Header files
│       │   └── src/          # Source files
│       ├── CMakeLists.txt
│       ├── sdkconfig
│       └── README.md          # Detailed ESP32 documentation
│
├── ExampleServer/
│   └── WebRelay.Server.Example.Blazor/  # Example Blazor server
│       ├── Components/        # Blazor components
│       ├── Endpoints/         # API endpoints
│       ├── Services/           # Business logic
│       └── Program.cs         # Application entry point
│
├── LICENSE
└── README.md                  # This file
```

## 🔧 Components

### ESP32 Firmware

The ESP32 firmware provides:

- **WiFi Management**: Connection, reconnection, credential storage
- **HTTP Client**: Polling server for commands, sending ACKs
- **Web Server**: Local web interface for direct control
- **UART Interface**: Serial command interface for configuration
- **Relay Control**: GPIO control for 2 relays
- **JSON Parser**: Processes server commands

**Key Files:**

- `main.c`: Application entry point and main loop
- `http.c`: HTTP client implementation
- `webserver.c`: Embedded HTTP server
- `server.c`: JSON parsing and command execution
- `wifi.c`: WiFi connection management
- `relay.c`: GPIO relay control

**See [ESP32 Firmware README](ESP32/firmware/README.md) for complete documentation.**

### Example Server (Blazor)

A .NET 10.0 Blazor Server application that demonstrates:

- Command queuing for ESP32 polling
- RESTful API endpoints
- Real-time web interface
- Command acknowledgment tracking

**Key Components:**

- `RelayEndpoints.cs`: API endpoints (`/api/relay`)
- `RelayCommandService.cs`: Command queue and state management
- `Home.razor`: Web UI for relay control

**API Endpoints:**

- `GET /api/relay`: Returns queued command (polled by ESP32)
- `POST /api/relay`: Receives acknowledgment from ESP32
- `GET /`: Web interface for relay control

## 📡 API Documentation

### Server Endpoints

#### GET `/api/relay`

Returns the next queued command for ESP32.

**Response:**

- **200 OK** with JSON command (if command queued)
- **200 OK** with `{}` (if no command queued)

**Example Response:**

```json
{
  "command_id": "abc12345",
  "relay1": {
    "state": 1,
    "duration": 5000
  }
}
```

#### POST `/api/relay`

Receives acknowledgment from ESP32.

**Request Body:**

```json
{
  "command_id": "abc12345",
  "status": "received"
}
```

**Response:**

- **200 OK**: Acknowledgment received

### ESP32 Web Server Endpoints

The ESP32 also runs a local web server (port 80):

- `GET /`: Main control page
- `GET /relay1/on`: Turn Relay 1 ON
- `GET /relay1/off`: Turn Relay 1 OFF
- `GET /relay2/on`: Turn Relay 2 ON
- `GET /relay2/off`: Turn Relay 2 OFF
- `POST /seturl`: Set server URL

## 📦 JSON Protocol

### Command Format

Commands sent from server to ESP32:

```json
{
  "command_id": "unique-id-8-chars",
  "relay1": {
    "state": 1,
    "duration": 5000
  },
  "relay2": {
    "state": 0
  }
}
```

**Fields:**

- `command_id` (string, optional): Unique identifier for acknowledgment
- `relay1` (object, optional): Command for Relay 1
- `relay2` (object, optional): Command for Relay 2

**Relay Object:**

- `state` (integer, required): `1` = ON, `0` = OFF
- `duration` (integer, optional): Auto-off duration in milliseconds (only when `state` is `1`)

### Acknowledgment Format

ACK sent from ESP32 to server:

```json
{
  "command_id": "unique-id-8-chars",
  "status": "received"
}
```

### Examples

**Turn Relay 1 ON for 5 seconds:**

```json
{
  "command_id": "cmd001",
  "relay1": {
    "state": 1,
    "duration": 5000
  }
}
```

**Turn Relay 2 OFF:**

```json
{
  "command_id": "cmd002",
  "relay2": {
    "state": 0
  }
}
```

**No command (empty response):**

```json
{}
```

## 🔌 Hardware Requirements

### ESP32 Development Board

- ESP32-WROOM-32 or compatible
- USB cable for programming
- Power supply (5V via USB or external)

### Relays

- 2 relay modules (active HIGH or LOW compatible)
- Connected to:
  - **Relay 1**: GPIO 16
  - **Relay 2**: GPIO 17

### Optional

- LED for WiFi status indication
- Breadboard and jumper wires

### GPIO Pinout

| Function | GPIO Pin |
| -------- | -------- |
| Relay 1  | GPIO 16  |
| Relay 2  | GPIO 17  |
| UART TX  | GPIO 1   |
| UART RX  | GPIO 3   |

## 📚 Documentation

- **[ESP32 Firmware README](ESP32/firmware/README.md)**: Complete ESP32 firmware documentation
  - Building instructions
  - Programming guide
  - UART commands
  - Web server API
  - JSON protocol details
  - Troubleshooting

## 🛠️ Development

### Building ESP32 Firmware

```bash
cd ESP32/firmware
idf.py build
```

### Running Example Server

```bash
cd ExampleServer/WebRelay.Server.Example.Blazor
dotnet run
```

### Development Workflow

1. **Modify ESP32 firmware**: Edit files in `ESP32/firmware/main/src/`
2. **Build and flash**: `idf.py build flash monitor`
3. **Modify server**: Edit files in `ExampleServer/`
4. **Test**: Use web interface or API directly

## 🔍 Testing

### Test ESP32 Locally

1. Connect ESP32 to serial monitor
2. Use UART commands:
   ```
   relay1 on
   relay2 off
   IP?
   ```

### Test Server API

```bash
# Get command (should return {})
curl http://localhost:5000/api/relay

# Send ACK
curl -X POST http://localhost:5000/api/relay \
  -H "Content-Type: application/json" \
  -d '{"command_id":"test123","status":"received"}'
```

### End-to-End Test

1. Start the example server
2. Configure ESP32 with server URL
3. Click buttons in web interface
4. Monitor ESP32 serial output for command execution
5. Verify relay states update in web UI after ACK

## 🐛 Troubleshooting

### ESP32 Issues

- **WiFi not connecting**: Check SSID/password, verify 2.4 GHz network
- **HTTP polling not working**: Verify URL is set, check server accessibility
- **Relays not responding**: Check GPIO connections, verify relay module power

See [ESP32 Firmware README - Troubleshooting](ESP32/firmware/README.md#troubleshooting) for detailed solutions.

### Server Issues

- **Port already in use**: Change port in `appsettings.json` or `launchSettings.json`
- **ESP32 can't reach server**: Ensure same network or configure port forwarding
- **Commands not executing**: Check ESP32 serial monitor for errors

## 📝 UART Commands

ESP32 accepts commands via UART (115200 baud):

**Relay Control:**

- `relay1 on` / `relay1 off`
- `relay2 on` / `relay2 off`
- `led on` / `led off`

**WiFi Configuration:**

- `SSID=<ssid>` - Set WiFi SSID
- `SSID?` - Query SSID
- `WIFIPASS=<password>` - Set password
- `WIFIPASS?` - Query password (masked)

**Server Configuration:**

- `URL=<url>` - Set server URL
- `URL?` - Query URL
- `IP?` - Query current IP address

## 🔐 Security Considerations

⚠️ **Important Security Notes:**

- The example server is for **development/demo purposes only**
- For production use:
  - Implement authentication/authorization
  - Use HTTPS (TLS) for all communications
  - Secure WiFi credentials storage
  - Add rate limiting to API endpoints
  - Validate and sanitize all inputs
  - Use secure password storage

## 📄 License

See [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Code Style

- **ESP32**: Follow ESP-IDF coding standards
- **C#**: Follow C# coding conventions
- Include appropriate error handling and logging
- Add comments for complex logic

## 🙏 Acknowledgments

- ESP-IDF framework by Espressif
- .NET and Blazor by Microsoft
- cJSON library for JSON parsing

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/hadideveloper/web-relay/issues)
- **Documentation**: See [ESP32 Firmware README](ESP32/firmware/README.md)

---

**Made with ❤️ for the IoT community**
