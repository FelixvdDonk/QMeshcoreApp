import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QMeshCore

Pane {
    id: root
    required property var device

    // Map center - can be user-controlled or auto-centered on nodes
    property var mapCenter: QtPositioning.coordinate(52.0, 5.5) // Default: Netherlands
    property real mapZoom: 8.0

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // Not connected message
        Label {
            visible: !device.connected
            Layout.fillWidth: true
            text: "Connect to a MeshCore device to view the mesh network map."
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

            Button {
                text: "Refresh"
                icon.name: "view-refresh"
                onClicked: device.requestContacts()
            }

            Button {
                text: "Fit All"
                onClicked: fitAllNodes()
            }

            Item { Layout.fillWidth: true }

            // Map type selector
            Label { text: "Map:" }
            ComboBox {
                id: mapTypeCombo
                model: map.supportedMapTypes
                textRole: "name"
                currentIndex: 0
                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex < map.supportedMapTypes.length) {
                        map.activeMapType = map.supportedMapTypes[currentIndex]
                    }
                }
            }

            Item { width: 16 }

            // Legend
            RowLayout {
                spacing: 16

                RowLayout {
                    spacing: 4
                    Rectangle { width: 12; height: 12; radius: 6; color: "#2196F3" }
                    Label { text: "Chat"; font.pixelSize: 12 }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 12; height: 12; radius: 6; color: "#4CAF50" }
                    Label { text: "Repeater"; font.pixelSize: 12 }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 12; height: 12; radius: 6; color: "#9C27B0" }
                    Label { text: "Room"; font.pixelSize: 12 }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 12; height: 12; radius: 6; color: "#FF9800" }
                    Label { text: "Self"; font.pixelSize: 12 }
                }
            }

            Label {
                text: countNodesWithLocation() + " nodes on map"
                opacity: 0.7
            }
        }

        // Map
        Map {
            id: map
            visible: device.connected
            Layout.fillWidth: true
            Layout.fillHeight: true

            center: mapCenter
            zoomLevel: mapZoom
            property geoCoordinate startCentroid
            property geoCoordinate dragStartCoord
            property geoCoordinate wheelZoomCoord
            property point wheelZoomPoint
            property real targetZoom: mapZoom

            // Smooth zoom animation that keeps cursor position stable
            NumberAnimation {
                id: zoomAnim
                target: map
                property: "zoomLevel"
                duration: 150
                easing.type: Easing.OutQuad
                onRunningChanged: {
                    if (running) {
                        // Continuously align during animation
                    }
                }
            }

            onZoomLevelChanged: {
                if (zoomAnim.running && wheelZoomCoord.isValid) {
                    map.alignCoordinateToPoint(wheelZoomCoord, wheelZoomPoint)
                }
            }

            PinchHandler {
                id: pinch
                target: null
                onActiveChanged: if (active) {
                    map.startCentroid = map.toCoordinate(pinch.centroid.position, false)
                }
                onScaleChanged: (delta) => {
                    map.zoomLevel += Math.log2(delta)
                    map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
                }
                onRotationChanged: (delta) => {
                    map.bearing -= delta
                    map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
                }
                grabPermissions: PointerHandler.TakeOverForbidden
            }

            WheelHandler {
                id: wheel
                acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
                                 ? PointerDevice.Mouse | PointerDevice.TouchPad
                                 : PointerDevice.Mouse
                onWheel: (event) => {
                    map.wheelZoomCoord = map.toCoordinate(point.position, false)
                    map.wheelZoomPoint = point.position
                    let delta = event.angleDelta.y > 0 ? 1 : -1
                    map.targetZoom = Math.max(map.minimumZoomLevel, Math.min(map.maximumZoomLevel, map.zoomLevel + delta))
                    zoomAnim.to = map.targetZoom
                    zoomAnim.restart()
                }
            }

            DragHandler {
                id: drag
                target: null
                onActiveChanged: {
                    if (active) {
                        map.dragStartCoord = map.toCoordinate(centroid.position, false)
                    }
                }
                onCentroidChanged: {
                    if (active) {
                        map.alignCoordinateToPoint(map.dragStartCoord, centroid.position)
                    }
                }
            }

            plugin: Plugin {
                id: mapPlugin
                name: "osm"
                // Use high-DPI tiles when available
                PluginParameter {
                    name: "osm.mapping.highdpi_tiles"
                    value: "true"
                }
            }

            // Select terrain map type when available (closest to satellite in OSM)
            Component.onCompleted: {
                console.log("Available map types:", map.supportedMapTypes.length)
                for (var i = 0; i < map.supportedMapTypes.length; i++) {
                    console.log("  Map type", i, ":", map.supportedMapTypes[i].name)
                }
                // Default to first type
                if (map.supportedMapTypes.length > 0) {
                    mapTypeCombo.currentIndex = 0
                }
            }

            // Self position marker
            MapQuickItem {
                id: selfMarker
                visible: device.selfInfo.latitudeDecimal !== 0 || device.selfInfo.longitudeDecimal !== 0
                coordinate: QtPositioning.coordinate(
                    device.selfInfo.latitudeDecimal,
                    device.selfInfo.longitudeDecimal
                )
                anchorPoint.x: 0
                anchorPoint.y: 0

                sourceItem: Item {
                    // Dot centered at (0,0) - the coordinate point
                    Rectangle {
                        id: selfDot
                        x: -10  // Center the 20px dot on (0,0)
                        y: -10
                        width: 20
                        height: 20
                        radius: 10
                        color: "#FF9800"
                        border.color: "white"
                        border.width: 2

                        Label {
                            anchors.centerIn: parent
                            text: "★"
                            font.pixelSize: 12
                            color: "white"
                        }
                    }

                    // Label below the dot
                    Rectangle {
                        x: -width / 2  // Center horizontally on (0,0)
                        y: 12  // Below the dot (10 + 2px spacing)
                        color: "#FF9800"
                        radius: 4
                        width: selfNameLabel.implicitWidth + 8
                        height: selfNameLabel.implicitHeight + 4

                        Label {
                            id: selfNameLabel
                            anchors.centerIn: parent
                            text: device.selfInfo.name || "Self"
                            font.pixelSize: 11
                            font.bold: true
                            color: "white"
                        }
                    }
                }
            }

            // Contact markers
            MapItemView {
                model: device.contacts
                delegate: MapQuickItem {
                    id: contactMarker
                    required property int index
                    required property string name
                    required property int type
                    required property double latitudeDecimal
                    required property double longitudeDecimal
                    required property int outPathLen
                    required property string publicKeyHex

                    visible: latitudeDecimal !== 0 || longitudeDecimal !== 0
                    coordinate: QtPositioning.coordinate(latitudeDecimal, longitudeDecimal)
                    anchorPoint.x: 0
                    anchorPoint.y: 0

                    sourceItem: Item {
                        // Size doesn't matter for Item, children are positioned relative to (0,0)

                        // Dot centered at (0,0) - the coordinate point
                        Rectangle {
                            id: contactDot
                            x: -8  // Center the 16px dot on (0,0)
                            y: -8
                            width: 16
                            height: 16
                            radius: 8
                            color: {
                                switch (contactMarker.type) {
                                case MeshCoreDevice.AdvertTypeChat: return "#2196F3"
                                case MeshCoreDevice.AdvertTypeRepeater: return "#4CAF50"
                                case MeshCoreDevice.AdvertTypeRoom: return "#9C27B0"
                                default: return "#9E9E9E"
                                }
                            }
                            border.color: "white"
                            border.width: 2
                        }

                        // Label below the dot
                        Rectangle {
                            x: -width / 2  // Center horizontally on (0,0)
                            y: 10  // Below the dot (8 + 2px spacing)
                            color: {
                                switch (contactMarker.type) {
                                case MeshCoreDevice.AdvertTypeChat: return "#2196F3"
                                case MeshCoreDevice.AdvertTypeRepeater: return "#4CAF50"
                                case MeshCoreDevice.AdvertTypeRoom: return "#9C27B0"
                                default: return "#9E9E9E"
                                }
                            }
                            radius: 4
                            width: nameLabel.implicitWidth + 8
                            height: nameLabel.implicitHeight + 4

                            Label {
                                id: nameLabel
                                anchors.centerIn: parent
                                text: contactMarker.name || contactMarker.publicKeyHex.substring(0, 8)
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }
                    }
                }
            }
        }

        // Info panel at bottom
        Rectangle {
            visible: device.connected && device.selfInfo.latitudeDecimal !== 0
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: Material.background
            border.color: Material.dividerColor
            border.width: 1
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8

                Label {
                    text: "📍 Your location: " +
                          device.selfInfo.latitudeDecimal.toFixed(6) + ", " +
                          device.selfInfo.longitudeDecimal.toFixed(6)
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Center on Me"
                    flat: true
                    onClicked: {
                        map.center = QtPositioning.coordinate(
                            device.selfInfo.latitudeDecimal,
                            device.selfInfo.longitudeDecimal
                        )
                        map.zoomLevel = 12
                    }
                }
            }
        }
    }

    // Helper functions
    function countNodesWithLocation() {
        let count = 0
        for (let i = 0; i < device.contacts.count; i++) {
            let contact = device.contacts.get(i)
            if (contact && (contact.latitudeDecimal !== 0 || contact.longitudeDecimal !== 0)) {
                count++
            }
        }
        // Include self if has location
        if (device.selfInfo.latitudeDecimal !== 0 || device.selfInfo.longitudeDecimal !== 0) {
            count++
        }
        return count
    }

    function fitAllNodes() {
        // Collect all coordinates
        let coords = []

        // Add self position
        if (device.selfInfo.latitudeDecimal !== 0 || device.selfInfo.longitudeDecimal !== 0) {
            coords.push({
                lat: device.selfInfo.latitudeDecimal,
                lon: device.selfInfo.longitudeDecimal
            })
        }

        // Add contact positions
        for (let i = 0; i < device.contacts.count; i++) {
            let contact = device.contacts.get(i)
            if (contact && (contact.latitudeDecimal !== 0 || contact.longitudeDecimal !== 0)) {
                coords.push({
                    lat: contact.latitudeDecimal,
                    lon: contact.longitudeDecimal
                })
            }
        }

        if (coords.length === 0) {
            return
        }

        // Calculate bounding box
        let minLat = coords[0].lat
        let maxLat = coords[0].lat
        let minLon = coords[0].lon
        let maxLon = coords[0].lon

        for (let j = 1; j < coords.length; j++) {
            if (coords[j].lat < minLat) minLat = coords[j].lat
            if (coords[j].lat > maxLat) maxLat = coords[j].lat
            if (coords[j].lon < minLon) minLon = coords[j].lon
            if (coords[j].lon > maxLon) maxLon = coords[j].lon
        }

        // Center on bounding box
        let centerLat = (minLat + maxLat) / 2
        let centerLon = (minLon + maxLon) / 2
        map.center = QtPositioning.coordinate(centerLat, centerLon)

        // Calculate zoom level to fit all points
        let latDiff = maxLat - minLat
        let lonDiff = maxLon - minLon
        let maxDiff = Math.max(latDiff, lonDiff)

        if (maxDiff < 0.01) {
            map.zoomLevel = 15
        } else if (maxDiff < 0.1) {
            map.zoomLevel = 12
        } else if (maxDiff < 1) {
            map.zoomLevel = 9
        } else if (maxDiff < 10) {
            map.zoomLevel = 6
        } else {
            map.zoomLevel = 4
        }
    }

    // Auto-fit when contacts change
    Connections {
        target: device.contacts
        function onCountChanged() {
            if (countNodesWithLocation() > 0) {
                fitAllNodes()
            }
        }
    }
}
