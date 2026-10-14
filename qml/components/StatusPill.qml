import QtQuick

// Status chip with its shortcut key; used in the inspector and the Focus bar.
Rectangle {
    id: root
    property int status: 0
    property bool active: false
    signal clicked()

    implicitHeight: 28
    implicitWidth: row.implicitWidth + 20
    radius: Theme.rControl
    color: active ? Theme.alpha(Statuses.color(status), 0.16) : hover.hovered ? Theme.elevated : Theme.alpha(Theme.raised, 0.7)
    border.color: active ? Theme.alpha(Statuses.color(status), 0.6) : Theme.hairlineStrong
    Behavior on color { ColorAnimation { duration: Theme.fast } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Icon { name: Statuses.icon(root.status); weight: "bold"; size: 12; color: root.active ? Statuses.color(root.status) : Theme.textDim; anchors.verticalCenter: parent.verticalCenter }
        Txt { text: Statuses.name(root.status); variant: "label"; font.pixelSize: 12; color: root.active ? Statuses.color(root.status) : Theme.text; anchors.verticalCenter: parent.verticalCenter }
        Txt { text: Statuses.key(root.status); variant: "mono"; muted: true; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
    }
    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.clicked() }
}
