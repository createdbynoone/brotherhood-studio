import QtQuick
import QtQuick.Controls.Basic

// Virtualized grid. Rows come from C++ (group headers, product headers, chunks of `cols` cards) so
// category sections scroll as cheaply as a flat grid. Geometry is plain arithmetic, which keeps the
// rectangular selection exact.
FocusScope {
    id: root
    readonly property var gallery: triage.gallery
    property rect marquee: Qt.rect(0, 0, 0, 0)
    property bool marqueeOn: false
    property var marqueeBase: []
    property bool pressInGap: false
    property Item inspectorItem: null      // clicks inside the floating inspector never start a marquee
    // While the pane is gliding open or closed its width changes every frame. Chasing that would shuffle
    // the cards continuously, so the arrangement is held and the gallery reflows once, at the end,
    // toward a stable target.
    property bool freeze: false
    signal exportRequested(string path)

    Component.onCompleted: { gallery.setViewport(width, triage.gridSize); gallery.setScroll(flick.contentY, flick.height); forceActiveFocus() }
    onWidthChanged: if (!freeze) gallery.setViewport(width, triage.gridSize)
    onFreezeChanged: if (!freeze) gallery.setViewport(width, triage.gridSize)
    Connections { target: triage; function onGridSizeChanged() { root.gallery.setViewport(root.width, triage.gridSize) } }
    Connections { target: root.gallery; function onScrollRequest(y) { flick.contentY = Math.min(y, Math.max(0, flick.contentHeight - flick.height)) } }

    Connections {
        target: triage
        function onScrollToPath(path) {
            const y = root.gallery.yOf(path)
            if (y < 0) return
            const h = root.gallery.cardHeight
            let target = flick.contentY
            if (y < flick.contentY + 8) target = Math.max(0, y - 16)
            else if (y + h > flick.contentY + flick.height - 8) target = y + h - flick.height + 24
            if (target !== flick.contentY) { scrollGlide.to = target; scrollGlide.restart() }
        }
    }
    NumberAnimation { id: scrollGlide; target: flick; property: "contentY"; duration: Theme.ms(260); easing.type: Easing.OutCubic }

    // ── Hit testing: cards are direct children of the flickable's content item ──
    function cardItems() {
        const out = []
        for (const c of flick.contentItem.children) if (c.isCard && c.card) out.push(c)
        return out
    }
    function cardAt(x, y) {
        const p = flick.contentItem.mapFromItem(root, x, y)
        for (const c of cardItems()) if (p.x >= c.x && p.x <= c.x + c.width && p.y >= c.y && p.y <= c.y + c.height) return c
        return null
    }
    function overInspector(x, y) {
        if (!inspectorItem || !inspectorItem.visible) return false
        const p = inspectorItem.mapFromItem(root, x, y)
        return p.x >= -8 && p.x <= inspectorItem.width + 8 && p.y >= 0 && p.y <= inspectorItem.height
    }
    function cardsIn(rect) {
        const out = []
        for (const c of cardItems()) {
            const tl = c.mapToItem(root, 0, 0)
            if (tl.x < rect.x + rect.width && tl.x + c.width > rect.x && tl.y < rect.y + rect.height && tl.y + c.height > rect.y)
                out.push(c.entry.path)
        }
        return out
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: root.gallery.totalHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        acceptedButtons: Qt.NoButton          // wheel / trackpad scroll only: mouse drags belong to the marquee
        ScrollBar.vertical: StudioScrollBar {}
        onContentYChanged: root.gallery.setScroll(contentY, height)
        onHeightChanged: root.gallery.setScroll(contentY, height)

        Repeater {
            model: root.gallery
            delegate: Item {
                id: node
                required property int index
                required property int kind
                required property var entry
                required property real nx
                required property real ny
                required property real nw
                required property real nh
                required property int stagger
                required property string title
                required property int count
                required property int subCount
                required property string groupId
                required property bool collapsed
                readonly property bool isCard: kind === 0
                readonly property Item card: cardLoader.item

                // Geometry comes from C++. Cards animate to a new position only while the gallery is
                // re-flowing (columns, grid size, filter, sort): a gentle wave, one column after another.
                x: nx; y: ny; width: nw; height: nh
                readonly property int wave: Math.min(stagger, 9) * 22
                Behavior on x { enabled: root.gallery.reflowing && !Theme.reduceMotion; SequentialAnimation { PauseAnimation { duration: node.wave } NumberAnimation { duration: 560; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout } } }
                Behavior on y { enabled: root.gallery.reflowing && !Theme.reduceMotion; SequentialAnimation { PauseAnimation { duration: node.wave } NumberAnimation { duration: 560; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout } } }
                Behavior on width { enabled: root.gallery.reflowing && !Theme.reduceMotion; SequentialAnimation { PauseAnimation { duration: node.wave } NumberAnimation { duration: 560; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout } } }
                Behavior on height { enabled: root.gallery.reflowing && !Theme.reduceMotion; SequentialAnimation { PauseAnimation { duration: node.wave } NumberAnimation { duration: 560; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout } } }

                // Newcomers (filter / collapse / new generation) ease in instead of popping. The fade is a
                // background-coloured veil over the card, never the card's own opacity: animating opacity on
                // a subtree that contains the rounded-corner shapes makes Qt dim the whole window.
                transformOrigin: Item.Center
                ParallelAnimation {
                    id: appear
                    NumberAnimation { target: veil; property: "opacity"; from: 1; to: 0; duration: 480; easing.type: Easing.OutCubic }
                    NumberAnimation { target: node; property: "scale"; from: 0.95; to: 1; duration: 560; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout }
                }
                Component.onCompleted: if (isCard && root.gallery.reflowing && !Theme.reduceMotion) appear.start()

                Loader {
                    id: cardLoader
                    anchors.fill: parent
                    active: node.isCard
                    sourceComponent: MediaCard {
                        entry: node.entry
                        onContextRequested: { ctx.target = node.entry; ctx.popup() }
                    }
                }

                Rectangle {
                    id: veil
                    anchors.fill: parent
                    radius: Theme.rCard
                    color: Theme.bg
                    opacity: 0
                    visible: opacity > 0.01
                    z: 20
                }

                // Category header
                Item {
                    visible: node.kind === 1
                    anchors.fill: parent
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 6
                        spacing: 10
                        Txt { text: node.title; variant: "overline"; font.pixelSize: 12; font.weight: Font.Bold; font.letterSpacing: 2; anchors.verticalCenter: parent.verticalCenter }
                        Txt { text: node.count; variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                        Txt { visible: node.subCount > 0; text: node.subCount + (node.subCount === 1 ? " product" : " products"); variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Icon {
                        anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter; anchors.verticalCenterOffset: 6
                        name: node.collapsed ? "caret-right" : "caret-down"; weight: "bold"; size: 12; color: Theme.textMuted
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: triage.toggleGroup(node.groupId) }
                }

                // Product header
                Item {
                    visible: node.kind === 2
                    anchors.fill: parent
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        Txt { text: node.title; variant: "strong"; font.pixelSize: 13; dim: true; anchors.verticalCenter: parent.verticalCenter }
                        Txt { text: node.count; variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }

        // Empty state
        Column {
            visible: triage.visibleCount === 0
            x: (flick.width - width) / 2
            y: Math.max(40, (flick.height - height) / 2)
            spacing: 14
            BrandMark { size: 36; tint: Theme.textMuted; anchors.horizontalCenter: parent.horizontalCenter }
            Txt {
                text: triage.totalCount > 0 ? "Nothing matches these filters" : "Drop images, videos or folders here, or use Import"
                dim: true; anchors.horizontalCenter: parent.horizontalCenter
            }
            Txt {
                visible: triage.totalCount === 0
                text: "BMP's output folder is scanned automatically"
                variant: "caption"; muted: true; anchors.horizontalCenter: parent.horizontalCenter
            }
            Btn {
                visible: triage.totalCount > 0
                kind: "secondary"; size: "s"; text: "Show all"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: { triage.filter = 0; triage.search = ""; triage.clearSelection() }
            }
        }
    }

    StudioMenu {
        id: ctx
        property var target: null
        readonly property var paths: target ? triage.contextTarget(target.path) : []
        readonly property bool many: paths.length > 1
        MenuItem { text: ctx.many ? "Open " + ctx.paths.length + " files" : "Open"; onTriggered: ctx.paths.forEach(p => triage.open(p)) }
        MenuItem { text: "Reveal in Finder"; onTriggered: triage.reveal(ctx.paths[0]) }
        MenuSeparator {}
        MenuItem { text: "Keep"; onTriggered: triage.setStatus(ctx.paths, 1) }
        MenuItem { text: "Maybe"; onTriggered: triage.setStatus(ctx.paths, 2) }
        MenuItem { text: "Discard"; onTriggered: triage.setStatus(ctx.paths, 3) }
        MenuItem { text: "Archive"; onTriggered: triage.toggleStatus(ctx.paths, 4) }
        MenuItem { text: "Mark unsorted"; onTriggered: triage.setStatus(ctx.paths, 0) }
        MenuSeparator {}
        MenuItem { text: "Use as reference in BMP"; enabled: !ctx.many; onTriggered: prompt.addRefs(["file://" + encodeURI(ctx.paths[0])]) }
        MenuItem { text: "Use as product in BMP"; onTriggered: prompt.addProducts(ctx.paths.map(p => "file://" + encodeURI(p))) }
        MenuSeparator {}
        MenuItem { text: "Export..."; enabled: !ctx.many && ctx.target && !ctx.target.isVideo; onTriggered: root.exportRequested(ctx.paths[0]) }
    }

    // ── Marquee ──────────────────────────────────────────────────────────
    TapHandler {
        id: gapTap
        acceptedButtons: Qt.LeftButton
        onPressedChanged: if (pressed) {
            const pp = point.pressPosition
            root.pressInGap = !root.cardAt(pp.x, pp.y) && !root.overInspector(pp.x, pp.y)
            root.forceActiveFocus()
        }
        onTapped: if (root.pressInGap) triage.clearSelection()
    }
    DragHandler {
        id: marqueeDrag
        target: null
        acceptedButtons: Qt.LeftButton
        dragThreshold: 4
        onActiveChanged: {
            if (active && root.pressInGap) {
                const m = Qt.application.keyboardModifiers
                root.marqueeBase = (m & (Qt.ShiftModifier | Qt.ControlModifier)) ? triage.selectedPaths() : []
                root.marqueeOn = true
            } else {
                root.marqueeOn = false
            }
        }
        onCentroidChanged: if (active && root.marqueeOn) {
            const a = centroid.pressPosition, b = centroid.position
            const r = Qt.rect(Math.min(a.x, b.x), Math.min(a.y, b.y), Math.abs(a.x - b.x), Math.abs(a.y - b.y))
            root.marquee = r
            const hit = root.cardsIn(r)
            triage.setSelection(root.marqueeBase.concat(hit.filter(p => root.marqueeBase.indexOf(p) < 0)), false)
        }
    }
    Rectangle {
        visible: root.marqueeOn
        x: root.marquee.x; y: root.marquee.y; width: root.marquee.width; height: root.marquee.height
        radius: 3
        color: Theme.alpha(Theme.accent, 0.10)
        border.color: Theme.alpha(Theme.accent, 0.6)
    }

    // ── Keyboard triage ──────────────────────────────────────────────────
    Keys.onPressed: event => {
        const cmd = (event.modifiers & Qt.ControlModifier) !== 0
        const other = event.modifiers & ~Qt.ShiftModifier & ~Qt.KeypadModifier
        switch (event.key) {
        case Qt.Key_Up: triage.moveAnchor(0, -1); break
        case Qt.Key_Down: triage.moveAnchor(0, 1); break
        case Qt.Key_Left: triage.moveAnchor(-1, 0); break
        case Qt.Key_Right: triage.moveAnchor(1, 0); break
        case Qt.Key_Return: case Qt.Key_Enter: triage.openFocus(); break
        case Qt.Key_Escape: if (triage.selectedCount > 1) triage.clearSelection(); break
        case Qt.Key_BracketLeft: triage.stepGrid(-1); break
        case Qt.Key_BracketRight: triage.stepGrid(1); break
        case Qt.Key_A: if (cmd) triage.selectAll(); else if (!other) triage.setStatusSelection(4); else return; break
        default:
            if (other) return
            switch (event.key) {
            case Qt.Key_K: triage.setStatusSelection(1); break
            case Qt.Key_M: triage.setStatusSelection(2); break
            case Qt.Key_D: triage.setStatusSelection(3); break
            case Qt.Key_U: triage.setStatusSelection(0); break
            case Qt.Key_F: triage.openFocus(); break
            case Qt.Key_N: if (triage.selectedCount > 0) { triage.focusNote = true; triage.inspectorOpen = true } break
            case Qt.Key_R: if (triage.anchorEntry) triage.reveal(triage.anchorEntry.path); break
            case Qt.Key_I: triage.inspectorOpen = !triage.inspectorOpen; break
            default: return
            }
        }
        event.accepted = true
    }

    // ── Drop files / folders to import ───────────────────────────────────
    DropArea {
        id: drop
        anchors.fill: parent
        onDropped: ev => { if (ev.hasUrls) triage.importPaths(ev.urls) }
    }
    Rectangle {
        visible: drop.containsDrag
        anchors.fill: parent; anchors.margins: 8
        radius: Theme.rPanel
        color: Theme.alpha(Theme.accent, 0.06)
        border.color: Theme.alpha(Theme.accent, 0.7)
        border.width: 2
        Txt { anchors.centerIn: parent; text: "Drop to import"; variant: "title"; color: Theme.accent }
    }
}
