import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QMeshCore

Pane {
    id: root
    required property var device

    // Track if we're in edit mode
    property bool editMode: false

    // Edited values (bound to selfInfo when not editing)
    property string editName: device.selfInfo.name || ""
    property double editLatitude: device.selfInfo.latitude
    property double editLongitude: device.selfInfo.longitude
    property int editTxPower: device.selfInfo.txPower
    property double editFreqMHz: device.selfInfo.radioFreqMhz
    property double editBwKHz: device.selfInfo.radioBwKhz
    property int editSf: device.selfInfo.radioSf
    property int editCr: device.selfInfo.radioCr

    // Update edited values when selfInfo changes (and not in edit mode)
    Connections {
        target: device
        function onSelfInfoChanged() {
            if (!editMode) {
                editName = device.selfInfo.name || ""
                editLatitude = device.selfInfo.latitude
                editLongitude = device.selfInfo.longitude
                editTxPower = device.selfInfo.txPower
                editFreqMHz = device.selfInfo.radioFreqMhz
                editBwKHz = device.selfInfo.radioBwKhz
                editSf = device.selfInfo.radioSf
                editCr = device.selfInfo.radioCr
            }
        }
    }

    // Battery polling timer - poll every 10 seconds when connected
    Timer {
        id: batteryPollTimer
        interval: 10000
        repeat: true
        running: device.connected
        onTriggered: device.requestBatteryVoltage()
    }

    // Initial battery request when connected
    Connections {
        target: device
        function onConnectedChanged() {
            if (device.connected) {
                device.requestBatteryVoltage()
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            // Not connected message
            Label {
                visible: !device.connected
                Layout.fillWidth: true
                text: "Connect to a MeshCore device to view and configure settings."
                wrapMode: Text.WordWrap
                opacity: 0.6
                horizontalAlignment: Text.AlignHCenter
                padding: 40
            }

            // Device Identity - Editable
            GroupBox {
                visible: device.connected
                Layout.fillWidth: true
                title: "Device Identity"

                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    columnSpacing: 16
                    rowSpacing: 12

                    // Name
                    Label { text: "Name:"; font.bold: true; Layout.alignment: Qt.AlignTop }
                    TextField {
                        id: nameField
                        Layout.fillWidth: true
                        text: editName
                        placeholderText: "Device name"
                        enabled: editMode
                        onTextChanged: if (editMode) editName = text
                    }
                    Button {
                        text: "Set"
                        visible: editMode
                        enabled: nameField.text !== device.selfInfo.name
                        onClicked: {
                            device.setAdvertName(nameField.text)
                            refreshTimer.start()
                        }
                    }
                    Item { visible: !editMode; width: 1 }

                    // Advert Type (read-only)
                    Label { text: "Advert Type:"; font.bold: true }
                    Label {
                        Layout.columnSpan: 2
                        text: {
                            switch (device.selfInfo.type) {
                            case MeshCoreDevice.AdvertTypeChat: return "Chat"
                            case MeshCoreDevice.AdvertTypeRepeater: return "Repeater"
                            case MeshCoreDevice.AdvertTypeRoom: return "Room"
                            default: return "Unknown (" + device.selfInfo.type + ")"
                            }
                        }
                    }

                    // Public Key (read-only)
                    Label { text: "Public Key:"; font.bold: true }
                    Label {
                        Layout.columnSpan: 2
                        text: device.selfInfo.publicKeyHex ? device.selfInfo.publicKeyHex.substring(0, 16) + "..." : "N/A"
                        font.family: "monospace"
                    }
                }
            }

            // Location Settings
            GroupBox {
                visible: device.connected
                Layout.fillWidth: true
                title: "Location"

                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    columnSpacing: 16
                    rowSpacing: 12

                    Label { text: "Latitude:"; font.bold: true }
                    TextField {
                        id: latField
                        Layout.fillWidth: true
                        text: editLatitude.toFixed(6)
                        enabled: editMode
                        validator: DoubleValidator { bottom: -90; top: 90 }
                        onTextChanged: if (editMode && acceptableInput) editLatitude = parseFloat(text)
                    }
                    Item { width: 1 }

                    Label { text: "Longitude:"; font.bold: true }
                    TextField {
                        id: lonField
                        Layout.fillWidth: true
                        text: editLongitude.toFixed(6)
                        enabled: editMode
                        validator: DoubleValidator { bottom: -180; top: 180 }
                        onTextChanged: if (editMode && acceptableInput) editLongitude = parseFloat(text)
                    }
                    Button {
                        text: "Set Location"
                        visible: editMode
                        enabled: latField.acceptableInput && lonField.acceptableInput
                        onClicked: {
                            device.setAdvertLocation(parseFloat(latField.text), parseFloat(lonField.text))
                            refreshTimer.start()
                        }
                    }
                    Item { visible: !editMode; width: 1 }

                    Label { text: "Altitude:"; font.bold: true }
                    Label {
                        Layout.columnSpan: 2
                        text: device.selfInfo.altitude + " m"
                    }
                }
            }

            // Radio Settings
            GroupBox {
                visible: device.connected
                Layout.fillWidth: true
                title: "Radio Settings"

                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    columnSpacing: 16
                    rowSpacing: 12

                    // TX Power
                    Label { text: "TX Power:"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        SpinBox {
                            id: txPowerSpinBox
                            from: 0
                            to: device.selfInfo.maxTxPower > 0 ? device.selfInfo.maxTxPower : 22
                            value: editTxPower
                            enabled: editMode
                            onValueChanged: if (editMode) editTxPower = value
                        }
                        Label { text: "dBm" }
                        Label {
                            text: "(max: " + device.selfInfo.maxTxPower + ")"
                            opacity: 0.6
                            visible: device.selfInfo.maxTxPower > 0
                        }
                    }
                    Button {
                        text: "Set"
                        visible: editMode
                        enabled: txPowerSpinBox.value !== device.selfInfo.txPower
                        onClicked: {
                            device.setTxPower(txPowerSpinBox.value)
                            refreshTimer.start()
                        }
                    }
                    Item { visible: !editMode; width: 1 }

                    // Frequency
                    Label { text: "Frequency:"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        DoubleSpinBox {
                            id: freqSpinBox
                            from: 400
                            to: 1000
                            value: editFreqMHz > 0 ? editFreqMHz : device.selfInfo.radioFreqMhz
                            enabled: editMode
                            stepSize: 0.001
                            decimals: 3
                            onValueModified: if (editMode) editFreqMHz = value
                        }
                        Label { text: "MHz" }
                    }
                    Item { width: 1 }

                    // Bandwidth
                    Label { text: "Bandwidth:"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        DoubleSpinBox {
                            id: bwSpinBox
                            from: 7.8
                            to: 500
                            value: editBwKHz > 0 ? editBwKHz : device.selfInfo.radioBwKhz
                            enabled: editMode
                            stepSize: 0.1
                            decimals: 2
                            onValueModified: if (editMode) editBwKHz = value
                        }
                        Label { text: "kHz" }
                    }
                    Item { width: 1 }

                    // Spreading Factor
                    Label { text: "Spreading Factor:"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            id: sfCombo
                            enabled: editMode
                            model: [6, 7, 8, 9, 10, 11, 12]
                            currentIndex: model.indexOf(editSf) >= 0 ? model.indexOf(editSf) : 4
                            onCurrentIndexChanged: if (editMode) editSf = model[currentIndex]
                            displayText: "SF" + currentValue
                        }
                    }
                    Item { width: 1 }

                    // Coding Rate
                    Label { text: "Coding Rate:"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            id: crCombo
                            enabled: editMode
                            model: [5, 6, 7, 8]
                            currentIndex: model.indexOf(editCr) >= 0 ? model.indexOf(editCr) : 0
                            onCurrentIndexChanged: if (editMode) editCr = model[currentIndex]
                            displayText: "4/" + currentValue
                        }
                    }
                    Button {
                        text: "Set Radio"
                        visible: editMode
                        onClicked: {
                            // Frequency is in kHz, bandwidth is in Hz
                            let freqKHz = Math.round(editFreqMHz * 1000)
                            let bwHz = Math.round(editBwKHz * 1000)
                            device.setRadioParams(freqKHz, bwHz, editSf, editCr)
                            refreshTimer.start()
                        }
                    }
                    Item { visible: !editMode; width: 1 }
                }
            }

            // Firmware Information (read-only)
            GroupBox {
                visible: device.connected
                Layout.fillWidth: true
                title: "Firmware Information"

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 8

                    Label { text: "Device Model:"; font.bold: true }
                    Label { text: device.deviceInfo.model || "N/A" }

                    Label { text: "Vendor:"; font.bold: true }
                    Label { text: device.deviceInfo.vendor || "N/A" }

                    Label { text: "Firmware Version:"; font.bold: true }
                    Label { text: device.deviceInfo.firmwareVersion || "N/A" }
                }
            }

            // Battery Info
            GroupBox {
                visible: device.connected
                Layout.fillWidth: true
                title: "Battery"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true

                        Label { text: "Voltage:"; font.bold: true }
                        Label {
                            text: device.batteryMilliVolts > 0 ? device.batteryVolts.toFixed(3) + " V" : "Polling..."
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            visible: device.batteryMilliVolts > 0
                            text: {
                                // Estimate percentage: 3.2V = 0%, 4.2V = 100%
                                let pct = Math.max(0, Math.min(100, Math.round((device.batteryVolts - 3.2) / 1.0 * 100)))
                                return "~" + pct + "%"
                            }
                            font.bold: true
                        }
                    }

                    // Battery bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 20
                        color: Material.dividerColor
                        radius: 4

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: parent.width * Math.max(0, Math.min(1, (device.batteryVolts - 3.2) / 1.0))
                            radius: 4
                            color: {
                                let pct = (device.batteryVolts - 3.2) / 1.0
                                if (pct < 0.2) return Material.color(Material.Red)
                                if (pct < 0.5) return Material.color(Material.Orange)
                                return Material.color(Material.Green)
                            }
                        }
                    }

                    Label {
                        text: "Updates every 10 seconds"
                        font.pixelSize: 11
                        opacity: 0.5
                    }
                }
            }

            // Actions toolbar
            GroupBox {
                visible: device.connected
                Layout.fillWidth: true
                title: "Actions"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    // Edit mode toggle
                    RowLayout {
                        Layout.fillWidth: true

                        Switch {
                            id: editModeSwitch
                            text: "Edit Mode"
                            checked: editMode
                            onCheckedChanged: {
                                editMode = checked
                                if (!checked) {
                                    // Reset to current values when exiting edit mode
                                    editName = device.selfInfo.name || ""
                                    editLatitude = device.selfInfo.latitude
                                    editLongitude = device.selfInfo.longitude
                                    editTxPower = device.selfInfo.txPower
                                    editFreqMHz = device.selfInfo.radioFreqMhz
                                    editBwKHz = device.selfInfo.radioBwKhz
                                    editSf = device.selfInfo.radioSf
                                    editCr = device.selfInfo.radioCr
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            visible: editMode
                            text: "⚠ Changes are applied immediately"
                            color: Material.color(Material.Orange)
                            font.italic: true
                        }
                    }

                    // Action buttons
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Button {
                            text: "🔄 Refresh Info"
                            onClicked: {
                                device.requestSelfInfo()
                                device.requestBatteryVoltage()
                            }
                        }

                        Button {
                            text: "⏱ Sync Time"
                            onClicked: device.syncDeviceTime()
                        }

                        Button {
                            text: "📡 Send Advert"
                            onClicked: device.sendFloodAdvert()
                        }

                        Button {
                            text: "🔄 Reboot"
                            onClicked: rebootConfirmDialog.open()
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    // Timer to refresh info after setting a value
    Timer {
        id: refreshTimer
        interval: 500
        onTriggered: device.requestSelfInfo()
    }

    // Reboot confirmation dialog
    Dialog {
        id: rebootConfirmDialog
        title: "Confirm Reboot"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            text: "Are you sure you want to reboot the device?\nThe connection will be lost."
            wrapMode: Text.WordWrap
        }

        onAccepted: device.reboot()
    }
}
