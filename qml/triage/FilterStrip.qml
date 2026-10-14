import QtQuick

// Status filter (with live counts) and search, one slim row under the pane header.
Rectangle {
    id: root
    color: Theme.bg
    implicitHeight: 48
    Hairline { anchors.bottom: parent.bottom }

    readonly property var names: ["All", "Unsorted", "Keep", "Maybe", "Discard", "Archived"]

    Segment {
        anchors.left: parent.left; anchors.leftMargin: Theme.s16
        anchors.verticalCenter: parent.verticalCenter
        options: root.names.map((n, i) => ({ key: i, label: n + "  " + triage.filterCounts[i] }))
        current: triage.filter
        onPicked: key => { triage.filter = key; triage.clearSelection() }
    }
    Field {
        id: search
        anchors.right: parent.right; anchors.rightMargin: Theme.s16
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(130, Math.min(220, parent.width - 560))
        icon: "magnifying-glass"; placeholder: "Search name or note"; clearable: true
        text: triage.search
        onEdited: triage.search = text
    }
}
