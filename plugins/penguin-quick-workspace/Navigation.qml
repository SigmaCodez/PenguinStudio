// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "penguinQuickNavigation"
    color: "#11141b"
    implicitWidth: 260
    implicitHeight: 620 * uiScale
    property real uiScale: bridge.uiScale
    property color muted: "#929aaf"

    Flickable {
        id: scroll
        objectName: "navigationScroll"
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ColumnLayout {
            id: content
            x: 16; y: 16
            width: scroll.width - 32
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Scenes"; color: "#f4f5fc"; font.pixelSize: 22 * root.uiScale; font.weight: Font.DemiBold; Layout.fillWidth: true }
                WorkspaceButton {
                    objectName: "navManageScenes"
                    uiScale: root.uiScale; scrollTarget: scroll
                    label: "Manage"; Accessible.name: "Manage scenes"
                    iconSource: "icons/scene.svg"
                    Layout.preferredWidth: 104 * root.uiScale
                    onTriggered: bridge.openPanel("scenesDock")
                }
            }
            Text {
                text: "Choose the layout your viewers see."
                color: root.muted; font.pixelSize: 11 * root.uiScale
                wrapMode: Text.WordWrap; Layout.fillWidth: true; Layout.bottomMargin: 8
            }
            Rectangle {
                Layout.fillWidth: true; implicitHeight: 38 * root.uiScale
                radius: 8; color: "#1c222d"
                border.color: sceneSearch.activeFocus ? "#b7a7ff" : "#343d50"
                TextInput {
                    id: sceneSearch
                    objectName: "sceneSearch"
                    anchors.fill: parent; anchors.margins: 10
                    font.pixelSize: 12 * root.uiScale; color: "#f0f2fa"
                    selectionColor: "#7155db"; selectedTextColor: "white"
                    clip: true; selectByMouse: true; activeFocusOnTab: true
                    Accessible.name: "Search scenes"
                    Keys.onEscapePressed: text = ""
                    Text { anchors.fill: parent; visible: !sceneSearch.text; text: "Search scenes"; color: root.muted; font: sceneSearch.font }
                }
            }
            ListView {
                id: scenes
                objectName: "quickSceneList"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(90, Math.min(5, count) * 46) * root.uiScale
                clip: true; spacing: 4
                model: bridge.scenes.filter(function(scene) { return scene.name.toLocaleLowerCase().indexOf(sceneSearch.text.toLocaleLowerCase()) !== -1; })
                delegate: WorkspaceButton {
                    required property var modelData
                    uiScale: root.uiScale; scrollTarget: scenes
                    width: scenes.width; label: modelData.name; iconSource: "icons/scene.svg"
                    selected: bridge.currentScene === modelData.id
                    onTriggered: bridge.selectScene(modelData.id)
                }
                Text { anchors.centerIn: parent; visible: scenes.count === 0; text: sceneSearch.text ? "No matching scenes" : "Use Manage to add a scene"; color: root.muted; font.pixelSize: 11 * root.uiScale }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#292e3a"; Layout.topMargin: 8; Layout.bottomMargin: 12 }
            Text { text: "YOUR TOOLS"; color: root.muted; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1.2; Layout.leftMargin: 12 }
            WorkspaceButton {
                objectName: "navInspector"; uiScale: root.uiScale; scrollTarget: scroll
                label: "Edit selected source"; iconSource: "icons/inspector.svg"; Layout.fillWidth: true
                selected: bridge.activePanel === "penguinInspectorDock"
                onTriggered: bridge.openPanel("penguinInspectorDock")
            }
            WorkspaceButton {
                objectName: "navOutputs"; uiScale: root.uiScale; scrollTarget: scroll
                label: "Streaming channels"; iconSource: "icons/outputs.svg"; Layout.fillWidth: true
                selected: bridge.activePanel === "penguin-broadcast-destinations"
                onTriggered: bridge.openPanel("penguin-broadcast-destinations")
            }
            WorkspaceButton {
                objectName: "navDetails"; uiScale: root.uiScale; scrollTarget: scroll
                label: "Title & category"; iconSource: "icons/details.svg"; Layout.fillWidth: true
                selected: bridge.activePanel === "penguin-stream-details"
                onTriggered: bridge.openPanel("penguin-stream-details")
            }
            WorkspaceButton {
                objectName: "navChat"; uiScale: root.uiScale; scrollTarget: scroll
                label: "Live chat"; iconSource: "icons/chat.svg"; Layout.fillWidth: true
                selected: bridge.activePanel === "penguinTwitchChat"
                onTriggered: bridge.openPanel("penguinTwitchChat")
            }
            Rectangle {
                Layout.fillWidth: true; Layout.topMargin: 12
                implicitHeight: guidance.implicitHeight + 24
                color: "#1c222d"; radius: 10
                ColumnLayout {
                    id: guidance
                    x: 12; y: 12; width: parent.width - 24; spacing: 6
                    Text { text: "Start here"; color: "#edf1fa"; font.pixelSize: 12 * root.uiScale; font.weight: Font.DemiBold }
                    Text {
                        text: "1. Add a source below the preview.\n2. Check your audio levels.\n3. Record a test before going live."
                        color: root.muted; font.pixelSize: 11 * root.uiScale; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                }
            }
            WorkspaceButton { objectName: "navLayout"; uiScale: root.uiScale; scrollTarget: scroll; label: "Workspace layout"; iconSource: "icons/scene.svg"; Layout.fillWidth: true; Layout.topMargin: 8; onTriggered: bridge.showLayoutMenu() }
        }
    }
    Rectangle {
        anchors.right: parent.right; anchors.rightMargin: 2
        width: 3; radius: 1.5; color: "#59657b"
        visible: scroll.contentHeight > scroll.height
        height: Math.max(24, scroll.height * scroll.visibleArea.heightRatio)
        y: Math.min(parent.height - height, scroll.visibleArea.yPosition * scroll.height)
    }
}
