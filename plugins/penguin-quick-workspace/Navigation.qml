// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "penguinQuickNavigation"
    color: "#11141b"
    implicitWidth: 240
    implicitHeight: 620 * root.uiScale
    property real uiScale: bridge.uiScale
    property color muted: "#929aaf"

    component NavButton: FocusScope {
        id: button
        property string label: ""
        property string symbol: ""
        property bool selected: false
        signal triggered()
        implicitHeight: 42 * root.uiScale
        implicitWidth: 200
        activeFocusOnTab: true
        Accessible.role: Accessible.Button
        Accessible.name: label
        Accessible.onPressAction: triggered()
        Keys.onReturnPressed: triggered()
        Keys.onEnterPressed: triggered()
        Keys.onSpacePressed: triggered()
        Rectangle {
            anchors.fill: parent
            radius: 9
            color: button.selected ? "#343052" : mouse.containsMouse ? "#242936" : "transparent"
            border.width: button.activeFocus ? 2 : 1
            border.color: button.activeFocus ? "#b7a7ff" : button.selected ? "#615087" : "transparent"
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 13
            anchors.rightMargin: 13
            spacing: 12
            Text { text: button.symbol; color: button.selected ? "#c5b9ff" : root.muted; font.pixelSize: 14 * root.uiScale; Layout.preferredWidth: 18 * root.uiScale; horizontalAlignment: Text.AlignHCenter }
            Text { text: button.label; textFormat: Text.PlainText; color: "#f0f2fa"; font.pixelSize: 13 * root.uiScale; font.weight: button.selected ? Font.DemiBold : Font.Normal; elide: Text.ElideRight; Layout.fillWidth: true }
        }
        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { button.forceActiveFocus(); button.triggered(); } }
    }

    Flickable {
        id: scroll
        objectName: "navigationScroll"
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, 420 * root.uiScale + 200)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
    ColumnLayout {
        x: 16
        y: 16
        width: scroll.width - 32
        height: scroll.contentHeight - 32
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 16
            Rectangle { width: 36 * root.uiScale; height: 36 * root.uiScale; radius: 11; color: "#8b70f9"
                Text { anchors.centerIn: parent; text: "P"; color: "white"; font.pixelSize: 22 * root.uiScale; font.weight: Font.Bold }
            }
            ColumnLayout { spacing: 2
                Text { text: "Penguin Studio"; color: "#f4f5fc"; font.pixelSize: 16 * root.uiScale; font.weight: Font.DemiBold }
                Text { text: "YOUR CREATIVE SPACE"; color: root.muted; font.pixelSize: 9 * root.uiScale; font.letterSpacing: 1 }
            }
        }
        Text { text: "WORKSPACE"; color: root.muted; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1.4; Layout.leftMargin: 12; Layout.bottomMargin: 3 }
        NavButton { objectName: "navInspector"; label: "Inspector"; symbol: "I"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguinInspectorDock") }
        NavButton { objectName: "navOutputs"; label: "Destinations"; symbol: "↗"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguin-broadcast-destinations") }
        NavButton { objectName: "navDetails"; label: "Stream details"; symbol: "≡"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguin-stream-details") }
        NavButton { objectName: "navChat"; label: "Live chat"; symbol: "••"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguinTwitchChat") }
        Rectangle { color: "#292e3a"; Layout.fillWidth: true; height: 1; Layout.topMargin: 12; Layout.bottomMargin: 12 }
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: 12; Layout.rightMargin: 10
            Text { text: "SCENES"; color: root.muted; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1.4; Layout.fillWidth: true }
            Text { text: bridge.scenes.length; color: root.muted; font.pixelSize: 11 * root.uiScale }
        }
        ListView {
            id: scenes
            objectName: "quickSceneList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            clip: true
            spacing: 4
            model: bridge.scenes
            delegate: NavButton {
                required property var modelData
                width: scenes.width
                label: modelData.name
                symbol: "◉"
                selected: bridge.currentScene === modelData.id
                onTriggered: bridge.selectScene(modelData.id)
            }
            Text { anchors.centerIn: parent; visible: scenes.count === 0; text: "Add a scene to begin"; color: root.muted; font.pixelSize: 12 * root.uiScale }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 62 * root.uiScale
            color: "#1c222d"; radius: 10
            RowLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 10
                Rectangle { width: 7; height: 7; radius: 4; color: bridge.live ? "#f57691" : "#82d7ba" }
                ColumnLayout { spacing: 3
                    Text { text: bridge.live ? "Primary stream active" : "Workspace ready"; color: "#edf1fa"; font.pixelSize: 12 * root.uiScale; font.weight: Font.DemiBold }
                    Text { text: bridge.recording ? "Recording in progress" : "Preview workspace"; color: root.muted; font.pixelSize: 11 * root.uiScale }
                }
            }
        }
        Text { text: "QML workspace preview"; color: "#707a91"; font.pixelSize: 10 * root.uiScale; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 6 }
    }
}
}
