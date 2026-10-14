import QtQuick
import QtQuick.Controls.Basic

// What the app is doing right now: model calls, uploads, polling, downloads.
Card {
    id: root
    padding: 0
    property int running: 0
    signal clear()

    Column {
        anchors.fill: parent
        Item {
            width: parent.width; height: 38
            Row {
                anchors.left: parent.left; anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Txt { text: "Activity"; variant: "strong"; anchors.verticalCenter: parent.verticalCenter }
                Pulse { visible: root.running > 0; running: visible; anchors.verticalCenter: parent.verticalCenter }
                Txt { visible: root.running > 0; text: root.running + " running"; variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter }
            }
            Btn {
                anchors.right: parent.right; anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                kind: "ghost"; size: "xs"; text: "Clear"
                onClicked: root.clear()
            }
        }
        Hairline {}
        ListView {
            id: list
            width: parent.width
            height: parent.height - 39
            clip: true
            model: prompt.log
            spacing: 3
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
                spacing: 10
                Txt { text: time; variant: "mono"; muted: true; font.pixelSize: 11; width: 52 }
                Txt {
                    width: parent.width - 62
                    text: line
                    variant: "small"
                    wrapMode: Text.Wrap
                    color: kind === 1 ? Theme.ok : kind === 2 ? Theme.danger : kind === 3 ? Theme.warn : kind === 4 ? Theme.accent : Theme.textDim
                }
            }
        }
    }
}
