import QtQuick

// Category / product chip.
Rectangle {
    id: root
    property string text: ""
    property bool active: false
    signal clicked()
    signal rightClicked()

    implicitHeight: 26
    implicitWidth: label.implicitWidth + 20
    radius: Theme.rControl
    color: active ? Theme.accentSoft : hover.hovered ? Theme.elevated : Theme.alpha(Theme.raised, 0.7)
    border.color: active ? Theme.alpha(Theme.accent, 0.6) : Theme.hairlineStrong
    Behavior on color { ColorAnimation { duration: Theme.fast } }
    Txt {
        id: label
        anchors.centerIn: parent
        text: root.text
        variant: "label"
        font.pixelSize: 12
        color: root.active ? Theme.accent : Theme.text
    }
    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { acceptedButtons: Qt.LeftButton; onTapped: root.clicked() }
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: root.rightClicked() }
}
