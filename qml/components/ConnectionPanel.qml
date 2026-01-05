import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QMeshCore

Pane {
    id: root
    required property var device

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Connection type selector
        Label {
            text: "Connection Type"
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true

            RadioButton {
                id: bleRadio
                text: "BLE"
                checked: true
                enabled: !device.connected
            }

            RadioButton {
                id: serialRadio
                text: "Serial"
                enabled: !device.connected
            }
        }

        // BLE Section
        GroupBox {
            visible: bleRadio.checked
            Layout.fillWidth: true
            title: "Bluetooth LE"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: device.scanning ? "Stop Scan" : "Scan"
                        enabled: !device.connected
                        onClicked: {
                            if (device.scanning) {
                                device.stopBleScan()
                            } else {
                                device.startBleScan()
                            }
                        }

                        BusyIndicator {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20
                            height: 20
                            running: device.scanning
                            visible: device.scanning
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Refresh"
                        enabled: !device.connected && !device.scanning
                        onClicked: {
                            device.clearBleDevices()
                            device.startBleScan()
                        }
                    }
                }

                // BLE device list
                ListView {
                    id: bleListView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    clip: true

                    model: device.discoveredBleDevices

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: modelData.name + " (" + modelData.address + ")"
                        highlighted: ListView.isCurrentItem
                        enabled: !device.connected

                        onClicked: bleListView.currentIndex = index
                        onDoubleClicked: {
                            device.connectBle(index)
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: Material.dividerColor
                        border.width: 1
                        radius: 4
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        text: device.scanning ? "Scanning..." : "No devices found"
                        opacity: 0.5
                        visible: bleListView.count === 0
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Connect"
                    enabled: bleListView.currentIndex >= 0 && !device.connected
                    onClicked: {
                        device.connectBle(bleListView.currentIndex)
                    }
                }
            }
        }

        // Serial Section
        GroupBox {
            visible: serialRadio.checked
            Layout.fillWidth: true
            title: "Serial Port"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Button {
                        text: "Refresh Ports"
                        enabled: !device.connected
                        onClicked: device.refreshSerialPorts()
                    }
                }

                // Serial port list
                ListView {
                    id: serialListView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    clip: true

                    model: device.availableSerialPorts

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: modelData.name + (modelData.description ? " - " + modelData.description : "")
                        highlighted: ListView.isCurrentItem
                        enabled: !device.connected

                        onClicked: serialListView.currentIndex = index
                        onDoubleClicked: {
                            device.connectSerialByIndex(index)
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: Material.dividerColor
                        border.width: 1
                        radius: 4
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        text: "No ports found"
                        opacity: 0.5
                        visible: serialListView.count === 0
                    }
                }

                // Baud rate selector
                RowLayout {
                    Layout.fillWidth: true

                    Label { text: "Baud Rate:" }

                    ComboBox {
                        id: baudRateCombo
                        Layout.fillWidth: true
                        model: ["115200", "57600", "38400", "19200", "9600"]
                        currentIndex: 0
                        enabled: !device.connected
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Connect"
                    enabled: serialListView.currentIndex >= 0 && !device.connected
                    onClicked: {
                        device.connectSerialByIndex(
                            serialListView.currentIndex,
                            parseInt(baudRateCombo.currentText)
                        )
                    }
                }
            }
        }

        // Disconnect button
        Button {
            Layout.fillWidth: true
            text: "Disconnect"
            visible: device.connected
            onClicked: device.disconnect()
            Material.background: Material.color(Material.Red, Material.Shade700)
            Material.foreground: "white"
        }

        // Connection info
        GroupBox {
            visible: device.connected
            Layout.fillWidth: true
            title: "Connection Info"

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                Label {
                    text: "Type: " + (device.connectionType === MeshCoreDevice.Ble ? "BLE" : "Serial")
                    font.pixelSize: 12
                }

                Label {
                    text: "State: Connected"
                    font.pixelSize: 12
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
