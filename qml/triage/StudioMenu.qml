import QtQuick
import QtQuick.Controls.Basic

// Context menu in the app's own look.
Menu {
    id: root
    padding: 6
    topPadding: 6; bottomPadding: 6
    background: Rectangle {
        implicitWidth: 200
        radius: Theme.rCard
        color: Theme.elevated
        border.color: Theme.hairlineStrong
    }
    delegate: MenuItem {
        id: item
        implicitHeight: 30
        implicitWidth: 200
        leftPadding: 10; rightPadding: 10
        indicator: Item {}
        contentItem: Row {
            spacing: 8
            Item {
                width: 14; height: parent.height
                Icon { anchors.centerIn: parent; visible: item.checked; name: "check"; weight: "bold"; size: 12; color: Theme.accent }
            }
            Txt {
                text: item.text
                variant: "small"
                font.pixelSize: 13
                color: item.enabled ? Theme.text : Theme.textMuted
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        background: Rectangle {
            radius: Theme.rControl - 2
            color: item.highlighted && item.enabled ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
        }
    }
}
