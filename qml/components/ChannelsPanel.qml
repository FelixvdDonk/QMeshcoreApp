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

        // Not connected message
        Label {
            visible: !device.connected
            Layout.fillWidth: true
            text: "Connect to a MeshCore device to view and manage channels."
            wrapMode: Text.WordWrap
            opacity: 0.6
            horizontalAlignment: Text.AlignHCenter
            padding: 40
        }

        // Channel list
        ListView {
            id: channelListView
            visible: device.connected
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: device.channels

            delegate: ItemDelegate {
                id: channelDelegate
                required property int index
                required property int channelIndex
                required property string name
                required property string secretHex
                required property bool isEmpty

                width: ListView.view.width
                height: 80
                highlighted: ListView.isCurrentItem

                onClicked: channelListView.currentIndex = index

                contentItem: ColumnLayout {
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Channel " + channelDelegate.channelIndex
                            font.bold: true
                        }

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: !channelDelegate.isEmpty ? Material.color(Material.Green) : Material.color(Material.Grey)
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Label {
                        text: channelDelegate.name || "(No name)"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        opacity: channelDelegate.name ? 1.0 : 0.5
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        Label {
                            text: "PSK: " + (channelDelegate.secretHex ? channelDelegate.secretHex.substring(0, 16) + "..." : "None")
                            font.family: "monospace"
                            font.pixelSize: 11
                            opacity: 0.6
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}

            // Empty state
            Label {
                anchors.centerIn: parent
                text: "No channels configured.\nClick 'Refresh Channels' to load."
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.5
                visible: channelListView.count === 0
            }
        }

        // Toolbar
        RowLayout {
            visible: device.connected
            Layout.fillWidth: true

            Button {
                text: "Refresh Channels"
                onClicked: device.requestAllChannels()
            }

            Item { Layout.fillWidth: true }

            Label {
                text: device.channels.count + " channels"
                opacity: 0.7
            }
        }

        // Channel actions
        RowLayout {
            visible: device.connected && channelListView.currentIndex >= 0
            Layout.fillWidth: true

            Button {
                text: "Edit Channel"
                onClicked: {
                    let channel = device.channels.get(channelListView.currentIndex)
                    if (channel) {
                        editChannelDialog.channelIndex = channel.index
                        editChannelDialog.channelName = channel.name || ""
                        editChannelDialog.channelPsk = channel.secretHex || ""
                        editChannelDialog.open()
                    }
                }
            }

            Button {
                text: "Send Message"
                onClicked: {
                    let channel = device.channels.get(channelListView.currentIndex)
                    if (channel) {
                        sendChannelMessageDialog.channelIndex = channel.index
                        sendChannelMessageDialog.channelName = channel.name || ""
                        sendChannelMessageDialog.open()
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        // Channel info
        GroupBox {
            visible: device.connected
            Layout.fillWidth: true
            title: "Channel Information"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label {
                    text: "MeshCore supports up to 8 channels (0-7)."
                    wrapMode: Text.WordWrap
                    opacity: 0.7
                }

                Label {
                    text: "Channel 0 is typically the primary/public channel."
                    wrapMode: Text.WordWrap
                    opacity: 0.7
                }

                Label {
                    text: "Each channel can have a unique name and PSK for encryption."
                    wrapMode: Text.WordWrap
                    opacity: 0.7
                }
            }
        }
    }

    // Edit channel dialog
    Dialog {
        id: editChannelDialog
        title: "Edit Channel " + channelIndex
        modal: true
        anchors.centerIn: parent
        width: 400

        property int channelIndex: -1
        property string channelName: ""
        property string channelPsk: ""
        property bool channelEnabled: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label { text: "Channel Name:" }

            TextField {
                id: nameInput
                Layout.fillWidth: true
                text: editChannelDialog.channelName
                placeholderText: "Enter channel name..."
            }

            Label { text: "PSK (hex):" }

            TextField {
                id: pskInput
                Layout.fillWidth: true
                text: editChannelDialog.channelPsk
                placeholderText: "Enter PSK in hex format..."
                font.family: "monospace"
            }

            CheckBox {
                id: enabledCheckbox
                text: "Channel Enabled"
                checked: editChannelDialog.channelEnabled
            }

            Label {
                text: "Note: Changes will be applied immediately to the device."
                font.pixelSize: 11
                opacity: 0.6
                wrapMode: Text.WordWrap
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            // Convert PSK hex string to QByteArray
            let pskBytes = hexToBytes(pskInput.text.trim())
            device.setChannel(channelIndex, nameInput.text.trim(), pskBytes)
        }
    }

    // Send channel message dialog
    Dialog {
        id: sendChannelMessageDialog
        title: "Send to Channel " + channelIndex + (channelName ? " (" + channelName + ")" : "")
        modal: true
        anchors.centerIn: parent
        width: 400

        property int channelIndex: -1
        property string channelName: ""

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label { text: "Message:" }

            TextArea {
                id: channelMessageInput
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                placeholderText: "Type your channel message..."
                wrapMode: TextEdit.Wrap
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            if (channelMessageInput.text.trim().length > 0) {
                device.sendChannelMessage(channelIndex, channelMessageInput.text.trim())
                channelMessageInput.text = ""
            }
        }

        onRejected: {
            channelMessageInput.text = ""
        }
    }

    // Helper function to convert hex string to byte array
    function hexToBytes(hex) {
        let bytes = []
        hex = hex.replace(/\s/g, "") // Remove whitespace
        for (let i = 0; i < hex.length; i += 2) {
            bytes.push(parseInt(hex.substr(i, 2), 16))
        }
        return bytes
    }
}
