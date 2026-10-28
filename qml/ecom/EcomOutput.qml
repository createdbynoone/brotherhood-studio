import QtQuick
import QtQuick.Controls.Basic

// The JSON specification (editable), the images made in this session and the activity log.
Item {
    id: root
    property bool compact: false
    readonly property bool hasResults: ecom.results.length > 0
    readonly property bool hasLog: ecom.logCount > 0 || ecom.tasks > 0
    implicitHeight: compact ? col.implicitHeight : 0

    Column {
        id: col
        anchors.fill: compact ? undefined : parent
        anchors.left: compact ? parent.left : undefined
        anchors.right: compact ? parent.right : undefined
        spacing: Theme.s12

        // JSON
        Card {
            visible: ecom.json.length > 0
            padding: 0
            width: parent.width
            height: compact ? 380 : Math.max(220, root.height - (root.hasResults ? 170 + Theme.s12 : 0) - (root.hasLog ? 220 + Theme.s12 : 0))
            Column {
                anchors.fill: parent
                Item {
                    width: parent.width; height: 40
                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        Txt { text: "Spec · JSON"; variant: "strong"; anchors.verticalCenter: parent.verticalCenter }
                        Txt {
                            text: ecom.jsonValid ? ecom.json.length + " characters" : "not valid JSON"
                            variant: "caption"; muted: ecom.jsonValid
                            color: ecom.jsonValid ? Theme.textMuted : Theme.danger
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Btn {
                        id: copyBtn
                        property bool copied: false
                        anchors.right: parent.right; anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        kind: "ghost"; size: "xs"
                        icon: copied ? "check" : "copy"; text: copied ? "Copied" : "Copy"
                        onClicked: { studio.copyText(ecom.json); copied = true; reset.restart() }
                        Timer { id: reset; interval: 1500; onTriggered: copyBtn.copied = false }
                    }
                }
                Hairline {}
                TextBox {
                    id: jsonBox
                    width: parent.width
                    height: parent.height - 41
                    radius: 0
                    color: "transparent"
                    border.width: 0
                    fontSize: 12
                    text: ecom.json
                    onEdited: if (ecom.json !== text) ecom.json = text
                    Component.onCompleted: editItem.font.family = Theme.mono
                }
            }
        }

        // Images from this session
        Card {
            visible: root.hasResults
            width: parent.width
            height: 170
            padding: 12
            Column {
                anchors.fill: parent
                spacing: 8
                Item {
                    width: parent.width; height: 20
                    Txt { text: "Results"; variant: "strong"; anchors.verticalCenter: parent.verticalCenter }
                    Btn {
                        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                        kind: "ghost"; size: "xs"; icon: "folder-open"; text: "Open folder"
                        onClicked: studio.openFolder(ecom.resultsPath)
                    }
                }
                ListView {
                    width: parent.width; height: parent.height - 28
                    orientation: ListView.Horizontal
                    spacing: 8
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: ecom.results
                    delegate: Rectangle {
                        required property string modelData
                        width: 88; height: ListView.view.height
                        radius: Theme.rControl
                        color: Theme.raised
                        border.color: hov.hovered ? Theme.hairlineStrong : Theme.hairline
                        clip: true
                        Image {
                            anchors.fill: parent; anchors.margins: 1
                            source: Theme.fileUrl(modelData)
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.height: 300
                        }
                        HoverHandler { id: hov; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: studio.revealFile(modelData) }
                    }
                }
            }
        }

        ActivityLog {
            visible: root.hasLog
            logModel: ecom.log
            width: parent.width
            height: compact ? 200 : (ecom.json.length > 0 || root.hasResults ? 220 : root.height)
            running: ecom.tasks
            onClear: ecom.clearLog()
        }
    }

    // Empty state
    Column {
        visible: !compact && ecom.json.length === 0 && !root.hasLog && !root.hasResults
        anchors.centerIn: parent
        spacing: 18
        BrandMark { size: 34; tint: Theme.textMuted; anchors.horizontalCenter: parent.horizontalCenter }
        Column {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter
            Repeater {
                model: ["Drop the product photo (front or back)", "Opus detects the product and matches the Brotherhood house style", "Review the JSON, then fire to Higgsfield"]
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
