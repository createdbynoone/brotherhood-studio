import QtQuick
import QtQuick.Controls.Basic

// The generated prompt (editorial reading column) and the activity log.
Item {
    id: root
    property bool compact: false
    implicitHeight: compact ? col.implicitHeight : 0

    Column {
        id: col
        anchors.fill: compact ? undefined : parent
        anchors.left: compact ? parent.left : undefined
        anchors.right: compact ? parent.right : undefined
        spacing: Theme.s12

        // Prompt
        Card {
            id: promptCard
            visible: prompt.prompt.length > 0
            padding: 0
            width: parent.width
            height: compact ? 300 : (prompt.logCount > 0 || prompt.tasks > 0 ? Math.max(180, root.height - 260 - Theme.s12) : root.height)
            Column {
                anchors.fill: parent
                Item {
                    width: parent.width; height: 40
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        Txt { text: "Prompt"; variant: "strong"; anchors.verticalCenter: parent.verticalCenter }
                        Txt { text: prompt.prompt.length + " characters"; variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Btn {
                        id: copyBtn
                        property bool copied: false
                        anchors.right: parent.right; anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        kind: "ghost"; size: "xs"
                        icon: copied ? "check" : "copy"; text: copied ? "Copied" : "Copy"
                        hint: "⌘⇧C"
                        onClicked: { studio.copyText(prompt.prompt); copied = true; reset.restart() }
                        Timer { id: reset; interval: 1500; onTriggered: copyBtn.copied = false }
                    }
                }
                Hairline {}
                Flickable {
                    id: promptScroll
                    width: parent.width
                    height: parent.height - 41
                    contentWidth: width
                    contentHeight: promptText.implicitHeight + 28
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: StudioScrollBar {}
                    Reading {
                        id: promptText
                        x: 14; y: 14
                        width: promptScroll.width - 28
                        plain: prompt.prompt
                    }
                }
            }
        }

        ActivityLog {
            visible: prompt.logCount > 0 || prompt.tasks > 0
            width: parent.width
            height: compact ? 200 : (prompt.prompt.length > 0 ? 260 : root.height)
            running: prompt.tasks
            onClear: prompt.clearLog()
        }
    }

    // Empty state: three steps, composed
    Column {
        visible: !compact && prompt.prompt.length === 0 && prompt.logCount === 0 && prompt.tasks === 0
        anchors.centerIn: parent
        spacing: 18
        BrandMark { size: 34; tint: Theme.textMuted; anchors.horizontalCenter: parent.horizontalCenter }
        Column {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter
            Repeater {
                model: ["Drop one reference and the product", "Add specs, or let Sonnet write the brief", "Generate with Opus, then fire to Higgsfield"]
                delegate: Row {
                    required property string modelData
                    required property int index
                    spacing: 12
                    Txt { text: "0" + (index + 1); variant: "mono"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                    Txt { text: modelData; dim: true; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
    }
}
