import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

// Sorter pane: header + filters + grid or focus view, with the floating inspector on top.
Rectangle {
    id: root
    property bool active: false
    property bool freezeLayout: false      // hold the grid's arrangement while the pane itself is gliding
    color: Theme.bg
    readonly property bool focusing: triage.focusIndex >= 0
    function requestExport(path) { exportDialog.openFor(path) }
    function requestTrash() { trashConfirm.open() }
    function requestImport() { importDialog.open() }

    FolderDialog { id: importDialog; title: "Select a folder with generations"; onAccepted: triage.importFolder(selectedFolder) }

    Column {
        anchors.fill: parent

        PaneHeader {
            width: parent.width
            title: "Generation Sorter"
            active: root.active

            Btn { kind: "ghost"; size: "s"; icon: "squares-four"; tip: "Sort and grid size"; hint: "[ ]"; onClicked: viewMenu.popup() }
            Btn {
                visible: (triage.statusCounts[3] || 0) > 0
                kind: "destructive"; size: "s"; icon: "trash"; text: String(triage.statusCounts[3])
                tip: "Move discarded files to the Trash"; hint: "⌘⌫"
                onClicked: trashConfirm.open()
            }
            Btn { kind: "ghost"; size: "s"; icon: "arrow-clockwise"; tip: "Rescan the output folder"; hint: "⌘⌥R"; onClicked: triage.rescan() }
            Btn { kind: "ghost"; size: "s"; icon: "folder-plus"; tip: "Import a folder"; hint: "⌘I"; onClicked: importDialog.open() }
            Btn { kind: "ghost"; size: "s"; icon: "sidebar"; tip: "Toggle inspector"; hint: "I"; onClicked: triage.inspectorOpen = !triage.inspectorOpen }
        }

        FilterStrip { width: parent.width; visible: !root.focusing; height: visible ? implicitHeight : 0 }

        Item {
            id: body
            width: parent.width
            height: parent.height - y

            GridArea {
                id: grid
                anchors.fill: parent
                visible: !root.focusing
                freeze: root.freezeLayout
                inspectorItem: inspector.panel
                onExportRequested: path => exportDialog.openFor(path)
            }
            Loader {
                id: focusLoader
                anchors.fill: parent
                active: root.focusing
                sourceComponent: FocusView { inspectorItem: inspector.panel; onExportRequested: path => exportDialog.openFor(path) }
            }
            InspectorOverlay {
                id: inspector
                anchors.fill: parent
                topInset: root.focusing ? 44 : 0
                bottomInset: root.focusing ? 52 : 0
                blurSource: root.focusing ? focusLoader : grid
                entry: root.focusing ? triage.focusEntry : triage.anchorEntry
            }
        }
    }

    StudioMenu {
        id: viewMenu
        MenuItem { text: "Sort by"; enabled: false }
        MenuItem { text: "Newest first"; checkable: true; checked: triage.sort === 0; onTriggered: triage.sort = 0 }
        MenuItem { text: "Oldest first"; checkable: true; checked: triage.sort === 1; onTriggered: triage.sort = 1 }
        MenuItem { text: "Status"; checkable: true; checked: triage.sort === 2; onTriggered: triage.sort = 2 }
        MenuItem { text: "Name"; checkable: true; checked: triage.sort === 3; onTriggered: triage.sort = 3 }
        MenuSeparator {}
        MenuItem { text: "Grid size"; enabled: false }
        MenuItem { text: "Small  120 px"; checkable: true; checked: triage.gridSize === 120; onTriggered: triage.gridSize = 120 }
        MenuItem { text: "Compact  160 px"; checkable: true; checked: triage.gridSize === 160; onTriggered: triage.gridSize = 160 }
        MenuItem { text: "Medium  220 px"; checkable: true; checked: triage.gridSize === 220; onTriggered: triage.gridSize = 220 }
        MenuItem { text: "Large  300 px"; checkable: true; checked: triage.gridSize === 300; onTriggered: triage.gridSize = 300 }
        MenuItem { text: "Extra large  400 px"; checkable: true; checked: triage.gridSize === 400; onTriggered: triage.gridSize = 400 }
    }

    ExportDialog { id: exportDialog }
    ConfirmDialog {
        id: trashConfirm
        title: "Move " + (triage.statusCounts[3] || 0) + " discarded files to the Trash?"
        message: "Files go to the Trash of their own volume, and their entries leave the library."
        confirmText: "Move to Trash"
        destructive: true
        onConfirmed: triage.trashDiscarded()
    }
}
