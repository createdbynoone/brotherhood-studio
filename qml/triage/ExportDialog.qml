import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

// Cover-crop (no bands) at the two Brotherhood sizes, JPEG 95 %.
Popup {
    id: root
    property string path: ""
    property var chosen: ["box", "vertical"]

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    modal: true
    width: 420
    padding: 22
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle { color: Theme.scrim }
    background: Rectangle { radius: Theme.rPanel; color: Theme.surface; border.color: Theme.hairlineStrong }

    function openFor(p) { path = p; open() }
    function toggle(key) {
        const c = chosen.slice()
        const i = c.indexOf(key)
        if (i >= 0) c.splice(i, 1); else c.push(key)
        chosen = c
    }

    FolderDialog {
        id: folder
        title: "Choose the destination folder"
        onAccepted: { triage.exportEntry(root.path, root.chosen, selectedFolder); root.close() }
    }

    contentItem: Column {
        spacing: 16
        Row {
            width: parent.width
            spacing: 10
            Icon { name: "export"; size: 18; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
            Txt { text: "Export"; variant: "title"; anchors.verticalCenter: parent.verticalCenter }
        }
        Txt {
            width: parent.width
            text: root.path.split("/").pop()
            variant: "mono"; dim: true; elide: Text.ElideMiddle
        }
        Column {
            width: parent.width
            spacing: 8
            Repeater {
                model: [{ key: "box", name: "Box", label: "1080 × 1080", w: 14, h: 14 }, { key: "vertical", name: "Vertical", label: "1080 × 1920", w: 9, h: 16 }]
                delegate: Rectangle {
                    required property var modelData
                    readonly property bool on: root.chosen.indexOf(modelData.key) >= 0
                    width: parent.width; height: 48
                    radius: Theme.rCard
                    color: on ? Theme.accentSoft : rowHover.hovered ? Theme.raised : "transparent"
                    border.color: on ? Theme.alpha(Theme.accent, 0.6) : Theme.hairlineStrong
                    Behavior on color { ColorAnimation { duration: Theme.fast } }
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 12
                        Rectangle {
                            width: modelData.w + 4; height: modelData.h + 4; radius: 3
                            color: "transparent"; border.color: on ? Theme.accent : Theme.textMuted; border.width: 1.5
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Txt { text: modelData.name; variant: "strong"; anchors.verticalCenter: parent.verticalCenter }
                        Txt { text: modelData.label; variant: "mono"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Icon {
                        anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter
                        name: "check-circle"; weight: on ? "fill" : "regular"; size: 18
                        color: on ? Theme.accent : Theme.textMuted
                    }
                    HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: root.toggle(modelData.key) }
                }
            }
        }
        Row {
            anchors.right: parent.right
            spacing: 8
            Btn { kind: "secondary"; size: "m"; text: "Cancel"; onClicked: root.close() }
            Btn {
                kind: "primary"; size: "m"
                text: "Export " + root.chosen.length
                enabled: root.chosen.length > 0
                onClicked: folder.open()
            }
        }
    }
}
