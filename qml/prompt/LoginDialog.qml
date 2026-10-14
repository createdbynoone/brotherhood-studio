import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: root
    modal: true
    anchors.centerIn: Overlay.overlay
    width: 400
    padding: 22
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle { color: Theme.scrim }
    background: Rectangle { radius: Theme.rPanel; color: Theme.surface; border.color: Theme.hairlineStrong }

    contentItem: Column {
        spacing: 14
        Row {
            spacing: 10
            Icon { name: "key"; size: 18; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
            Txt { text: "Higgsfield sign in"; variant: "title"; anchors.verticalCenter: parent.verticalCenter }
        }
        Txt {
            width: parent.width
            wrapMode: Text.Wrap
            dim: true
            text: "Finish signing in in the browser window that just opened, or run higgsfield auth login in a terminal."
        }
        Row {
            anchors.right: parent.right
            Btn { kind: "primary"; size: "m"; text: "Done"; onClicked: root.close() }
        }
    }
}
