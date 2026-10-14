import QtQuick

Rectangle {
    id: root
    default property alias content: inner.data
    property int padding: 14
    property bool raisedTone: false
    implicitHeight: inner.childrenRect.height + padding * 2
    radius: Theme.rCard
    color: raisedTone ? Theme.raised : Theme.surface
    border.color: Theme.hairline
    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
