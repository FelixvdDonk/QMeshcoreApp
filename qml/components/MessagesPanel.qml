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
            text: "Connect to a MeshCore device to view messages."
            wrapMode: Text.WordWrap
            opacity: 0.6
            horizontalAlignment: Text.AlignHCenter
            padding: 40
        }

        // Toolbar
        RowLayout {
            visible: device.connected
            Layout.fillWidth: true

            Button {
                text: "Sync Messages"
                icon.name: "view-refresh"
                onClicked: device.syncAllMessages()
            }

            Item { Layout.fillWidth: true }

            Label {
                text: device.messages.count + " messages"
                opacity: 0.7
            }
        }

        // Message list
        ListView {
            id: messageListView
            visible: device.connected
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: device.messages
            verticalLayoutDirection: ListView.BottomToTop

            delegate: Rectangle {
                id: messageDelegate
                required property int index
                required property int messageType
                required property string sender
                required property string text
                required property int timestamp
                required property int channelIndex
                required property int pathLen

                // Helper computed properties
                property bool isChannelMessage: messageType === 1  // ChannelMessageType
                property bool isContactMessage: messageType === 0  // ContactMessageType

                width: ListView.view.width - 20
                height: messageContent.implicitHeight + 20
                x: 10
                color: isChannelMessage ? Material.color(Material.Green, Material.Shade100) :
                       Material.color(Material.Grey, Material.Shade200)
                radius: 8

                ColumnLayout {
                    id: messageContent
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        // Message type indicator
                        Label {
                            text: messageDelegate.isChannelMessage ? "📢" : "📥"
                            font.pixelSize: 14
                        }

                        Label {
                            text: {
                                if (messageDelegate.isChannelMessage) {
                                    return "Channel " + messageDelegate.channelIndex
                                } else {
                                    return "From: " + messageDelegate.sender.substring(0, 12)
                                }
                            }
                            font.bold: true
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: formatTimestamp(messageDelegate.timestamp)
                            font.pixelSize: 11
                            opacity: 0.6
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: messageDelegate.text
                        wrapMode: Text.WordWrap
                    }

                    // Path length info
                    Label {
                        visible: messageDelegate.pathLen > 0 && messageDelegate.pathLen < 255
                        text: messageDelegate.pathLen + " hop" + (messageDelegate.pathLen > 1 ? "s" : "")
                        font.pixelSize: 11
                        opacity: 0.5
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}

            // Empty state
            Label {
                anchors.centerIn: parent
                text: "No messages yet.\nMessages will appear here when received."
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.5
                visible: messageListView.count === 0
            }
        }

        // Message input area
        GroupBox {
            visible: device.connected
            Layout.fillWidth: true
            title: sendTargetCombo.currentIndex === 0 ? "Send Direct Message" : "Send Channel Message"

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Label { text: "Send to:" }

                    ComboBox {
                        id: sendTargetCombo
                        Layout.fillWidth: true
                        model: buildSendTargets()

                        function buildSendTargets() {
                            let targets = ["Select contact..."]
                            
                            // Add contacts
                            for (let i = 0; i < device.contacts.count; i++) {
                                let contact = device.contacts.get(i)
                                if (contact) {
                                    targets.push("📱 " + (contact.name || "Contact " + i))
                                }
                            }
                            
                            // Add channels
                            for (let j = 0; j < device.channels.count; j++) {
                                let channel = device.channels.get(j)
                                if (channel && channel.name) {
                                    targets.push("📢 Ch" + j + ": " + channel.name)
                                }
                            }
                            
                            return targets
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: messageTextField
                        Layout.fillWidth: true
                        placeholderText: "Type a message..."
                        enabled: sendTargetCombo.currentIndex > 0

                        Keys.onReturnPressed: sendButton.clicked()
                    }

                    Button {
                        id: sendButton
                        text: "Send"
                        enabled: messageTextField.text.trim().length > 0 && sendTargetCombo.currentIndex > 0

                        onClicked: {
                            let idx = sendTargetCombo.currentIndex - 1
                            let text = messageTextField.text.trim()
                            
                            if (idx < device.contacts.count) {
                                // Send to contact - use publicKey (first 6 bytes for prefix)
                                let contact = device.contacts.get(idx)
                                if (contact) {
                                    device.sendContactMessage(contact.publicKey, text)
                                }
                            } else {
                                // Send to channel
                                let channelIdx = idx - device.contacts.count
                                device.sendChannelMessage(channelIdx, text)
                            }
                            
                            messageTextField.text = ""
                        }
                    }
                }
            }
        }
    }

    // Helper function to format timestamp
    function formatTimestamp(timestamp) {
        if (timestamp === 0) return ""
        
        let date = new Date(timestamp * 1000)
        let now = new Date()
        
        // If same day, show time only
        if (date.toDateString() === now.toDateString()) {
            return date.toLocaleTimeString(Qt.locale(), "HH:mm")
        }
        
        // Otherwise show date and time
        return date.toLocaleDateString(Qt.locale(), "MMM d") + " " +
               date.toLocaleTimeString(Qt.locale(), "HH:mm")
    }

    // Refresh targets when contacts or channels change
    Connections {
        target: device.contacts
        function onCountChanged() {
            sendTargetCombo.model = sendTargetCombo.buildSendTargets()
        }
    }

    Connections {
        target: device.channels
        function onCountChanged() {
            sendTargetCombo.model = sendTargetCombo.buildSendTargets()
        }
    }
}
