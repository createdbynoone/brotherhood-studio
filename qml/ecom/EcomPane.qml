import QtQuick
import QtQuick.Controls.Basic

// Ecommerce pane: product photo in, JSON spec out, fire. Wide panes use two columns; narrow ones stack them.
Rectangle {
    id: root
    color: Theme.bg
    Component.onCompleted: ecom.scheduleCost()

    Column {
        anchors.fill: parent

        PaneHeader {
            width: parent.width
            title: "Ecommerce"
            active: true
            Btn { kind: "ghost"; size: "s"; icon: "folder-open"; tip: "Open results folder"; onClicked: studio.openFolder(ecom.resultsPath) }
            Btn { kind: "ghost"; size: "s"; icon: "arrow-counter-clockwise"; tip: "Reset inputs"; hint: "⌘⇧K"; onClicked: ecom.reset() }
        }

        Item {
            id: content
            width: parent.width
            height: parent.height - y - dock.height
            readonly property bool wide: root.width >= 760

            Row {
                visible: content.wide
                anchors.fill: parent
                Flickable {
                    width: 380; height: parent.height
                    contentWidth: width
                    contentHeight: wideInputs.implicitHeight + Theme.s16 * 2
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: StudioScrollBar {}
                    EcomInputs { id: wideInputs; x: Theme.s16; y: Theme.s16; width: 380 - Theme.s16 * 2 }
                }
                Hairline { vertical: true; height: parent.height }
                Rectangle {
                    width: parent.width - 381; height: parent.height
                    color: Theme.alpha(Theme.surface, 0.5)
                    EcomOutput { anchors.fill: parent; anchors.margins: Theme.s16; compact: false }
                }
            }

            Flickable {
                visible: !content.wide
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
                    EcomInputs { width: parent.width }
                    EcomOutput { width: parent.width; compact: true; visible: ecom.json.length > 0 || ecom.logCount > 0 || ecom.tasks > 0 || ecom.results.length > 0 }
                }
            }
        }

        FireDock { id: dock; width: parent.width; ctl: ecom; choosable: false }
    }
}
