import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

// The window: top bar, the two panes, footer, and everything that floats above them.
Item {
    id: view

    Component.onCompleted: {
        prompt.boot(); triage.boot(); studio.checkForUpdate()
        if (startOpen === "settings") settings.open()
        else if (startOpen === "palette") palette.show()
        else if (startOpen === "log") prompt.demoLog()
        else if (startOpen === "focus") Qt.callLater(() => focusTimer.start())
    }
    Timer { id: focusTimer; interval: 1800; onTriggered: { triage.moveAnchor(1, 0); triage.openFocus() } }

    // New versions are picked up while the app stays open (it used to check only at launch)
    Timer {
        interval: 30 * 60 * 1000; repeat: true; running: true
        onTriggered: if (!studio.updateAvailable && !studio.updating && !studio.checkingUpdate) studio.checkForUpdate()
    }

    // ── Feedback from the three controllers ──────────────────────────────
    Connections { target: studio; function onToast(m, k) { toasts.show(m, k) } }
    Connections { target: prompt; function onToast(m, k) { toasts.show(m, k) } }
    Connections { target: triage; function onToast(m, k) { toasts.show(m, k) } }

    // ── Commands (palette + shortcuts) ───────────────────────────────────
    function nameFilter(i) { return ["All", "Unsorted", "Keep", "Maybe", "Discard", "Archived"][i] }
    readonly property var commands: [
        { title: "Show Marketing Prompts only", group: "View", hint: "⌘1", icon: "square-half", run: () => studio.layout = "prompt" },
        { title: "Show both panes", group: "View", hint: "⌘2", icon: "columns", run: () => studio.layout = "split" },
        { title: "Show Generation Sorter only", group: "View", hint: "⌘3", icon: "square-half", run: () => studio.layout = "triage" },
        { title: "Switch pane", group: "View", hint: "⌃Tab", icon: "swap", run: () => studio.cyclePane() },
        { title: "Generate prompt", group: "Marketing Prompts", hint: "⌘↩", icon: "magic-wand", run: () => prompt.generate() },
        { title: "Fire to Higgsfield", group: "Marketing Prompts", hint: "⌘⇧↩", icon: "lightning", run: () => prompt.fire() },
        { title: "Auto-write brief", group: "Marketing Prompts", icon: "sparkle", run: () => prompt.autoWriteBrief() },
        { title: "Prompt history", group: "Marketing Prompts", hint: "⌘Y", icon: "clock-counter-clockwise", run: () => history.open() },
        { title: "Reset inputs", group: "Marketing Prompts", hint: "⌘⇧K", icon: "arrow-counter-clockwise", run: () => prompt.reset() },
        { title: "Copy prompt", group: "Marketing Prompts", hint: "⌘⇧C", icon: "copy", run: () => studio.copyText(prompt.prompt) },
        { title: "Open output folder", group: "Marketing Prompts", hint: "⌘⇧O", icon: "folder-open", run: () => studio.openFolder(prompt.outputPath) },
        { title: "Rescan output folder", group: "Generation Sorter", hint: "⌘⌥R", icon: "arrow-clockwise", run: () => triage.rescan() },
        { title: "Import a folder", group: "Generation Sorter", hint: "⌘I", icon: "folder-plus", run: () => triagePane.requestImport() },
        { title: "Select all", group: "Generation Sorter", hint: "⌘A", icon: "check", run: () => triage.selectAll() },
        { title: "Deselect", group: "Generation Sorter", hint: "⌘⇧D", icon: "x", run: () => triage.clearSelection() },
        { title: "Toggle inspector", group: "Generation Sorter", hint: "I", icon: "sidebar", run: () => triage.inspectorOpen = !triage.inspectorOpen },
        { title: "Trash discarded", group: "Generation Sorter", hint: "⌘⌫", icon: "trash", run: () => triagePane.requestTrash() },
        { title: "Filter: All", group: "Generation Sorter", icon: "funnel", run: () => triage.filter = 0 },
        { title: "Filter: Unsorted", group: "Generation Sorter", icon: "funnel", run: () => triage.filter = 1 },
        { title: "Filter: Keep", group: "Generation Sorter", icon: "arrow-up", run: () => triage.filter = 2 },
        { title: "Filter: Maybe", group: "Generation Sorter", icon: "approximate-equals", run: () => triage.filter = 3 },
        { title: "Filter: Discard", group: "Generation Sorter", icon: "x", run: () => triage.filter = 4 },
        { title: "Filter: Archived", group: "Generation Sorter", icon: "archive-box", run: () => triage.filter = 5 },
        { title: "Settings", group: "App", hint: "⌘,", icon: "gear", run: () => settings.open() },
        { title: "Check for updates", group: "App", icon: "download-simple", run: () => studio.checkForUpdate() }
    ]

    Shortcut { sequences: ["Ctrl+K"]; onActivated: palette.show() }
    Shortcut { sequences: ["Ctrl+1"]; onActivated: studio.layout = "prompt" }
    Shortcut { sequences: ["Ctrl+2"]; onActivated: studio.layout = "split" }
    Shortcut { sequences: ["Ctrl+3"]; onActivated: studio.layout = "triage" }
    Shortcut { sequences: ["Meta+Tab"]; onActivated: studio.cyclePane() }
    Shortcut { sequences: ["Ctrl+Return", "Ctrl+Enter"]; enabled: studio.layout !== "triage"; onActivated: prompt.generate() }
    Shortcut { sequences: ["Ctrl+Shift+Return", "Ctrl+Shift+Enter"]; enabled: studio.layout !== "triage"; onActivated: prompt.fire() }
    Shortcut { sequences: ["Ctrl+Y"]; enabled: studio.layout !== "triage"; onActivated: history.open() }
    Shortcut { sequences: ["Ctrl+Shift+K"]; onActivated: prompt.reset() }
    Shortcut { sequences: ["Ctrl+Shift+C"]; enabled: prompt.prompt.length > 0; onActivated: studio.copyText(prompt.prompt) }
    Shortcut { sequences: ["Ctrl+Shift+O"]; onActivated: studio.openFolder(prompt.outputPath) }
    Shortcut { sequences: ["Ctrl+,"]; onActivated: settings.open() }
    Shortcut { sequences: ["Ctrl+I"]; enabled: studio.layout !== "prompt"; onActivated: triagePane.requestImport() }
    Shortcut { sequences: ["Ctrl+Alt+R"]; enabled: studio.layout !== "prompt"; onActivated: triage.rescan() }
    Shortcut { sequences: ["Ctrl+Shift+R"]; enabled: studio.layout !== "prompt" && triage.anchorEntry !== null; onActivated: triage.reveal(triage.anchorEntry.path) }
    Shortcut { sequences: ["Ctrl+Shift+D"]; enabled: studio.layout !== "prompt"; onActivated: triage.clearSelection() }
    Shortcut { sequences: ["Ctrl+Backspace"]; enabled: studio.layout !== "prompt" && (triage.statusCounts[3] || 0) > 0; onActivated: triagePane.requestTrash() }
    Shortcut { sequences: ["Ctrl+Esc"]; enabled: triage.focusIndex >= 0; onActivated: triage.closeFocus() }

    // True when (sx, sy), in window (scene) coordinates, is title-bar space where a native drag may start. Qt leaves a
    // safe-area strip above the content for the traffic lights, so the point is mapped into this item first.
    function dragAt(sx, sy) {
        const p = view.mapFromItem(null, sx, sy)
        if (p.y > topBar.height) return false
        if (p.y < 0) return true                      // the strip with the traffic lights
        const c = topBar.childAt(p.x, p.y)
        return !c || c.dragThrough === true
    }

    // ── Top bar ──────────────────────────────────────────────────────────
    Rectangle {
        id: topBar
        width: parent.width; height: 60
        color: Theme.bg
        Hairline { anchors.bottom: parent.bottom; property bool dragThrough: true }

        Row {   // after the traffic lights
            property bool dragThrough: true
            anchors.left: parent.left; anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14
            BrandMark { size: 34; anchors.verticalCenter: parent.verticalCenter }
            Wordmark { visible: view.width >= 1420; size: 19; anchors.verticalCenter: parent.verticalCenter }
        }

        Segment {
            anchors.centerIn: parent
            height_: 40; fontSize: 12; hPad: 22; gap: 6; inset: 4; heading: true
            options: view.width >= 1080 ? [{ key: "prompt", label: "Marketing Prompts" }, { key: "split", label: "Split" }, { key: "triage", label: "Generation Sorter" }]
                                        : [{ key: "prompt", label: "Prompts" }, { key: "split", label: "Split" }, { key: "triage", label: "Sorter" }]
            current: studio.layout
            onPicked: key => studio.layout = key
        }

        Row {
            anchors.right: parent.right; anchors.rightMargin: Theme.s16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // Credits
            Row {
                visible: prompt.hasCredits
                spacing: 7
                anchors.verticalCenter: parent.verticalCenter
                Item {
                    width: 14; height: 14
                    anchors.verticalCenter: parent.verticalCenter
                    CreditsRing { fraction: Math.min(prompt.credits / 1000, 1) }
                }
                Txt {
                    text: prompt.credits % 1 === 0 ? prompt.credits + " cr" : prompt.credits.toFixed(1) + " cr"
                    variant: "small"; dim: true; font.family: Theme.mono
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Palette
            Rectangle {
                id: palBox
                width: paletteRow.implicitWidth + 20; height: 28
                readonly property bool slim: view.width < 1240
                radius: Theme.rControl
                color: palHover.hovered ? Theme.elevated : Theme.raised
                border.color: palHover.hovered ? Theme.hairlineStrong : Theme.hairline
                anchors.verticalCenter: parent.verticalCenter
                Row {
                    id: paletteRow
                    anchors.centerIn: parent
                    spacing: 8
                    Icon { name: "magnifying-glass"; size: 13; color: Theme.textDim; anchors.verticalCenter: parent.verticalCenter }
                    Txt { visible: !palBox.slim; text: "Search actions"; variant: "small"; dim: true; anchors.verticalCenter: parent.verticalCenter }
                    Txt { text: "⌘K"; variant: "mono"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                }
                HoverHandler { id: palHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: palette.show() }
            }

            Btn { kind: "ghost"; size: "s"; icon: "gear"; tip: "Settings"; hint: "⌘,"; anchors.verticalCenter: parent.verticalCenter; onClicked: settings.open() }
        }
    }

    // Update notice
    Rectangle {
        id: updateBar
        anchors.top: topBar.bottom
        width: parent.width
        height: studio.updateAvailable ? 34 : 0
        visible: height > 0
        clip: true
        color: Theme.alpha(Theme.accent, 0.08)
        Behavior on height { NumberAnimation { duration: Theme.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curve } }
        Hairline { anchors.bottom: parent.bottom }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Icon { name: "download-simple"; weight: "bold"; size: 14; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
            Txt { text: "Version " + studio.updateVersion + " is available"; variant: "small"; anchors.verticalCenter: parent.verticalCenter }
            Btn {
                kind: "primary"; size: "xs"
                text: studio.updating ? "Downloading " + Math.round(studio.updateProgress * 100) + "%" : "Install and restart"
                busy: studio.updating
                enabled: !studio.updating
                anchors.verticalCenter: parent.verticalCenter
                onClicked: studio.installUpdate()
            }
        }
        Btn { anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; kind: "ghost"; size: "xs"; icon: "x"; onClicked: studio.dismissUpdate() }
    }

    // ── Panes ────────────────────────────────────────────────────────────
    // Each pane lives in a clipping box whose width animates, while the pane itself never gets narrower
    // than its minimum: a collapsing pane is covered like a curtain instead of being squeezed, and an
    // opening one is revealed at full layout. That removes the squashed intermediate frames.
    Item {
        id: split
        y: topBar.height + updateBar.height
        width: parent.width
        height: parent.height - y - footer.height
        clip: true

        readonly property real minLeft: 400
        readonly property real minRight: 460
        readonly property bool both: studio.layout === "split"
        readonly property real target: studio.layout === "prompt" ? width
            : studio.layout === "triage" ? 0
            : Math.min(Math.max(width * studio.splitFraction, minLeft), Math.max(minLeft, width - minRight - 1))
        property real leftW: target
        Behavior on leftW {
            enabled: !divider.dragging
            NumberAnimation { id: paneGlide; duration: Theme.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curveLayout }
        }
        readonly property bool gliding: paneGlide.running
        readonly property real rightX: leftW + (both ? 1 : 0)

        Item {
            id: leftBox
            x: 0; width: split.leftW; height: parent.height
            clip: true
            visible: width > 0.5
            PromptPane {
                x: 0; height: parent.height
                width: Math.max(leftBox.width, split.minLeft)
                modeWidth: split.target
                active: studio.layout === "split" && studio.activePane === "prompt"
                onOpenHistory: history.open()
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; grabPermissions: PointerHandler.ApprovesTakeOverByAnything; onPressedChanged: if (pressed) studio.activePane = "prompt" }
            }
            Rectangle {   // veil: fades the pane out as its box closes
                anchors.fill: parent
                color: Theme.bg
                opacity: 1 - Math.min(1, leftBox.width / 280)
                visible: opacity > 0.01
            }
        }

        Item {
            id: divider
            readonly property bool dragging: dragger.active
            x: split.leftW - 4; width: 9; height: parent.height
            opacity: split.both ? 1 : 0
            visible: opacity > 0.01
            Behavior on opacity { NumberAnimation { duration: Theme.base } }
            z: 5
            property real startLeft: 0
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 1; height: parent.height; color: dividerHover.hovered || dragger.active ? Theme.hairlineStrong : Theme.hairline }
            HoverHandler { id: dividerHover; cursorShape: Qt.SplitHCursor }
            DragHandler {
                id: dragger
                target: null
                xAxis.enabled: true
                onActiveChanged: { if (active) divider.startLeft = split.leftW; else studio.saveSplit() }
                onTranslationChanged: if (active) studio.splitFraction = (divider.startLeft + translation.x) / split.width
            }
            TapHandler { onDoubleTapped: { studio.splitFraction = 0.42; studio.saveSplit() } }
        }

        Item {
            id: rightBox
            x: split.rightX; width: split.width - x; height: parent.height
            clip: true
            visible: width > 0.5
            TriagePane {
                anchors.right: parent.right
                height: parent.height
                width: Math.max(rightBox.width, split.minRight)
                active: studio.layout === "split" && studio.activePane === "triage"
                id: triagePane
                freezeLayout: split.gliding
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; grabPermissions: PointerHandler.ApprovesTakeOverByAnything; onPressedChanged: if (pressed) studio.activePane = "triage" }
            }
            Rectangle {
                anchors.fill: parent
                color: Theme.bg
                opacity: 1 - Math.min(1, rightBox.width / 280)
                visible: opacity > 0.01
            }
        }
    }

    // ── Footer ───────────────────────────────────────────────────────────
    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        width: parent.width; height: 28
        color: Theme.surface
        Hairline { anchors.top: parent.top }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            visible: studio.layout !== "triage"
            Txt { text: prompt.providerLabel + "  " + prompt.aspectRatio + "  " + prompt.resolution.toUpperCase(); variant: "caption"; muted: true; font.family: Theme.mono }
            Txt { visible: prompt.memoryTotal > 0; text: prompt.memoryTotal + " in memory"; variant: "caption"; muted: true }
            Txt { visible: prompt.memoryTotal > 0; text: "★ " + prompt.memoryFired + " fired"; variant: "caption"; color: Theme.alpha(Theme.accent, 0.8) }
        }
        Row {
            anchors.right: parent.right; anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            visible: true
            Row {
                visible: studio.layout !== "prompt"
                spacing: 10
                Txt { text: triage.totalCount + " total"; variant: "caption"; muted: true }
                Txt { visible: (triage.statusCounts[1] || 0) > 0; text: triage.statusCounts[1] + " keep"; variant: "caption"; color: Theme.alpha(Theme.ok, 0.85) }
                Txt { visible: (triage.statusCounts[2] || 0) > 0; text: triage.statusCounts[2] + " maybe"; variant: "caption"; color: Theme.alpha(Theme.warn, 0.9) }
                Txt { visible: (triage.statusCounts[3] || 0) > 0; text: triage.statusCounts[3] + " discard"; variant: "caption"; color: Theme.alpha(Theme.danger, 0.85) }
                Txt { visible: (triage.statusCounts[0] || 0) > 0; text: triage.statusCounts[0] + " unsorted"; variant: "caption"; muted: true }
                Txt { visible: triage.selectedCount > 1; text: triage.selectedCount + " selected"; variant: "caption" }
                Txt { text: studio.abbreviate(triage.watchPath); variant: "caption"; muted: true; font.family: Theme.mono; elide: Text.ElideMiddle; width: Math.min(implicitWidth, 260) }
            }
            Txt { text: "v" + studio.version; variant: "caption"; muted: true }
        }
    }

    // ── Floating layers ──────────────────────────────────────────────────
    ToastHost { id: toasts; anchors.fill: parent; anchors.bottomMargin: footer.height + 16 }
    CommandPalette { id: palette; anchors.fill: parent; commands: view.commands; z: 20 }
    HistoryDialog { id: history }
    SettingsDialog { id: settings }
    LoginDialog { id: login; visible: prompt.showLogin }
}
