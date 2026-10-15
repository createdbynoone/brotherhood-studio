import QtQuick
import QtQuick.Effects

// Floating inspector: a rounded glass card inset from the edges that slides in over the content
// (it never reflows the grid), plus a round toggle that travels with it.
Item {
    id: root
    property Item blurSource: null          // the item whose pixels are blurred behind the panel
    property var entry: null
    property int topInset: 0         // Focus view: keep the panel below its top bar and above its status bar
    property int bottomInset: 0
    readonly property alias panel: panel
    readonly property int panelWidth: 300
    readonly property int inset: 12
    readonly property bool open: triage.inspectorOpen
    // One progress value drives panel, fade and arrow: when the pane itself resizes (layout switch,
    // window resize) they follow instantly instead of chasing the new edge with their own animation.
    property real prog: open ? 1 : 0
    Behavior on prog { NumberAnimation { duration: Theme.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout } }

    Item {
        id: panel
        width: root.panelWidth
        height: root.height - root.inset * 2 - root.topInset - root.bottomInset
        y: root.inset + root.topInset
        x: root.width + 24 - (root.panelWidth + root.inset + 24) * root.prog
        opacity: Math.min(1, root.prog * 1.6)
        visible: root.prog > 0.01

        // Glass: blurred copy of what is behind the panel + a tint that keeps text readable over any photo
        ShaderEffectSource {
            id: backdrop
            anchors.fill: parent
            sourceItem: root.blurSource
            sourceRect: root.blurSource ? Qt.rect(panel.x, panel.y, panel.width, panel.height) : Qt.rect(0, 0, 0, 0)
            visible: false
            live: root.prog > 0.01
        }
        MultiEffect {
            anchors.fill: parent
            source: backdrop
            autoPaddingEnabled: false
            blurEnabled: true
            blurMax: 48
            blur: 1.0
            maskEnabled: true
            maskSource: maskShape
            maskThresholdMin: 0.5
            maskSpreadAtMin: 1.0
        }
        Rectangle {
            id: maskShape
            anchors.fill: parent
            radius: Theme.rPanel
            layer.enabled: true
            visible: false
        }
        Rectangle {
            anchors.fill: parent
            radius: Theme.rPanel
            color: Theme.alpha(Theme.surface, 0.72)
            border.color: Theme.hairlineStrong
        }
        Rectangle {   // top highlight: the glass edge
            anchors.fill: parent
            radius: Theme.rPanel
            color: "transparent"
            border.color: Qt.rgba(1, 1, 1, 0.06)
            anchors.margins: 1
        }
        // The panel floats over the grid / focus view: every click, drag, scroll and hover that lands on it stays on it.
        // (Without this the cards or the zoomable image underneath also received them.)
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; hoverEnabled: true; onWheel: w => w.accepted = true }
        InspectorPanel { anchors.fill: parent; entry: root.entry }
    }

    // Round toggle, vertically centred, rides along the panel's edge
    Rectangle {
        id: toggle
        width: 40; height: 40; radius: 20
        y: root.topInset + (root.height - root.topInset - root.bottomInset - height) / 2
        x: Math.min(panel.x - width - 12, root.width - width - 16)     // follows the (animated) panel edge
        color: toggleHover.hovered ? Theme.elevated : Theme.alpha(Theme.surface, 0.9)
        border.color: Theme.hairlineStrong
        Icon {
            anchors.centerIn: parent
            name: "caret-left"; weight: "bold"; size: 13
            color: toggleHover.hovered ? Theme.text : Theme.textDim
            rotation: 180 * root.prog
        }
        HoverHandler { id: toggleHover; cursorShape: Qt.PointingHandCursor }
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onClicked: triage.inspectorOpen = !triage.inspectorOpen; onWheel: w => w.accepted = true }
    }
}
