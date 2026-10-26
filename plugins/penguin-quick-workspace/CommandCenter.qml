// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Layouts
Rectangle {
    id: root
    property real uiScale: commandBridge.uiScale
    Keys.onEscapePressed: commandBridge.dismiss()
    color: "#11141b"
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14
        Text { text: "PENGUIN STUDIO"; color: "#b7a7ff"; font.pixelSize: 10 * root.uiScale; font.letterSpacing: 1.4 }
        Text { text: "Command center"; color: "#f0f2fa"; font.pixelSize: 26 * root.uiScale; font.weight: Font.DemiBold }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 48 * root.uiScale
            radius: 10
            color: "#1c222d"
            border.color: search.activeFocus ? "#b7a7ff" : "#343d50"
            TextInput {
                id: search
                objectName: "commandSearch"
                anchors.fill: parent
                anchors.margins: 14
                color: "#f0f2fa"
                font.pixelSize: 14 * root.uiScale
                clip: true
                focus: true
                selectByMouse: true
                Accessible.name: "Search commands"
                onTextChanged: results.currentIndex = 0
                Keys.onDownPressed: { results.currentIndex = Math.min(results.count - 1, results.currentIndex + 1); results.positionViewAtIndex(results.currentIndex, ListView.Contain); }
                Keys.onUpPressed: { results.currentIndex = Math.max(0, results.currentIndex - 1); results.positionViewAtIndex(results.currentIndex, ListView.Contain); }
                Keys.onReturnPressed: { if (results.currentItem) commandBridge.activate(results.currentItem.commandIndex); }
                Keys.onEnterPressed: { if (results.currentItem) commandBridge.activate(results.currentItem.commandIndex); }
                Keys.onEscapePressed: commandBridge.dismiss()
                Text { anchors.fill: parent; text: "Search actions, settings and tools…"; visible: !search.text; color: "#929aaf"; font: search.font }
            }
        }
        ListView {
            id: results
            objectName: "commandResults"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: commandBridge.commands.filter(function(command) { return (command.title + " " + command.group).toLocaleLowerCase().indexOf(search.text.toLocaleLowerCase()) !== -1; })
            delegate: FocusScope {
                id: row
                required property var modelData
                required property int index
                property int commandIndex: modelData.index
                width: results.width
                height: 62 * root.uiScale
                activeFocusOnTab: modelData.enabled
                Accessible.role: Accessible.Button
                Accessible.name: modelData.title
                Accessible.description: modelData.group
                Accessible.onPressAction: commandBridge.activate(commandIndex)
                Keys.onReturnPressed: commandBridge.activate(commandIndex)
                Keys.onEnterPressed: commandBridge.activate(commandIndex)
                Keys.onSpacePressed: commandBridge.activate(commandIndex)
                onActiveFocusChanged: if (activeFocus) { results.currentIndex = index; results.positionViewAtIndex(index, ListView.Contain); }
                Rectangle {
                    anchors.fill: parent
                    radius: 9
                    color: row.ListView.isCurrentItem ? "#343052" : mouse.containsMouse ? "#202735" : "transparent"
                    border.color: row.activeFocus ? "#b7a7ff" : row.ListView.isCurrentItem ? "#615087" : "transparent"
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: (row.modelData.checked ? "✓ " : "") + row.modelData.title; textFormat: Text.PlainText; color: row.modelData.enabled ? "#f0f2fa" : "#788297"; font.pixelSize: 14 * root.uiScale; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: row.modelData.group; textFormat: Text.PlainText; color: "#929aaf"; font.pixelSize: 11 * root.uiScale; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                    Text { text: row.modelData.shortcut; textFormat: Text.PlainText; color: "#929aaf"; font.pixelSize: 11 * root.uiScale }
                }
                MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: row.modelData.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor; onClicked: commandBridge.activate(row.commandIndex) }
            }
            Rectangle {
                anchors.right: parent.right
                width: 3
                radius: 1.5
                color: "#59657b"
                visible: results.contentHeight > results.height
                height: Math.max(24, results.height * results.visibleArea.heightRatio)
                y: Math.min(parent.height - height, results.visibleArea.yPosition * results.height)
            }
            Text { anchors.centerIn: parent; visible: results.count === 0; text: "No matching commands"; color: "#929aaf"; font.pixelSize: 14 * root.uiScale }
        }
        Text { text: "↑ ↓ to browse · Enter to run · Esc to close"; color: "#929aaf"; font.pixelSize: 11 * root.uiScale }
    }
}
