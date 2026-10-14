import QtQuick
import QtQuick.Controls.Basic

// BMP pane: inputs, prompt, fire. Wide panes use two columns; narrow ones stack them in one scroll.
Rectangle {
    id: root
    property bool active: false
    property real modeWidth: width        // the width this pane is heading to (decides one vs two columns)
    signal openHistory()
    color: Theme.bg

    Column {
        anchors.fill: parent

        PaneHeader {
            width: parent.width
            title: "Marketing Prompts"
            active: root.active
            Btn { kind: "ghost"; size: "s"; icon: "clock-counter-clockwise"; tip: "Prompt history"; hint: "⌘Y"; onClicked: root.openHistory() }
            Btn { kind: "ghost"; size: "s"; icon: "arrow-counter-clockwise"; tip: "Reset inputs"; hint: "⌘⇧K"; onClicked: prompt.reset() }
        }

        // Missing CLI notice
        Rectangle {
            visible: prompt.missingTools.length > 0
            width: parent.width
            height: visible ? 34 : 0
            color: Theme.alpha(Theme.warn, 0.08)
            Hairline { anchors.bottom: parent.bottom }
            Row {
                anchors.left: parent.left; anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Icon { name: "warning"; weight: "fill"; size: 14; color: Theme.warn; anchors.verticalCenter: parent.verticalCenter }
                Txt {
                    text: "Missing CLI: " + prompt.missingTools.join(" and ") + ". Install with npm i -g @anthropic-ai/claude-code @higgsfield/cli and reopen the app."
                    variant: "small"; color: Theme.warn; anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        Item {
            id: content
            width: parent.width
            height: parent.height - y - dock.height
            // Wide / narrow follows the width the pane is heading to, not the animating one, and the swap
            // happens behind a short veil so the columns never reflow in view.
            property bool wide: root.modeWidth >= 760
            property bool shownWide: wide
            onWideChanged: swap.restart()
            SequentialAnimation {
                id: swap
                NumberAnimation { target: veil; property: "opacity"; to: 1; duration: Theme.ms(90); easing.type: Easing.OutCubic }
                ScriptAction { script: content.shownWide = content.wide }
                NumberAnimation { target: veil; property: "opacity"; to: 0; duration: Theme.ms(220); easing.type: Easing.InOutCubic }
            }

            // Wide: inputs | output
            Row {
                visible: content.shownWide
                anchors.fill: parent
                Flickable {
                    width: 380; height: parent.height
                    contentWidth: width
                    contentHeight: wideInputs.implicitHeight + Theme.s16 * 2
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: StudioScrollBar {}
                    InputsColumn { id: wideInputs; x: Theme.s16; y: Theme.s16; width: 380 - Theme.s16 * 2 }
                }
                Hairline { vertical: true; height: parent.height }
                Rectangle {
                    width: parent.width - 381; height: parent.height
                    color: Theme.alpha(Theme.surface, 0.5)
                    OutputColumn { anchors.fill: parent; anchors.margins: Theme.s16; compact: false }
                }
            }

            // Narrow: one scrolling column
            Flickable {
                visible: !content.shownWide
                anchors.fill: parent
                contentWidth: width
                contentHeight: narrow.implicitHeight + Theme.s16 * 2
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: StudioScrollBar {}
                Column {
                    id: narrow
                    x: Theme.s16; y: Theme.s16
                    width: parent.width - Theme.s16 * 2
                    spacing: Theme.s12
                    InputsColumn { width: parent.width }
                    OutputColumn { width: parent.width; compact: true; visible: prompt.hasOutput }
                }
            }

            Rectangle { id: veil; anchors.fill: parent; color: Theme.bg; opacity: 0; visible: opacity > 0.01; z: 5 }
        }

        FireDock { id: dock; width: parent.width }
    }
}
