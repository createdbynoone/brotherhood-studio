import QtQuick

// Small caption above a Segment (the fire bar's building block).
Column {
    property string title: ""
    property alias options: seg.options
    property alias current: seg.current
    signal picked(var key)
    spacing: 6
    Txt { text: parent.title; variant: "label"; muted: true }
    Segment { id: seg; onPicked: key => parent.picked(key) }
}
