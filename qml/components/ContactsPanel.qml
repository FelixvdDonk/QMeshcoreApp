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
            text: "Connect to a MeshCore device to view contacts."
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
                text: "Refresh"
                icon.name: "view-refresh"
                onClicked: device.requestContacts()
            }

            Item { Layout.fillWidth: true }

            Label {
                text: device.contacts.count + " contacts"
                opacity: 0.7
            }
        }

        // Contact list
        ListView {
            id: contactListView
            visible: device.connected
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: device.contacts

            delegate: ItemDelegate {
                width: ListView.view.width
                height: 80
                highlighted: ListView.isCurrentItem

                onClicked: contactListView.currentIndex = index
                onDoubleClicked: {
                    sendMessageDialog.targetPublicKeyPrefix = model.publicKeyPrefix
                    sendMessageDialog.targetName = model.name
                    sendMessageDialog.open()
                }

                contentItem: ColumnLayout {
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        // Contact type indicator
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: {
                                switch (model.type) {
                                case MeshCoreDevice.AdvertTypeChat: return Material.color(Material.Blue)
                                case MeshCoreDevice.AdvertTypeRepeater: return Material.color(Material.Green)
                                case MeshCoreDevice.AdvertTypeRoom: return Material.color(Material.Purple)
                                default: return Material.color(Material.Grey)
                                }
                            }
                        }

                        Label {
                            text: model.name || "Unknown"
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: model.lastAdvert > 0 ? formatTimeAgo(model.lastAdvert) : ""
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: model.publicKeyHex.substring(0, 24) + "..."
                            font.family: "monospace"
                            font.pixelSize: 11
                            opacity: 0.7
                        }

                        Item { Layout.fillWidth: true }

                        // Location if available
                        Label {
                            visible: model.latitudeDecimal !== 0 || model.longitudeDecimal !== 0
                            text: "📍"
                            font.pixelSize: 14
                            opacity: 0.7
                        }
                    }

                    // Type and path info
                    Label {
                        visible: model.outPathLen >= 0
                        text: model.typeString + (model.outPathLen > 0 ? " • " + model.outPathLen + " hops" : "")
                        font.pixelSize: 11
                        opacity: 0.5
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}

            // Empty state
            Label {
                anchors.centerIn: parent
                text: "No contacts yet"
                opacity: 0.5
                visible: contactListView.count === 0
            }
        }

        // Contact actions
        RowLayout {
            visible: device.connected && contactListView.currentIndex >= 0
            Layout.fillWidth: true

            Button {
                text: "Send Message"
                onClicked: {
                    let contact = device.contacts.get(contactListView.currentIndex)
                    if (contact) {
                        sendMessageDialog.targetPublicKeyPrefix = contact.publicKeyPrefix
                        sendMessageDialog.targetName = contact.name
                        sendMessageDialog.open()
                    }
                }
            }

            Button {
                text: "Trace Path"
                onClicked: {
                    let contact = device.contacts.get(contactListView.currentIndex)
                    if (contact) {
                        device.sendTracePath(contact.publicKeyPrefix)
                    }
                }
            }

            Button {
                text: "Request Telemetry"
                onClicked: {
                    let contact = device.contacts.get(contactListView.currentIndex)
                    if (contact) {
                        device.requestTelemetry(contact.publicKeyPrefix)
                    }
                }
            }
        }
    }

    // Send message dialog
    Dialog {
        id: sendMessageDialog
        title: "Send Message to " + (targetName || "Contact")
        modal: true
        anchors.centerIn: parent
        width: 400

        property var targetPublicKeyPrefix: null
        property string targetName: ""

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: "Message:"
            }

            TextArea {
                id: messageInput
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                placeholderText: "Type your message..."
                wrapMode: TextEdit.Wrap
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            if (messageInput.text.trim().length > 0 && targetPublicKeyPrefix) {
                device.sendContactMessage(targetPublicKeyPrefix, messageInput.text.trim())
                messageInput.text = ""
            }
        }

        onRejected: {
            messageInput.text = ""
        }
    }

    // Helper function to format time ago
    function formatTimeAgo(timestamp) {
        let now = Date.now() / 1000
        let diff = now - timestamp
        
        if (diff < 60) return Math.floor(diff) + "s ago"
        if (diff < 3600) return Math.floor(diff / 60) + "m ago"
        if (diff < 86400) return Math.floor(diff / 3600) + "h ago"
        return Math.floor(diff / 86400) + "d ago"
    }
}
