// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Layouts

FocusScope {
    id: button
    property string label: ""
    property url iconSource
    property real uiScale: 1
    property var scrollTarget: null
    property bool selected: false
    onActiveFocusChanged: {
        if (!activeFocus || !scrollTarget) return;
        const y = mapToItem(scrollTarget.contentItem, 0, 0).y;
        if (y < scrollTarget.contentY) scrollTarget.contentY = Math.max(0, y - 8);
        else if (y + height > scrollTarget.contentY + scrollTarget.height)
            scrollTarget.contentY = Math.min(scrollTarget.contentHeight - scrollTarget.height, y + height - scrollTarget.height + 8);
    }
    signal triggered()
    implicitHeight: 42 * uiScale
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
        color: button.selected ? "#343052" : mouse.containsMouse ? "#242c3b" : "transparent"
        border.width: button.activeFocus ? 2 : 1
        border.color: button.activeFocus ? "#b7a7ff" : button.selected ? "#615087" : "transparent"
    }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 13
        anchors.rightMargin: 13
        spacing: 12
        Image {
            source: button.iconSource
            sourceSize: Qt.size(18 * button.uiScale, 18 * button.uiScale)
            Layout.preferredWidth: 18 * button.uiScale
            Layout.preferredHeight: 18 * button.uiScale
        }
        Text {
            text: button.label
            textFormat: Text.PlainText
            color: "#f0f2fa"
            font.pixelSize: 13 * button.uiScale
            font.weight: button.selected ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: { button.forceActiveFocus(); button.triggered(); }
    }
}
