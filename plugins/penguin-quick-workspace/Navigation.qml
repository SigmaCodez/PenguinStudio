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

    Flickable {
        id: scroll
        objectName: "navigationScroll"
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, 660 * root.uiScale + 180)
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
        WorkspaceButton { uiScale: root.uiScale; scrollTarget: scroll; objectName: "navSources"; label: "Sources"; iconSource: "icons/sources.svg"; Layout.fillWidth: true; onTriggered: bridge.openPanel("sourcesDock") }
        WorkspaceButton { uiScale: root.uiScale; scrollTarget: scroll; objectName: "navAudio"; label: "Audio mixer"; iconSource: "icons/audio.svg"; Layout.fillWidth: true; onTriggered: bridge.openPanel("mixerDock") }
        WorkspaceButton {
                uiScale: root.uiScale; scrollTarget: scroll; objectName: "navInspector"; label: "Inspector"; iconSource: "icons/inspector.svg"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguinInspectorDock") }
        WorkspaceButton {
                uiScale: root.uiScale; scrollTarget: scroll; objectName: "navOutputs"; label: "Destinations"; iconSource: "icons/outputs.svg"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguin-broadcast-destinations") }
        WorkspaceButton {
                uiScale: root.uiScale; scrollTarget: scroll; objectName: "navDetails"; label: "Stream details"; iconSource: "icons/details.svg"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguin-stream-details") }
        WorkspaceButton {
                uiScale: root.uiScale; scrollTarget: scroll; objectName: "navChat"; label: "Live chat"; iconSource: "icons/chat.svg"; Layout.fillWidth: true; onTriggered: bridge.openPanel("penguinTwitchChat") }
        Rectangle { color: "#292e3a"; Layout.fillWidth: true; height: 1; Layout.topMargin: 12; Layout.bottomMargin: 12 }
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: 12; Layout.rightMargin: 10
            Text { text: "SCENES"; color: root.muted; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1.4; Layout.fillWidth: true }
            Text { text: bridge.scenes.length; color: root.muted; font.pixelSize: 11 * root.uiScale }
            WorkspaceButton {
                objectName: "navManageScenes"
                uiScale: root.uiScale
                scrollTarget: scroll
                label: "Edit"
                Accessible.name: "Manage scenes"
                iconSource: "icons/scene.svg"
                Layout.preferredWidth: 100 * root.uiScale
                onTriggered: bridge.openPanel("scenesDock")
            }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 38 * root.uiScale
            radius: 8
            color: "#1c222d"
            border.color: sceneSearch.activeFocus ? "#b7a7ff" : "#343d50"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Image { source: "icons/search.svg"; sourceSize: Qt.size(16, 16); Layout.preferredWidth: 16; Layout.preferredHeight: 16 }
                TextInput {
                    id: sceneSearch
                    objectName: "sceneSearch"
                    Layout.fillWidth: true
                    font.pixelSize: 12 * root.uiScale
                    color: "#f0f2fa"
                    selectionColor: "#7155db"
                    selectedTextColor: "white"
                    clip: true
                    selectByMouse: true
                    activeFocusOnTab: true
                    Accessible.name: "Search scenes"
                    Keys.onEscapePressed: text = ""
                    Text { anchors.fill: parent; visible: !sceneSearch.text; text: "Search scenes"; color: "#929aaf"; font: sceneSearch.font }
                }
            }
        }
        ListView {
            id: scenes
            objectName: "quickSceneList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            clip: true
            spacing: 4
            model: bridge.scenes.filter(function(scene) { return scene.name.toLocaleLowerCase().indexOf(sceneSearch.text.toLocaleLowerCase()) !== -1; })
            delegate: WorkspaceButton {
                uiScale: root.uiScale; scrollTarget: scroll;
                required property var modelData
                width: scenes.width
                label: modelData.name
                iconSource: "icons/scene.svg"
                selected: bridge.currentScene === modelData.id
                onTriggered: bridge.selectScene(modelData.id)
            }
            Text { anchors.centerIn: parent; visible: scenes.count === 0; text: sceneSearch.text ? "No matching scenes" : "Add a scene to begin"; color: root.muted; font.pixelSize: 12 * root.uiScale }
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
        WorkspaceButton { uiScale: root.uiScale; scrollTarget: scroll; objectName: "navLayout"; label: "Workspace layout"; iconSource: "icons/inspector.svg"; Layout.fillWidth: true; onTriggered: bridge.showLayoutMenu() }
        Text { text: "Local design preview"; color: "#707a91"; font.pixelSize: 10 * root.uiScale; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 6 }
    }
}
    Rectangle {
        // Keep overflow discoverable at large text sizes and short window heights.
        anchors.right: parent.right
        anchors.rightMargin: 2
        width: 3
        radius: 1.5
        color: "#59657b"
        visible: scroll.contentHeight > scroll.height
        height: Math.max(24, scroll.height * scroll.visibleArea.heightRatio)
        y: Math.min(parent.height - height, scroll.visibleArea.yPosition * scroll.height)
    }
}
