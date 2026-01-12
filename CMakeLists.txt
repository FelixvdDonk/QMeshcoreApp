cmake_minimum_required(VERSION 3.21)

project(QMeshCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    Bluetooth
    SerialPort
    Concurrent
    Positioning
    Location
)

qt_standard_project_setup(REQUIRES 6.10)

# Main application executable (named differently from QML module to avoid conflict)
qt_add_executable(QMeshCoreApp
    src/main.cpp
)

# Add QML module with all sources directly on the executable
# This avoids duplicate type registration issues with static libraries
qt_add_qml_module(QMeshCoreApp
    URI QMeshCore
    VERSION 1.0
    # Disable auto type registration - we'll do it manually
    NO_GENERATE_QMLTYPES
    NO_PLUGIN_OPTIONAL
    QML_FILES
        qml/Main.qml
        qml/components/ConnectionPanel.qml
        qml/components/DeviceInfoPanel.qml
        qml/components/ContactsPanel.qml
        qml/components/MessagesPanel.qml
        qml/components/ChannelsPanel.qml
        qml/components/MapPanel.qml
        qml/components/RxLogPanel.qml
    SOURCES
        # Plugin registration
        src/meshcore/qmeshcore_plugin.h
        src/meshcore/qmeshcore_plugin.cpp

        # Core
        src/meshcore/MeshCoreDevice.cpp
        src/meshcore/MeshCoreDevice.h
        src/meshcore/MeshCoreConstants.cpp
        src/meshcore/MeshCoreConstants.h

        # Data types
        src/meshcore/types/Contact.cpp
        src/meshcore/types/Contact.h
        src/meshcore/types/SelfInfo.cpp
        src/meshcore/types/SelfInfo.h
        src/meshcore/types/DeviceInfo.cpp
        src/meshcore/types/DeviceInfo.h
        src/meshcore/types/ChannelInfo.cpp
        src/meshcore/types/ChannelInfo.h
        src/meshcore/types/ContactMessage.cpp
        src/meshcore/types/ContactMessage.h
        src/meshcore/types/ChannelMessage.cpp
        src/meshcore/types/ChannelMessage.h
        src/meshcore/types/RepeaterStats.cpp
        src/meshcore/types/RepeaterStats.h
        src/meshcore/types/Advert.cpp
        src/meshcore/types/Advert.h
        src/meshcore/types/Packet.cpp
        src/meshcore/types/Packet.h
        src/meshcore/types/TraceData.cpp
        src/meshcore/types/TraceData.h
        src/meshcore/types/TelemetryData.cpp
        src/meshcore/types/TelemetryData.h
        src/meshcore/types/RxLogEntry.cpp
        src/meshcore/types/RxLogEntry.h

        # Models
        src/meshcore/models/ContactModel.cpp
        src/meshcore/models/ContactModel.h
        src/meshcore/models/ChannelModel.cpp
        src/meshcore/models/ChannelModel.h
        src/meshcore/models/MessageModel.cpp
        src/meshcore/models/MessageModel.h
        src/meshcore/models/RxLogModel.cpp
        src/meshcore/models/RxLogModel.h

        # Connections
        src/meshcore/connection/MeshCoreConnection.cpp
        src/meshcore/connection/MeshCoreConnection.h
        src/meshcore/connection/BleConnection.cpp
        src/meshcore/connection/BleConnection.h
        src/meshcore/connection/NusBleConnection.cpp
        src/meshcore/connection/NusBleConnection.h
        src/meshcore/connection/SerialConnection.cpp
        src/meshcore/connection/SerialConnection.h
        # Linux-only D-Bus based connections
        $<$<PLATFORM_ID:Linux>:src/meshcore/connection/DBusBleConnection.cpp>
        $<$<PLATFORM_ID:Linux>:src/meshcore/connection/DBusBleConnection.h>
        $<$<PLATFORM_ID:Linux>:src/meshcore/connection/BluezAgent.cpp>
        $<$<PLATFORM_ID:Linux>:src/meshcore/connection/BluezAgent.h>
        # Windows-only WinRT BLE pairing
        $<$<PLATFORM_ID:Windows>:src/meshcore/connection/WinRtBlePairing.cpp>
        $<$<PLATFORM_ID:Windows>:src/meshcore/connection/WinRtBlePairing.h>

        # Utils
        src/meshcore/utils/BufferReader.cpp
        src/meshcore/utils/BufferReader.h
        src/meshcore/utils/BufferWriter.cpp
        src/meshcore/utils/BufferWriter.h
        src/meshcore/utils/CayenneLpp.cpp
        src/meshcore/utils/CayenneLpp.h
)

target_include_directories(QMeshCoreApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QMeshCoreApp PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Bluetooth
    Qt6::SerialPort
    Qt6::Concurrent
    Qt6::Positioning
    Qt6::Location
)

# Windows-specific: Link WinRT libraries for BLE pairing
if(WIN32)
    target_link_libraries(QMeshCoreApp PRIVATE
        windowsapp
    )
    target_compile_options(QMeshCoreApp PRIVATE /await)
endif()

# Install rules
install(TARGETS QMeshCoreApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
