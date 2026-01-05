import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QMeshCore

Pane {
    id: root
    required property var device

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Not connected message
        Label {
            visible: !device.connected
            Layout.fillWidth: true
            text: "Connect to a MeshCore device to view RX log."
            wrapMode: Text.WordWrap
            opacity: 0.6
            horizontalAlignment: Text.AlignHCenter
            padding: 40
        }

        // Toolbar
        RowLayout {
            visible: device.connected
            Layout.fillWidth: true
            spacing: 8

            Switch {
                id: enableSwitch
                text: "Enable RX Logging"
                checked: device.rxLog.enabled
                onToggled: device.rxLog.enabled = checked
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Clear"
                icon.name: "edit-clear"
                enabled: device.rxLog.count > 0
                onClicked: device.rxLog.clear()
            }

            Label {
                text: device.rxLog.count + " entries"
                opacity: 0.7
            }
        }

        // Hint when logging is disabled
        Label {
            visible: device.connected && !device.rxLog.enabled
            Layout.fillWidth: true
            text: "Enable RX logging to see raw received packets.\nNote: This may generate a lot of data on busy networks."
            wrapMode: Text.WordWrap
            opacity: 0.6
            horizontalAlignment: Text.AlignHCenter
            padding: 20
        }

        // RX Log list
        ListView {
            id: rxLogListView
            visible: device.connected && device.rxLog.enabled
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: device.rxLog

            // Auto-scroll to bottom when new entries arrive
            onCountChanged: {
                if (autoScrollCheck.checked && count > 0) {
                    positionViewAtEnd()
                }
            }

            ScrollBar.vertical: ScrollBar { }

            delegate: ItemDelegate {
                width: ListView.view.width
                height: contentColumn.implicitHeight + 16
                padding: 8

                background: Rectangle {
                    color: index % 2 === 0 ? "transparent" : 
                           (Material.theme === Material.Dark ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(0, 0, 0, 0.03))
                }

                contentItem: ColumnLayout {
                    id: contentColumn
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        // Route type badge
                        Rectangle {
                            width: routeLabel.implicitWidth + 8
                            height: 18
                            radius: 3
                            color: {
                                switch (model.routeType) {
                                case 0: return Material.color(Material.Orange)  // T-FLOOD
                                case 1: return Material.color(Material.Blue)    // FLOOD
                                case 2: return Material.color(Material.Green)   // DIRECT
                                case 3: return Material.color(Material.Teal)    // T-DIRECT
                                default: return Material.color(Material.Grey)
                                }
                            }
                            Label {
                                id: routeLabel
                                anchors.centerIn: parent
                                text: model.routeTypeName
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }

                        // Payload type badge
                        Rectangle {
                            width: payloadLabel.implicitWidth + 8
                            height: 18
                            radius: 3
                            color: {
                                switch (model.payloadType) {
                                case 4: return Material.color(Material.Purple)  // ADVERT
                                case 2: return Material.color(Material.Indigo)  // TEXT
                                case 3: return Material.color(Material.Green)   // ACK
                                case 0: return Material.color(Material.Amber)   // REQ
                                case 1: return Material.color(Material.LightGreen) // RESP
                                case 5: // GRP_TXT
                                case 6: return Material.color(Material.Cyan)    // GRP_DATA
                                case 9: return Material.color(Material.Pink)    // TRACE
                                default: return Material.color(Material.Grey)
                                }
                            }
                            Label {
                                id: payloadLabel
                                anchors.centerIn: parent
                                text: model.payloadTypeName
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }

                        // Hop count
                        Label {
                            visible: model.hopCount > 0
                            text: model.hopCount + " hop" + (model.hopCount > 1 ? "s" : "")
                            font.pixelSize: 11
                            font.family: "monospace"
                            opacity: 0.8
                            color: model.hopCount <= 2 ? Material.foreground : 
                                   (model.hopCount <= 4 ? Material.color(Material.Orange) : Material.color(Material.Red))
                        }

                        Label {
                            text: model.timestampString
                            font.family: "monospace"
                            font.pixelSize: 11
                            opacity: 0.6
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: "SNR: " + model.snr.toFixed(1)
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: model.snr >= 0 ? Material.color(Material.Green) : 
                                   (model.snr >= -10 ? Material.color(Material.Orange) : Material.color(Material.Red))
                        }

                        Label {
                            text: "RSSI: " + model.rssi
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: model.rssi >= -70 ? Material.color(Material.Green) : 
                                   (model.rssi >= -90 ? Material.color(Material.Orange) : Material.color(Material.Red))
                        }

                        Label {
                            text: model.dataLength + "B"
                            font.family: "monospace"
                            font.pixelSize: 11
                            opacity: 0.6
                        }
                    }

                    // Second row: payload-specific info
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: model.destHash >= 0 || model.advertName !== "" || model.hasLocation

                        // Dest/Src hash for most packet types
                        Label {
                            visible: model.destHash >= 0
                            text: "→" + model.destHash.toString(16).toUpperCase().padStart(2, '0')
                            font.family: "monospace"
                            font.pixelSize: 11
                            opacity: 0.8
                            ToolTip.visible: hovered
                            ToolTip.text: "Destination hash"
                        }

                        Label {
                            visible: model.srcHash >= 0
                            text: "←" + model.srcHash.toString(16).toUpperCase().padStart(2, '0')
                            font.family: "monospace"
                            font.pixelSize: 11
                            opacity: 0.8
                            ToolTip.visible: hovered
                            ToolTip.text: "Source hash"
                        }

                        // Advert-specific info
                        Rectangle {
                            visible: model.payloadType === 4 && model.advertTypeName !== ""
                            width: advertTypeLabel.implicitWidth + 6
                            height: 16
                            radius: 2
                            color: {
                                switch (model.advertType) {
                                case 1: return Material.color(Material.Blue)    // Chat
                                case 2: return Material.color(Material.Green)   // Repeater
                                case 3: return Material.color(Material.Purple)  // Room
                                default: return Material.color(Material.Grey)
                                }
                            }
                            Label {
                                id: advertTypeLabel
                                anchors.centerIn: parent
                                text: model.advertTypeName
                                font.pixelSize: 9
                                color: "white"
                            }
                        }

                        Label {
                            visible: model.advertName !== ""
                            text: "\"" + model.advertName + "\""
                            font.pixelSize: 11
                            font.italic: true
                            elide: Text.ElideRight
                            Layout.maximumWidth: 200
                        }

                        Label {
                            visible: model.hasLocation
                            text: "📍 " + model.latitude.toFixed(4) + ", " + model.longitude.toFixed(4)
                            font.pixelSize: 10
                            opacity: 0.7
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Raw data hex display
                    Label {
                        Layout.fillWidth: true
                        text: model.rawDataHex
                        font.family: "monospace"
                        font.pixelSize: 10
                        wrapMode: Text.WrapAnywhere
                        opacity: 0.7
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }

            // Empty state
            Label {
                visible: device.rxLog.enabled && device.rxLog.count === 0
                anchors.centerIn: parent
                text: "Waiting for packets..."
                opacity: 0.6
            }
        }

        // Footer with auto-scroll option
        RowLayout {
            visible: device.connected && device.rxLog.enabled
            Layout.fillWidth: true

            CheckBox {
                id: autoScrollCheck
                text: "Auto-scroll"
                checked: true
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Max entries: " + device.rxLog.maxEntries
                font.pixelSize: 12
                opacity: 0.6
            }
        }
    }
}
