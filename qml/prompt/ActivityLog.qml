import QtQuick
import QtQuick.Controls.Basic

// What the app is doing right now (model calls, uploads, polling, downloads), drawn as a terminal session:
// phosphor green on near-black, monospace, one prefixed line per event and a blinking block cursor.
Rectangle {
    id: root
    property int running: 0
    signal clear()

    readonly property color ink: "#8FD3A2"          // ordinary lines
    readonly property color dimInk: "#3F6B4D"       // timestamps, chrome
    readonly property color okInk: "#4ADE80"
    readonly property color errInk: "#FF6B5C"
    readonly property color warnInk: "#F2C14E"
    readonly property color actInk: "#F54F1B"

    color: "#060908"
    radius: Theme.rCard
    border.color: Qt.rgba(0.29, 0.87, 0.5, 0.20)
    clip: true

    function prefix(kind) { return kind === 1 ? "✓" : kind === 2 ? "✗" : kind === 3 ? "!" : kind === 4 ? "$" : ">" }
    function tone(kind) { return kind === 1 ? okInk : kind === 2 ? errInk : kind === 3 ? warnInk : kind === 4 ? actInk : ink }

    Column {
        anchors.fill: parent
        Item {
            width: parent.width; height: 34
            Row {
                anchors.left: parent.left; anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 7
                Repeater {
                    model: 3
                    delegate: Rectangle { width: 8; height: 8; radius: 4; color: root.dimInk; opacity: 0.8; anchors.verticalCenter: parent.verticalCenter }
                }
                Text {
                    text: "  activity — higgsfield"
                    color: root.dimInk; font.family: Theme.mono; font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    visible: root.running > 0
                    text: "· " + root.running + " running"
                    color: root.okInk; font.family: Theme.mono; font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Item {
                id: clearBtn
                anchors.right: parent.right; anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: clearText.implicitWidth + 16; height: 22
                Rectangle { anchors.fill: parent; radius: 5; color: clearHover.hovered ? Qt.rgba(0.29, 0.87, 0.5, 0.12) : "transparent" }
                Text { id: clearText; anchors.centerIn: parent; text: "clear"; color: clearHover.hovered ? root.okInk : root.dimInk; font.family: Theme.mono; font.pixelSize: 11 }
                HoverHandler { id: clearHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.clear() }
            }
        }
        Rectangle { width: parent.width; height: 1; color: Qt.rgba(0.29, 0.87, 0.5, 0.14) }
        ListView {
            id: list
            width: parent.width
            height: parent.height - 35
            clip: true
            model: prompt.log
            spacing: 2
            topMargin: 10; bottomMargin: 10
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: StudioScrollBar {}
            onCountChanged: Qt.callLater(() => positionViewAtEnd())
            delegate: Row {
                required property string time
                required property string line
                required property int kind
                x: 14
                width: list.width - 28
                spacing: 8
                Text { text: "[" + time + "]"; color: root.dimInk; font.family: Theme.mono; font.pixelSize: 11; width: 66; renderType: Text.QtRendering }
                Text { text: root.prefix(kind); color: root.tone(kind); font.family: Theme.mono; font.pixelSize: 12; font.bold: true; width: 10; horizontalAlignment: Text.AlignHCenter; renderType: Text.QtRendering }
                Text {
                    width: parent.width - 66 - 10 - 16
                    text: line.replace(/^▶\s*/, "").replace(/\s*✓$/, "")
                    wrapMode: Text.Wrap
                    color: root.tone(kind)
                    font.family: Theme.mono; font.pixelSize: 12
                    lineHeight: 1.15
                    renderType: Text.QtRendering
                }
            }
            // the prompt line: a block cursor that blinks on a slow timer (no continuous animation)
            footer: Item {
                width: list.width; height: 22
                Row {
                    x: 14; y: 4
                    spacing: 8
                    Text { text: "[--:--:--]"; color: "transparent"; font.family: Theme.mono; font.pixelSize: 11; width: 66 }
                    Text { text: "$"; color: root.actInk; font.family: Theme.mono; font.pixelSize: 12; font.bold: true; width: 10; horizontalAlignment: Text.AlignHCenter }
                    Rectangle {
                        id: cursor
                        width: 8; height: 14; y: 1
                        color: root.okInk
                        opacity: blink.on ? 0.9 : 0
                        Timer {
                            id: blink
                            property bool on: true
                            interval: 530; repeat: true
                            running: root.visible && !Theme.reduceMotion
                            onTriggered: on = !on
                        }
                    }
                }
            }
        }
    }
}
