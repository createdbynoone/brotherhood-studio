import QtQuick

// Header strip of each pane. In the split layout the active pane's title is brighter.
Rectangle {
    id: root
    property string title: ""
    property string subtitle: ""
    property bool active: false
    default property alias actions: actionRow.data

    implicitHeight: 44
    color: Theme.alpha(Theme.surface, 0.7)

    Hairline { anchors.bottom: parent.bottom }

    Row {
        anchors.left: parent.left; anchors.leftMargin: Theme.s16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10
        Txt {
            text: root.title
            variant: "title"
            color: root.active ? Theme.text : Theme.textDim
            Behavior on color { ColorAnimation { duration: Theme.base } }
            anchors.verticalCenter: parent.verticalCenter
        }
        Txt { visible: root.subtitle.length > 0; text: root.subtitle; variant: "small"; muted: true; anchors.verticalCenter: parent.verticalCenter }
    }
    Row {
        id: actionRow
        anchors.right: parent.right; anchors.rightMargin: Theme.s12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
    }
}
