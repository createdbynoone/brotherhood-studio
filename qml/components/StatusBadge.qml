import QtQuick

// Circular status marker on cards.
Rectangle {
    property int status: 0
    property int size: 20
    width: size; height: size; radius: size / 2
    color: Statuses.color(status)
    Icon {
        anchors.centerIn: parent
        name: Statuses.icon(status)
        weight: "bold"
        size: parent.size * 0.52
        color: status === 2 ? Theme.accentInk : "#FFFFFF"
    }
}
