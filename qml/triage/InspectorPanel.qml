import QtQuick
import QtQuick.Controls.Basic

// Contents of the floating inspector: file, status, note, category, product.
Item {
    id: root
    property var entry: null
    property bool syncing: false

    // Category lists depend on the store revision so renames / deletes refresh the chips
    readonly property var parents: { triage.revision; return triage.parentCategories() }
    readonly property string activeParent: { triage.revision; return entry ? triage.categoryOf(entry.path) : "" }
    readonly property var subs: { triage.revision; return activeParent.length ? triage.subCategories(activeParent) : [] }
    property string info: ""
    property string addingFor: "none"     // "none" | "parent" | <parentId>
    property var renaming: null
    property var deleting: null

    onEntryChanged: {
        syncing = true
        noteBox.text = entry ? entry.note : ""
        syncing = false
        info = entry ? triage.mediaInfo(entry.path) : ""
        addingFor = "none"
    }
    Connections {
        target: triage
        function onFocusNoteChanged() { if (triage.focusNote) noteBox.forceFocus() }
    }

    // Empty
    Column {
        visible: !root.entry
        anchors.centerIn: parent
        spacing: 10
        BrandMark { size: 28; tint: Theme.textMuted; anchors.horizontalCenter: parent.horizontalCenter }
        Txt { text: "Select an image to inspect"; variant: "small"; dim: true; anchors.horizontalCenter: parent.horizontalCenter }
    }

    Flickable {
        visible: root.entry !== null
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: StudioScrollBar {}

        Column {
            id: col
            x: 16; y: 16
            width: parent.width - 32
            spacing: 18

            // File
            Column {
                width: parent.width
                spacing: 5
                Txt { width: parent.width; text: root.entry ? root.entry.filename : ""; variant: "strong"; font.pixelSize: 13; elide: Text.ElideMiddle }
                Txt {
                    width: parent.width
                    text: root.entry ? root.entry.addedLabel + (root.info ? "  ·  " + root.info : "") + (root.entry.isVideo ? "  ·  Video" : "") : ""
                    variant: "caption"; dim: true
                }
                Row {
                    spacing: 6
                    topPadding: 6
                    Btn { kind: "secondary"; size: "xs"; text: "Reveal"; hint: "R"; onClicked: triage.reveal(root.entry.path) }
                    Btn { kind: "secondary"; size: "xs"; text: "Open"; onClicked: triage.open(root.entry.path) }
                    Txt { visible: root.entry && root.entry.missing; text: "Missing"; variant: "caption"; color: Theme.warn; anchors.verticalCenter: parent.verticalCenter }
                }
            }

            Hairline {}

            // Status
            Column {
                width: parent.width
                spacing: 8
                Txt { text: "Status"; variant: "label"; dim: true }
                Flow {
                    width: parent.width
                    spacing: 4
                    Repeater {
                        model: Statuses.pillOrder
                        delegate: StatusPill {
                            required property int modelData
                            status: modelData
                            active: root.entry && root.entry.status === modelData
                            onClicked: triage.setStatus([root.entry.path], modelData)
                        }
                    }
                }
            }

            // Note
            Column {
                width: parent.width
                spacing: 8
                Row { spacing: 6; Txt { text: "Note"; variant: "label"; dim: true } Txt { text: "N"; variant: "mono"; muted: true; font.pixelSize: 10 } }
                TextBox {
                    id: noteBox
                    width: parent.width
                    implicitHeight: 86
                    fontSize: 13
                    placeholder: "Changes, ideas, what to fix"
                    onEdited: if (!root.syncing) save.restart()
                    editItem.onActiveFocusChanged: if (!editItem.activeFocus && triage.focusNote) triage.focusNote = false
                    Timer {
                        id: save; interval: 400
                        onTriggered: if (root.entry && noteBox.text !== root.entry.note) triage.setNote(root.entry.path, noteBox.text)
                    }
                }
            }

            // Category
            Column {
                width: parent.width
                spacing: 8
                Item {
                    width: parent.width; height: 22
                    Txt { text: "Category"; variant: "label"; dim: true; anchors.verticalCenter: parent.verticalCenter }
                    Btn { anchors.right: parent.right; kind: "ghost"; size: "xs"; icon: "plus"; tip: "New category"; onClicked: { root.addingFor = "parent"; newField.text = ""; newField.forceFocus() } }
                }
                Flow {
                    width: parent.width
                    spacing: 4
                    Repeater {
                        model: root.parents
                        delegate: Chip {
                            required property var modelData
                            text: modelData.name
                            active: root.entry && root.entry.categories.indexOf(modelData.id) >= 0
                            onClicked: triage.toggleCategory(root.entry.path, modelData.id)
                            onRightClicked: { chipMenu.cat = modelData; chipMenu.popup() }
                        }
                    }
                    Field {
                        id: newField
                        visible: root.addingFor === "parent"
                        width: 140; implicitHeight: 26
                        placeholder: "New category"
                        onAccepted: {
                            const id = triage.addCategory(text, "")
                            if (id.length) triage.toggleCategory(root.entry.path, id)
                            root.addingFor = "none"
                        }
                        inputItem.Keys.onEscapePressed: root.addingFor = "none"
                    }
                }
            }

            // Product (sub-category of the active category)
            Column {
                visible: root.activeParent.length > 0
                width: parent.width
                spacing: 8
                Item {
                    width: parent.width; height: 22
                    Txt { text: "Product"; variant: "label"; dim: true; anchors.verticalCenter: parent.verticalCenter }
                    Btn { anchors.right: parent.right; kind: "ghost"; size: "xs"; icon: "plus"; tip: "New product"; onClicked: { root.addingFor = root.activeParent; newSub.text = ""; newSub.forceFocus() } }
                }
                Flow {
                    width: parent.width
                    spacing: 4
                    Repeater {
                        model: root.subs
                        delegate: Chip {
                            required property var modelData
                            text: modelData.name
                            active: root.entry && root.entry.categories.indexOf(modelData.id) >= 0
                            onClicked: triage.toggleCategory(root.entry.path, modelData.id)
                            onRightClicked: { chipMenu.cat = modelData; chipMenu.popup() }
                        }
                    }
                    Field {
                        id: newSub
                        visible: root.addingFor === root.activeParent && root.activeParent.length > 0
                        width: 140; implicitHeight: 26
                        placeholder: "New product"
                        onAccepted: {
                            const id = triage.addCategory(text, root.activeParent)
                            if (id.length) triage.toggleCategory(root.entry.path, id)
                            root.addingFor = "none"
                        }
                        inputItem.Keys.onEscapePressed: root.addingFor = "none"
                    }
                }
                Txt {
                    visible: root.subs.length === 0 && root.addingFor !== root.activeParent
                    text: "No products yet. Add one with +"
                    variant: "caption"; muted: true
                }
            }
        }
    }

    StudioMenu {
        id: chipMenu
        property var cat: null
        MenuItem { text: "Rename"; onTriggered: { renameDialog.cat = chipMenu.cat; renameDialog.open() } }
        MenuItem { text: "Delete..."; onTriggered: { deleteDialog.cat = chipMenu.cat; deleteDialog.open() } }
    }

    ConfirmDialog {
        id: deleteDialog
        property var cat: null
        title: "Delete \"" + (cat ? cat.name : "") + "\"?"
        message: "Every image loses this category, and its products are deleted too."
        confirmText: "Delete"
        destructive: true
        onConfirmed: triage.deleteCategory(cat.id)
    }
    Popup {
        id: renameDialog
        property var cat: null
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        modal: true; width: 340; padding: 18
        Overlay.modal: Rectangle { color: Theme.scrim }
        background: Rectangle { radius: Theme.rPanel; color: Theme.surface; border.color: Theme.hairlineStrong }
        onAboutToShow: { rn.text = cat ? cat.name : ""; rn.forceFocus(); rn.inputItem.selectAll() }
        contentItem: Column {
            spacing: 12
            Txt { text: "Rename"; variant: "title" }
            Field { id: rn; width: parent.width; implicitHeight: 34; onAccepted: { triage.renameCategory(renameDialog.cat.id, text); renameDialog.close() } }
            Row {
                anchors.right: parent.right; spacing: 8
                Btn { kind: "secondary"; size: "m"; text: "Cancel"; onClicked: renameDialog.close() }
                Btn { kind: "primary"; size: "m"; text: "Rename"; onClicked: { triage.renameCategory(renameDialog.cat.id, rn.text); renameDialog.close() } }
            }
        }
    }
}
