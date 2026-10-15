import QtQuick

// Status filter (with live counts) and search, one slim row under the pane header.
Rectangle {
    id: root
    color: Theme.bg
    implicitHeight: 48
    Hairline { anchors.bottom: parent.bottom }

    readonly property var names: ["All", "Unsorted", "Keep", "Maybe", "Discard", "Archived"]

    // Narrow pane: the counts go first, and the search field gives way when even that does not leave it room
    readonly property bool compact: width < 720
    Segment {
        id: seg
        anchors.left: parent.left; anchors.leftMargin: Theme.s16
        anchors.verticalCenter: parent.verticalCenter
        options: root.names.map((n, i) => ({ key: i, label: root.compact ? n : n + "  " + triage.filterCounts[i] }))
        current: triage.filter
        onPicked: key => { triage.filter = key; triage.clearSelection() }
    }
    Field {
        id: search
        anchors.right: parent.right; anchors.rightMargin: Theme.s16
        anchors.verticalCenter: parent.verticalCenter
        readonly property real room: root.width - Theme.s16 * 2 - seg.width - 12
        visible: room >= 120
        width: Math.max(120, Math.min(220, room))
        icon: "magnifying-glass"; placeholder: "Search name or note"; clearable: true
        text: triage.search
        onEdited: triage.search = text
    }
}
