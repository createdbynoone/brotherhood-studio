import QtQuick
import QtQuick.Controls.Basic

// Every prompt ever generated (shared with BMP Electron / Native). Stars are the ones that were fired.
Popup {
    id: root
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(820, Overlay.overlay ? Overlay.overlay.width - 80 : 820)
    height: Math.min(560, Overlay.overlay ? Overlay.overlay.height - 80 : 560)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var entries: []
    property string query: ""
    property var shownEntries: {
        const q = query.trim().toLowerCase()
        return q.length === 0 ? entries : entries.filter(e => e.description.toLowerCase().includes(q) || e.prompt.toLowerCase().includes(q))
    }
    property int selectedIndex: 0
    readonly property var current: shownEntries.length > selectedIndex ? shownEntries[selectedIndex] : null

    onAboutToShow: { entries = prompt.history(); query = ""; search.text = ""; selectedIndex = 0 }

    Overlay.modal: Rectangle { color: Theme.scrim }
    background: Rectangle { radius: Theme.rPanel; color: Theme.surface; border.color: Theme.hairlineStrong }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.base } }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.fast } }

    contentItem: Column {
        Item {
            width: parent.width; height: 56
            Txt { text: "Prompt history"; variant: "title"; anchors.left: parent.left; anchors.leftMargin: 18; anchors.verticalCenter: parent.verticalCenter }
            Field {
                id: search
                anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter
                width: 240
                icon: "magnifying-glass"; placeholder: "Search"; clearable: true
                onEdited: { root.query = text; root.selectedIndex = 0 }
            }
        }
        Hairline {}
        Row {
            width: parent.width; height: parent.height - 56 - 57 - 2
            ListView {
                id: list
                width: 300; height: parent.height
                clip: true
                model: root.shownEntries
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: StudioScrollBar {}
                delegate: Item {
                    required property var modelData
                    required property int index
                    width: list.width; height: 56
                    Rectangle {
                        anchors.fill: parent; anchors.margins: 4
                        radius: Theme.rControl
                        color: index === root.selectedIndex ? Theme.elevated : rowHover.hovered ? Theme.raised : "transparent"
                    }
                    Column {
                        anchors.left: parent.left; anchors.leftMargin: 16; anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4
                        Row {
                            spacing: 6
                            Icon { visible: modelData.fired; name: "star"; weight: "fill"; size: 11; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                            Txt { text: modelData.description; variant: "strong"; elide: Text.ElideRight; width: 250 - (modelData.fired ? 17 : 0); font.pixelSize: 13 }
                        }
                        Txt { text: modelData.date + (modelData.aspectRatio ? "  ·  " + modelData.aspectRatio : ""); variant: "caption"; muted: true }
                    }
                    HoverHandler { id: rowHover }
                    TapHandler { onTapped: root.selectedIndex = index }
                }
            }
            Hairline { vertical: true; height: parent.height }
            Item {
                width: parent.width - 301; height: parent.height
                Flickable {
                    anchors.fill: parent
                    contentWidth: width
                    contentHeight: detail.implicitHeight + 36
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: StudioScrollBar {}
                    Column {
                        id: detail
                        x: 18; y: 18
                        width: parent.width - 36
                        spacing: 12
                        visible: root.current !== null
                        Txt { text: root.current ? root.current.description : ""; variant: "strong"; width: parent.width; wrapMode: Text.Wrap }
                        Reading {
                            width: parent.width
                            plain: root.current ? root.current.prompt : ""
                            font.pixelSize: 14
                        }
                    }
                }
                Txt { visible: root.current === null; anchors.centerIn: parent; text: root.entries.length === 0 ? "No prompts yet" : "Select a prompt"; dim: true }
            }
        }
        Hairline {}
        Item {
            width: parent.width; height: 56
            Txt { text: root.shownEntries.length + " prompts"; variant: "caption"; muted: true; anchors.left: parent.left; anchors.leftMargin: 18; anchors.verticalCenter: parent.verticalCenter }
            Row {
                anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Btn { kind: "secondary"; size: "m"; text: "Cancel"; onClicked: root.close() }
                Btn {
                    kind: "primary"; size: "m"; text: "Use prompt"
                    enabled: root.current !== null
                    onClicked: { prompt.usePrompt(root.current.prompt); root.close() }
                }
            }
        }
    }
}
