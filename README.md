# QMeshCoreApp

A Qt6/QML desktop companion application for [MeshCore](https://github.com/ripplebiz/MeshCore) devices. Connect to your MeshCore radio via Bluetooth Low Energy (BLE) or USB Serial to manage contacts, send messages, view the mesh network on a map, and monitor radio traffic.

![Qt](https://img.shields.io/badge/Qt-6.10+-41CD52?logo=qt&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-blue)
![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey)

## Features

- **BLE & Serial Connection** - Connect to MeshCore devices via Bluetooth LE or USB serial
- **Contact Management** - View, add, and manage mesh network contacts
- **Messaging** - Send and receive direct messages and channel messages
- **Map View** - Visualize mesh nodes on an interactive map with location data
- **Device Configuration** - Edit device name, location, TX power, and radio parameters
- **RX Log** - Monitor raw radio traffic with packet decoding (route type, payload type, hop count, etc.)
- **Battery Monitoring** - Real-time battery voltage display with polling
- **Dark Mode** - Toggle between light and dark themes

## Screenshots

*Coming soon*

## Requirements

- Qt 6.10 or later
- Qt modules: Core, Quick, QuickControls2, Bluetooth, SerialPort, Positioning, Location
- CMake 3.21+
- C++20 compiler

### Linux Dependencies

On Debian/Ubuntu:
```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-connectivity-dev \
    qt6-serialport-dev qt6-positioning-dev libqt6location6-plugins \
    cmake build-essential
```

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Or open the project in Qt Creator and build from there.

## Usage

1. Launch the application
2. **BLE Connection**: Click "Scan for BLE Devices", select your MeshCore device, and click "Connect"
3. **Serial Connection**: Select the serial port and baud rate (default 115200), then click "Connect"
4. Once connected, navigate through the tabs to:
   - **Device Info**: View and edit device settings
   - **Contacts**: See mesh network contacts and their status
   - **Messages**: View message history and send new messages
   - **Channels**: Manage channel subscriptions
   - **Map**: View node locations on a map
   - **RX Log**: Enable packet logging to see raw radio traffic

## Project Structure

```
QMeshCoreApp/
├── CMakeLists.txt          # Build configuration
├── qml/
│   ├── Main.qml            # Main application window
│   └── components/         # QML UI components
│       ├── ConnectionPanel.qml
│       ├── DeviceInfoPanel.qml
│       ├── ContactsPanel.qml
│       ├── MessagesPanel.qml
│       ├── ChannelsPanel.qml
│       ├── MapPanel.qml
│       └── RxLogPanel.qml
└── src/
    ├── main.cpp
    └── meshcore/           # MeshCore protocol implementation
        ├── MeshCoreDevice.h/cpp      # High-level device API
        ├── connection/               # BLE and Serial connection handlers
        ├── types/                    # Data types (Contact, Message, etc.)
        └── models/                   # Qt models for QML ListView binding
```

## MeshCore Protocol

This application implements the MeshCore companion protocol for communicating with MeshCore firmware devices. Key features:

- Framed binary protocol with CRC16 checksums
- Support for all MeshCore commands (get contacts, send messages, etc.)
- Push notification handling for incoming messages and adverts
- RX log packet parsing with route type, payload type, and hop count extraction

## Related Projects

- [MeshCore](https://github.com/ripplebiz/MeshCore) - The MeshCore firmware
- [meshcore.js](https://github.com/ripplebiz/meshcore.js) - JavaScript implementation
- [meshcore-cli](https://github.com/ripplebiz/meshcore-cli) - Command-line interface

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.
