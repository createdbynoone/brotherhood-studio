import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

// One library entry. The corners are cut by a bg-coloured mask drawn over the image (no offscreen layer
// per card), so a few hundred cards stay cheap. Dimming applies to the image only, never to the mask.
Item {
    id: root
    property var entry
    property bool hovered: hover.hovered
    signal contextRequested()

    readonly property bool sel: entry ? entry.selected : false
    readonly property bool prim: entry ? entry.primary : false
    readonly property int status: entry ? entry.status : 0

    scale: entry && entry.isNew ? 1.035 : 1
    Behavior on scale { SpringAnimation { spring: 3.2; damping: 0.3; epsilon: 0.002 } }

    Rectangle { anchors.fill: parent; radius: Theme.rCard; color: Theme.raised }

    Image {
        id: img
        anchors.fill: parent
        source: root.entry ? root.entry.thumbSource : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        smooth: true
        opacity: status === 3 ? 0.4 : status === 4 ? 0.6 : 1
        Behavior on opacity { NumberAnimation { duration: Theme.base } }
        // Archived / discarded entries read as "set aside": desaturated on top of the dimming
        layer.enabled: status === 3
        layer.effect: MultiEffect { saturation: -1.0 }
    }
    Icon {   // undecodable / missing file
        visible: img.status === Image.Error
        anchors.centerIn: parent
        name: "image"; size: 20; color: Theme.textMuted
    }

    // Round the corners
    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillRule: ShapePath.OddEvenFill
            fillColor: Theme.bg
            strokeColor: "transparent"
            PathRectangle { x: 0; y: 0; width: root.width; height: root.height }
            PathRectangle { x: 0; y: 0; width: root.width; height: root.height; radius: Theme.rCard }
        }
    }

    // Video marker
    Rectangle {
        visible: root.entry && root.entry.isVideo
        anchors.centerIn: parent
        width: 36; height: 36; radius: 18
        color: Qt.rgba(0, 0, 0, 0.55)
        Icon { anchors.centerIn: parent; name: "play"; weight: "fill"; size: 14; color: "#FFFFFF" }
    }

    StatusBadge {
        visible: root.status !== 0
        status: root.status
        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 8
    }

    Rectangle {
        visible: root.entry && root.entry.missing
        anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 8
        height: 20; width: missingText.implicitWidth + 12; radius: Theme.rControl - 3
        color: Qt.rgba(0, 0, 0, 0.7)
        Txt { id: missingText; anchors.centerIn: parent; text: "Missing"; variant: "caption"; dim: true }
    }

    // Hover strip: note marker + category chips
    Rectangle {
        id: strip
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 38
        opacity: root.hovered && root.entry && (root.entry.chips.length > 0 || root.entry.note.length > 0) ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.fast } }
        gradient: Gradient {
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.78) }
        }
        Row {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.margins: 8
            spacing: 4
            layoutDirection: Qt.RightToLeft
            Txt { visible: root.entry && root.entry.chipExtra > 0; text: "+" + (root.entry ? root.entry.chipExtra : 0); variant: "caption"; dim: true; anchors.verticalCenter: parent.verticalCenter }
            Repeater {
                model: root.entry ? root.entry.chips : []
                delegate: Rectangle {
                    required property string modelData
                    height: 18; width: chipText.implicitWidth + 10; radius: 5
                    color: Qt.rgba(0, 0, 0, 0.6)
                    Txt { id: chipText; anchors.centerIn: parent; text: modelData; variant: "caption"; font.pixelSize: 11 }
                }
            }
            Rectangle { visible: root.entry && root.entry.note.length > 0; width: 6; height: 6; radius: 3; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
        }
    }

    // Selection ring (2 px) and the "primary" halo
    Rectangle {
        anchors.fill: parent
        radius: Theme.rCard
        color: "transparent"
        border.width: root.sel || (root.entry && root.entry.isNew) ? 2 : 1
        border.color: root.sel || (root.entry && root.entry.isNew) ? Theme.accent : root.hovered ? Theme.hairlineStrong : Theme.hairline
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }
    }
    Rectangle {
        anchors.fill: parent; anchors.margins: -4
        radius: Theme.rCard + 3
        color: "transparent"
        border.width: 1
        border.color: Theme.accent
        opacity: root.prim ? 0.4 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.fast } }
    }

    HoverHandler { id: hover }
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: {
            const m = Qt.application.keyboardModifiers
            triage.click(root.entry.path, (m & Qt.ControlModifier) !== 0, (m & Qt.ShiftModifier) !== 0)
        }
        onDoubleTapped: triage.openFocus(root.entry.path)
    }
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: root.contextRequested() }

    // Native drag-out: hands the ORIGINAL file to Finder / BMP / any drop target
    Item {
        id: dragProxy
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: root.entry ? ({ "text/uri-list": "file://" + encodeURI(root.entry.path) }) : ({})
        Drag.active: dragHandler.active
        // the drag shows the thumbnail itself instead of the file URL as text
        Drag.imageSource: root.entry ? root.entry.thumbSource : ""
        Drag.hotSpot: Qt.point(Math.min(root.width, 200) / 2, Math.min(root.height, 200) / 2)
    }
    DragHandler { id: dragHandler; target: null; acceptedButtons: Qt.LeftButton; dragThreshold: 8 }
}
