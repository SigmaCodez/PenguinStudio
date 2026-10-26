// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#11141b"
    property real uiScale: settingsBridge.uiScale
    Flickable {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: content.implicitHeight + 40
        boundsBehavior: Flickable.StopAtBounds
        ColumnLayout {
            id: content
            x: 16
            y: 20
            width: scroll.width - 32
            spacing: 6
            Text { text: "PENGUIN STUDIO"; color: "#b7a7ff"; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1 }
            Text { text: "Settings"; color: "#f0f2fa"; font.pixelSize: 24 * root.uiScale; font.weight: Font.DemiBold; Layout.bottomMargin: 14 }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 38 * root.uiScale
                radius: 8
                color: "#1c222d"
                border.color: search.activeFocus ? "#b7a7ff" : "#343d50"
                TextInput {
                    id: search
                    objectName: "settingsSearch"
                    anchors.fill: parent
                    anchors.margins: 10
                    color: "#f0f2fa"
                    font.pixelSize: 12 * root.uiScale
                    clip: true
                    selectByMouse: true
                    activeFocusOnTab: true
                    Accessible.name: "Find a settings page"
                    Keys.onEscapePressed: text = ""
                    Text { anchors.fill: parent; text: "Find a page…"; visible: !search.text; color: "#929aaf"; font: search.font }
                }
            }
            Text { text: "YOUR STUDIO"; color: "#929aaf"; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1; Layout.topMargin: 14; Layout.bottomMargin: 6 }
            Repeater {
                id: pages
                objectName: "settingsPageList"
                model: settingsBridge.pages.filter(function(page) { return page.title.toLocaleLowerCase().indexOf(search.text.toLocaleLowerCase()) !== -1; })
                delegate: WorkspaceButton {
                    required property var modelData
                    objectName: "settingsPage" + modelData.index
                    uiScale: root.uiScale
                    scrollTarget: scroll
                    label: modelData.title
                    iconSource: "icons/" + modelData.icon + ".svg"
                    selected: settingsBridge.currentPage === modelData.index
                    Layout.fillWidth: true
                    onTriggered: settingsBridge.selectPage(modelData.index)
                }
            }
            Text { text: "No matching pages"; visible: pages.count === 0; color: "#929aaf"; font.pixelSize: 12 * root.uiScale; Layout.topMargin: 8 }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#292e3a"; Layout.topMargin: 16; Layout.bottomMargin: 12 }
            Text { text: "Changes are saved with Apply or OK."; color: "#929aaf"; font.pixelSize: 11 * root.uiScale; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }
    Rectangle {
        anchors.right: parent.right
        width: 3
        radius: 1.5
        color: "#59657b"
        visible: scroll.contentHeight > scroll.height
        height: Math.max(24, scroll.height * scroll.visibleArea.heightRatio)
        y: Math.min(parent.height - height, scroll.visibleArea.yPosition * scroll.height)
    }
}
