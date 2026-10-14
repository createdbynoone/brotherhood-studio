import QtQuick
import QtQuick.Dialogs

// Image inputs. mode "single": one big preview that a new drop replaces. mode "multi": thumbnail strip + add tile.
Item {
    id: root
    property var files: []
    property string mode: "multi"
    property string placeholder: "Drop an image"
    signal dropped(var urls)
    signal removed(int index)

    readonly property bool hasFiles: files.length > 0
    implicitHeight: mode === "single" ? (hasFiles ? 220 : 120) : (hasFiles ? flow.implicitHeight : 96)

    FileDialog {
        id: picker
        title: "Choose images"
        fileMode: root.mode === "single" ? FileDialog.OpenFile : FileDialog.OpenFiles
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp *.heic *.heif *.tif *.tiff *.gif *.bmp *.avif)"]
        onAccepted: root.dropped(selectedFiles)
    }

    // Empty state / drop target
    Rectangle {
        id: empty
        anchors.fill: parent
        visible: !root.hasFiles
        radius: Theme.rCard
        color: drop.containsDrag ? Theme.accentSoft : emptyHover.hovered ? Theme.raised : "transparent"
        border.color: drop.containsDrag ? Theme.alpha(Theme.accent, 0.7) : Theme.hairlineStrong
        Behavior on color { ColorAnimation { duration: Theme.fast } }
        Column {
            anchors.centerIn: parent
            spacing: 8
            Icon { name: "image"; size: 22; color: drop.containsDrag ? Theme.accent : Theme.textMuted; anchors.horizontalCenter: parent.horizontalCenter }
            Txt { text: root.placeholder; variant: "small"; dim: true; anchors.horizontalCenter: parent.horizontalCenter }
            Txt { text: "or click to browse"; variant: "caption"; muted: true; anchors.horizontalCenter: parent.horizontalCenter }
        }
        HoverHandler { id: emptyHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: picker.open() }
    }

    // Single: large preview
    Item {
        anchors.fill: parent
        visible: root.mode === "single" && root.hasFiles
        Rectangle {
            id: frame
            anchors.fill: parent
            radius: Theme.rCard
            color: Theme.raised
            border.color: drop.containsDrag ? Theme.alpha(Theme.accent, 0.7) : Theme.hairline
            clip: true
            Image {
                anchors.fill: parent
                anchors.margins: 1
                source: root.files.length > 0 && root.mode === "single" ? "file://" + root.files[0] : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                sourceSize.height: 640
                smooth: true
            }
        }
        Row {
            anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 8
            spacing: 4
            Btn { kind: "secondary"; size: "xs"; icon: "arrow-clockwise"; iconOnly: true; tip: "Replace"; onClicked: picker.open() }
            Btn { kind: "secondary"; size: "xs"; icon: "x"; iconOnly: true; tip: "Remove"; onClicked: root.removed(0) }
        }
    }

    // Multi: wrapping thumbnails
    Flow {
        id: flow
        anchors.left: parent.left; anchors.right: parent.right
        visible: root.mode === "multi" && root.hasFiles
        spacing: 8
        Repeater {
            model: root.mode === "multi" ? root.files : []
            delegate: Item {
                id: thumb
                required property string modelData
                required property int index
                width: 84; height: 104
                Rectangle {
                    anchors.fill: parent
                    radius: Theme.rControl
                    color: Theme.raised
                    border.color: th.hovered ? Theme.hairlineStrong : Theme.hairline
                    clip: true
                    Image {
                        anchors.fill: parent; anchors.margins: 1
                        source: "file://" + thumb.modelData
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        sourceSize.height: 260
                    }
                }
                Btn {
                    visible: th.hovered
                    anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 4
                    kind: "secondary"; size: "xs"; icon: "x"; iconOnly: true
                    onClicked: root.removed(thumb.index)
                }
                HoverHandler { id: th }
            }
        }
        Rectangle {
            width: 84; height: 104
            radius: Theme.rControl
            color: addHover.hovered ? Theme.raised : "transparent"
            border.color: drop.containsDrag ? Theme.alpha(Theme.accent, 0.7) : Theme.hairlineStrong
            Icon { anchors.centerIn: parent; name: "plus"; size: 18; color: Theme.textDim }
            HoverHandler { id: addHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: picker.open() }
        }
    }

    DropArea {
        id: drop
        anchors.fill: parent
        onDropped: drop => { if (drop.hasUrls) root.dropped(drop.urls) }
    }
}
