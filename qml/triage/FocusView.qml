import QtQuick
import QtQuick.Controls.Basic
import QtMultimedia

// One item at a time, keyboard first. Images zoom (wheel, pinch, double click) and pan; videos play.
FocusScope {
    id: root
    readonly property var entry: triage.focusEntry
    property real zoom: 1
    property point pan: Qt.point(0, 0)
    property point panStart: Qt.point(0, 0)
    signal exportRequested(string path)
    property Item inspectorItem: null

    function resetView() { zoom = 1; pan = Qt.point(0, 0); panStart = Qt.point(0, 0) }
    onEntryChanged: resetView()
    Component.onCompleted: forceActiveFocus()

    Rectangle { anchors.fill: parent; color: Theme.bg }

    // Top bar
    Rectangle {
        id: top
        width: parent.width; height: 44
        color: Theme.alpha(Theme.surface, 0.7)
        Hairline { anchors.bottom: parent.bottom }
        Row {
            anchors.left: parent.left; anchors.leftMargin: Theme.s12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Btn { kind: "ghost"; size: "s"; icon: "caret-left"; iconWeight: "bold"; text: "Grid"; hint: "Esc"; onClicked: triage.closeFocus() }
            StatusBadge { visible: root.entry !== null; status: root.entry ? root.entry.status : 0; size: 18; anchors.verticalCenter: parent.verticalCenter }
            Txt {
                text: root.entry ? root.entry.filename : ""
                variant: "mono"; dim: true
                elide: Text.ElideMiddle
                width: Math.min(implicitWidth, Math.max(120, top.width - 560))
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Row {
            anchors.right: parent.right; anchors.rightMargin: Theme.s12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            Txt { text: (triage.focusIndex + 1) + " of " + triage.focusCount; variant: "caption"; muted: true; rightPadding: 8; anchors.verticalCenter: parent.verticalCenter }
            Btn { kind: "ghost"; size: "s"; icon: "caret-left"; iconWeight: "bold"; tip: "Previous"; hint: "←"; enabled: triage.focusIndex > 0; onClicked: triage.focusNavigate(-1) }
            Btn { kind: "ghost"; size: "s"; icon: "caret-right"; iconWeight: "bold"; tip: "Next"; hint: "→"; enabled: triage.focusIndex < triage.focusCount - 1; onClicked: triage.focusNavigate(1) }
            Btn { visible: root.entry && !root.entry.isVideo; kind: "ghost"; size: "s"; icon: "export"; text: "Export"; hint: "E"; onClicked: root.exportRequested(root.entry.path) }
            Btn { kind: "ghost"; size: "s"; icon: "sidebar"; tip: "Inspector"; hint: "I"; onClicked: triage.inspectorOpen = !triage.inspectorOpen }
        }
    }

    // Stage
    Item {
        id: stage
        y: top.height
        width: parent.width
        height: parent.height - top.height - bottom.height
        clip: true

        // Image
        Item {
            id: imageLayer
            visible: root.entry && !root.entry.isVideo
            width: stage.width; height: stage.height
            scale: root.zoom
            x: root.pan.x; y: root.pan.y
            Image {
                id: big
                anchors.fill: parent; anchors.margins: 16
                source: root.entry && !root.entry.isVideo ? "image://full/" + encodeURIComponent(root.entry.path) : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                mipmap: true
                cache: false
                opacity: status === Image.Ready ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.base } }
            }
        }
        Pulse { visible: root.entry && !root.entry.isVideo && big.status === Image.Loading; running: visible; anchors.centerIn: parent }

        WheelHandler {
            enabled: root.entry && !root.entry.isVideo
            onWheel: event => {
                const f = event.angleDelta.y > 0 ? 1.12 : 1 / 1.12
                root.zoom = Math.min(8, Math.max(0.25, root.zoom * f))
                if (root.zoom <= 1) root.pan = Qt.point(0, 0)
                root.panStart = root.pan
            }
        }
        PinchHandler {
            id: pinch
            target: null
            enabled: root.entry && !root.entry.isVideo
            property real startZoom: 1
            onActiveChanged: if (active) startZoom = root.zoom
            onActiveScaleChanged: if (active) root.zoom = Math.min(8, Math.max(0.25, startZoom * activeScale))
        }
        DragHandler {
            target: null
            enabled: root.zoom > 1
            onTranslationChanged: root.pan = Qt.point(root.panStart.x + translation.x, root.panStart.y + translation.y)
            onActiveChanged: if (!active) root.panStart = root.pan
        }
        TapHandler {
            enabled: root.entry && !root.entry.isVideo
            onDoubleTapped: { root.zoom = root.zoom > 1 ? 1 : 2; root.pan = Qt.point(0, 0); root.panStart = root.pan }
        }

        // Video
        Item {
            id: videoLayer
            visible: root.entry && root.entry.isVideo
            anchors.fill: parent
            anchors.margins: 16
            MediaPlayer {
                id: player
                source: root.entry && root.entry.isVideo ? "file://" + encodeURI(root.entry.path) : ""
                videoOutput: vo
                audioOutput: AudioOutput {}
                loops: MediaPlayer.Infinite
                onSourceChanged: if (source.toString().length) play()
            }
            VideoOutput { id: vo; anchors.fill: parent; fillMode: VideoOutput.PreserveAspectFit }
            TapHandler { onTapped: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play() }
            Column {
                visible: player.error !== MediaPlayer.NoError
                anchors.centerIn: parent
                spacing: 10
                Icon { name: "film-strip"; size: 24; color: Theme.textMuted; anchors.horizontalCenter: parent.horizontalCenter }
                Txt { text: "Can't decode this video here"; dim: true; anchors.horizontalCenter: parent.horizontalCenter }
                Btn { kind: "secondary"; size: "s"; text: "Reveal in Finder"; anchors.horizontalCenter: parent.horizontalCenter; onClicked: triage.reveal(root.entry.path) }
            }
        }
        Component.onDestruction: { player.stop(); player.source = "" }
    }

    // Bottom bar
    Rectangle {
        id: bottom
        anchors.bottom: parent.bottom
        width: parent.width; height: 52
        color: Theme.alpha(Theme.surface, 0.7)
        Hairline { anchors.top: parent.top }
        Row {
            anchors.left: parent.left; anchors.leftMargin: Theme.s12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            Repeater {
                model: Statuses.pillOrder.filter(s => s !== 0).concat([0])
                delegate: StatusPill {
                    required property int modelData
                    status: modelData
                    active: root.entry && root.entry.status === modelData
                    onClicked: triage.focusStatus(modelData)
                }
            }
        }
        Row {
            anchors.right: parent.right; anchors.rightMargin: Theme.s16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14
            Txt { visible: root.zoom !== 1; text: Math.round(root.zoom * 100) + "%"; variant: "caption"; muted: true }
            Txt { text: "← →  navigate     K M D A U  status     N  note     E  export"; variant: "caption"; muted: true }
        }
    }

    Keys.onPressed: event => {
        const other = event.modifiers & ~Qt.ShiftModifier & ~Qt.KeypadModifier
        if (other) return
        switch (event.key) {
        case Qt.Key_Left: triage.focusNavigate(-1); break
        case Qt.Key_Right: case Qt.Key_Space: triage.focusNavigate(1); break
        case Qt.Key_Escape: triage.closeFocus(); break
        case Qt.Key_K: triage.focusStatus(1); break
        case Qt.Key_M: triage.focusStatus(2); break
        case Qt.Key_D: triage.focusStatus(3); break
        case Qt.Key_U: triage.focusStatus(0); break
        case Qt.Key_A: triage.focusStatus(4); break
        case Qt.Key_N: triage.focusNote = true; triage.inspectorOpen = true; break
        case Qt.Key_R: if (root.entry) triage.reveal(root.entry.path); break
        case Qt.Key_E: if (root.entry && !root.entry.isVideo) root.exportRequested(root.entry.path); break
        case Qt.Key_I: triage.inspectorOpen = !triage.inspectorOpen; break
        default: return
        }
        event.accepted = true
    }
}
