import QtQuick
import QtQuick.Controls.Basic
import QtMultimedia

// One item at a time, keyboard first. Images zoom (wheel, pinch, double click) and pan; videos play.
FocusScope {
    id: root
    readonly property var entry: triage.focusEntry
    property real zoom: 1
    property point pan: Qt.point(0, 0)
    property Item inspectorItem: null
    function overInspector(sx, sy) {
        if (!inspectorItem || !inspectorItem.visible) return false
        const p = inspectorItem.mapFromItem(null, sx, sy)
        return p.x >= 0 && p.x <= inspectorItem.width && p.y >= 0 && p.y <= inspectorItem.height
    }

    function resetView() { zoom = 1; pan = Qt.point(0, 0) }
    function setPan(x, y) {   // keep the picture from being thrown off-screen
        const lx = stage.width * (zoom - 1) / 2 + 80, ly = stage.height * (zoom - 1) / 2 + 80
        pan = Qt.point(Math.max(-lx, Math.min(lx, x)), Math.max(-ly, Math.min(ly, y)))
    }
    function panBy(dx, dy) { setPan(pan.x + dx, pan.y + dy) }
    function zoomTo(nz, sx, sy) {   // zoom keeping the point under (sx, sy) fixed; layer scales about the stage centre
        nz = Math.max(1, Math.min(10, nz))
        const ox = stage.width / 2, oy = stage.height / 2, k = nz / zoom
        const px = nz <= 1.001 ? 0 : sx - ox - k * (sx - ox - pan.x)
        const py = nz <= 1.001 ? 0 : sy - oy - k * (sy - oy - pan.y)
        zoom = nz
        setPan(px, py)
    }
    function zoomAt(sx, sy, f) { zoomTo(zoom * f, sx, sy) }
    // Reset only when the picture really changes, never when the same entry is merely re-announced (status, note)
    property string shownPath: ""
    onEntryChanged: { const p = entry ? entry.path : ""; if (p !== shownPath) { shownPath = p; resetView() } }
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
                anchors.fill: parent; anchors.margins: 28
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

        // Zoom / pan. Mouse wheel and Cmd+scroll zoom around the cursor; trackpad scroll (pixelDelta) pans;
        // pinch zooms. macOS adds tiny magnify events to a two-finger scroll, so a pinch only counts once it
        // clearly differs from 1 and is ignored while scrolling.
        property real lastScrollMs: 0
        WheelHandler {
            id: wheel
            enabled: root.entry && !root.entry.isVideo
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: event => {
                stage.lastScrollMs = Date.now()
                const trackpad = event.pixelDelta.x !== 0 || event.pixelDelta.y !== 0
                const cmd = (event.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) !== 0
                if (trackpad && !cmd) {
                    if (root.zoom > 1) root.panBy(event.pixelDelta.x, event.pixelDelta.y)
                } else {
                    const d = trackpad ? event.pixelDelta.y * 0.01 : event.angleDelta.y * 0.0015
                    root.zoomAt(point.position.x, point.position.y, Math.exp(Math.max(-0.25, Math.min(0.25, d))))
                }
            }
        }
        // Pinch comes from AppKit as magnification deltas (Mac::installMagnify): only a deliberate gesture counts,
        // never scroll noise, a click, or resting fingers.
        property real pinchSum: 0
        property bool pinchOn: false
        Connections {
            target: studio
            enabled: root.visible && root.entry && !root.entry.isVideo
            function onMagnified(x, y, mag, phase) {
                if (root.overInspector(x, y)) return          // pinching over the floating panel is not for the image behind it
                if (phase & 1) { stage.pinchSum = 0; stage.pinchOn = false }
                if ((phase & 24) && !(phase & 6)) { stage.pinchOn = false; stage.pinchSum = 0; return }
                if (clickGuard.running || (!stage.pinchOn && Date.now() - stage.lastScrollMs < 200)) return
                if (!stage.pinchOn) {
                    stage.pinchSum += mag
                    if (Math.abs(stage.pinchSum) < 0.04) return
                    stage.pinchOn = true; mag = stage.pinchSum
                }
                const p = stage.mapFromItem(null, x, y)
                root.zoomAt(p.x, p.y, Math.max(0.8, Math.min(1.25, 1 + mag)))
            }
        }
        Timer { id: clickGuard; interval: 250 }   // a press or release just happened: not a pinch
        DragHandler {
            target: null
            enabled: root.zoom > 1 && root.entry && !root.entry.isVideo
            acceptedDevices: PointerDevice.Mouse
            property point origin
            onActiveChanged: if (active) origin = root.pan
            onTranslationChanged: if (active) root.setPan(origin.x + translation.x, origin.y + translation.y)
        }
        TapHandler {
            enabled: root.entry && !root.entry.isVideo
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onPressedChanged: clickGuard.restart()
            // a single click never touches the zoom: only a double click resets it (or zooms in from 1x)
            onDoubleTapped: root.zoomTo(root.zoom > 1.01 ? 1 : 2.5, point.position.x, point.position.y)
        }

        // Video
        Item {
            id: videoLayer
            visible: root.entry && root.entry.isVideo
            anchors.fill: parent
            anchors.margins: 16
            MediaPlayer {
                id: player
                source: root.entry && root.entry.isVideo ? Theme.fileUrl(root.entry.path) : ""
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
            Txt { text: "← →  navigate     K M D A U  status     N  note"; variant: "caption"; muted: true }
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
        case Qt.Key_I: triage.inspectorOpen = !triage.inspectorOpen; break
        default: return
        }
        event.accepted = true
    }
}
