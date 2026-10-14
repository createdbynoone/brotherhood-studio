import QtQuick

// Header strip of each pane. In the split layout the active pane's title is brighter.
Rectangle {
    id: root
    property string title: ""
    property bool active: false
    default property alias actions: actionRow.data

    implicitHeight: 44
    color: Theme.alpha(Theme.surface, 0.7)

    Hairline { anchors.bottom: parent.bottom }

    Txt {
        text: root.title
        variant: "heading"
        font.pixelSize: 13
        color: root.active ? Theme.text : Theme.textDim
        Behavior on color { ColorAnimation { duration: Theme.base } }
        anchors.left: parent.left; anchors.leftMargin: Theme.s16
        anchors.verticalCenter: parent.verticalCenter
    }
    Row {
        id: actionRow
        anchors.right: parent.right; anchors.rightMargin: Theme.s12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
    }
}
