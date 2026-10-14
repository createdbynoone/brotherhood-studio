pragma Singleton
import QtQuick

// Triage statuses, indexed like the C++ enum: 0 unsorted, 1 keep, 2 maybe, 3 discard, 4 archived.
QtObject {
    readonly property var all: [
        { name: "Unsorted", key: "U", icon: "circle-dashed", color: Theme.textDim },
        { name: "Keep", key: "K", icon: "arrow-up", color: Theme.ok },
        { name: "Maybe", key: "M", icon: "approximate-equals", color: Theme.warn },
        { name: "Discard", key: "D", icon: "x", color: Theme.danger },
        { name: "Archived", key: "A", icon: "archive-box", color: Theme.info }
    ]
    // Order used in pills and the Focus bar
    readonly property var pillOrder: [1, 2, 3, 4, 0]
    function name(i) { return all[i].name }
    function key(i) { return all[i].key }
    function icon(i) { return all[i].icon }
    function color(i) { return all[i].color }
}
