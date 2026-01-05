import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.settings
import QMeshCore
import "components"

ApplicationWindow {
    id: root
    width: 1200
    height: 800
    visible: true
    title: "QMeshCore - " + (device.connected ? device.selfInfo.name || "Connected" : "Disconnected")

    // Theme settings
    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.accent: Material.Teal
    property bool darkMode: settings.darkMode

    // Persist settings
    Settings {
        id: settings
        property bool darkMode: false  // Default to light mode
    }

    // Update persisted setting when darkMode changes
    onDarkModeChanged: settings.darkMode = darkMode

    // MeshCore device instance (singleton)
    property var device: MeshCoreDevice

    // Handle connection errors and messages
    Connections {
        target: device
        function onConnectionError(error) {
            errorDialog.text = error
            errorDialog.open()
        }
        function onConnectedChanged() {
            if (device.connected) {
                // Auto-sync messages and channels when connected
                device.syncAllMessages()
                device.requestAllChannels()
            }
        }
        function onMsgWaiting() {
            // Auto-sync when new messages are waiting
            device.syncAllMessages()
        }
        function onContactMessageReceived(message) {
            messageNotification.text = "Message from " + message.senderPublicKeyPrefixHex + ": " + message.text
            messageNotification.open()
        }
        function onChannelMessageReceived(message) {
            messageNotification.text = "Channel message: " + message.text
            messageNotification.open()
        }
    }

    // Error dialog
    Dialog {
        id: errorDialog
        title: "Error"
        modal: true
        anchors.centerIn: parent
        property alias text: errorLabel.text

        Label {
            id: errorLabel
            wrapMode: Text.WordWrap
        }

        standardButtons: Dialog.Ok
    }

    // Message notification popup
    Popup {
        id: messageNotification
        property alias text: notificationLabel.text
        x: parent.width - width - 20
        y: 20
        width: 300
        padding: 12

        Timer {
            running: messageNotification.visible
            interval: 5000
            onTriggered: messageNotification.close()
        }

        background: Rectangle {
            color: Material.background
            border.color: Material.accent
            border.width: 2
            radius: 8
        }

        contentItem: Label {
            id: notificationLabel
            wrapMode: Text.WordWrap
        }
    }

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header with connection status
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Material.primary

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 16

                Label {
                    text: "QMeshCore"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }

                Item { Layout.fillWidth: true }

                // Connection indicator
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: {
                        switch (device.connectionState) {
                        case MeshCoreDevice.Connected: return "#4CAF50"
                        case MeshCoreDevice.Connecting: return "#FFC107"
                        case MeshCoreDevice.Error: return "#F44336"
                        default: return "#9E9E9E"
                        }
                    }
                }

                Label {
                    text: {
                        switch (device.connectionState) {
                        case MeshCoreDevice.Connected: return "Connected"
                        case MeshCoreDevice.Connecting: return "Connecting..."
                        case MeshCoreDevice.Error: return "Error"
                        default: return "Disconnected"
                        }
                    }
                    color: "white"
                }

                // Battery indicator (when connected)
                RowLayout {
                    visible: device.connected && device.batteryMilliVolts > 0
                    spacing: 4

                    Label {
                        text: "🔋"
                        font.pixelSize: 16
                    }
                    Label {
                        text: device.batteryVolts.toFixed(2) + "V"
                        color: "white"
                    }
                }

                // Dark mode toggle
                ToolButton {
                    text: root.darkMode ? "☀️" : "🌙"
                    font.pixelSize: 20
                    onClicked: root.darkMode = !root.darkMode
                    ToolTip.visible: hovered
                    ToolTip.text: root.darkMode ? "Switch to light mode" : "Switch to dark mode"
                }
            }
        }

        // Main content area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left sidebar - Connection panel
            ConnectionPanel {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                device: root.device
            }

            // Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Material.dividerColor
            }

            // Center area - tabs
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                TabBar {
                    id: tabBar
                    Layout.fillWidth: true

                    TabButton {
                        text: "Device Info"
                    }
                    TabButton {
                        text: "Contacts (" + device.contacts.count + ")"
                    }
                    TabButton {
                        text: "Messages (" + device.messages.count + ")"
                    }
                    TabButton {
                        text: "Channels (" + device.channels.count + ")"
                    }
                    TabButton {
                        text: "🗺️ Map"
                    }
                    TabButton {
                        text: "📡 RX Log" + (device.rxLog.enabled ? " (" + device.rxLog.count + ")" : "")
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: tabBar.currentIndex

                    DeviceInfoPanel {
                        device: root.device
                    }

                    ContactsPanel {
                        device: root.device
                    }

                    MessagesPanel {
                        device: root.device
                    }

                    ChannelsPanel {
                        device: root.device
                    }

                    MapPanel {
                        device: root.device
                    }

                    RxLogPanel {
                        device: root.device
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Material.background

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Label {
                    text: device.errorString || (device.connected ? "Ready" : "Not connected")
                    font.pixelSize: 12
                    opacity: 0.7
                    color: device.errorString ? Material.color(Material.Red) : Material.foreground
                }

                Item { Layout.fillWidth: true }

                Label {
                    visible: device.connected
                    text: device.connectionType === MeshCoreDevice.Ble ? "BLE" : "Serial"
                    font.pixelSize: 12
                    opacity: 0.7
                }
            }
        }
    }
}
