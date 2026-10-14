import QtQuick
import QtQuick.Controls.Basic

// Multi-line input with its own scrolling.
Rectangle {
    id: root
    property alias text: edit.text
    property string placeholder: ""
    property int fontSize: 13
    property alias editItem: edit
    signal edited()

    implicitHeight: 96
    radius: Theme.rControl
    color: Theme.alpha(Theme.bg, 0.7)
    border.color: edit.activeFocus ? Theme.alpha(Theme.accent, 0.7) : hover.hovered ? Theme.hairlineStrong : Theme.hairline
    Behavior on border.color { ColorAnimation { duration: Theme.fast } }

    function forceFocus() { edit.forceActiveFocus() }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 9
        contentWidth: width
        contentHeight: edit.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: StudioScrollBar {}

        TextEdit {
            id: edit
            width: flick.width
            wrapMode: TextEdit.Wrap
            font.family: Theme.sans
            font.pixelSize: root.fontSize
            color: Theme.text
            selectionColor: Theme.alpha(Theme.accent, 0.45)
            selectedTextColor: Theme.text
            selectByMouse: true
            onTextChanged: root.edited()
            onCursorRectangleChanged: {
                if (cursorRectangle.y < flick.contentY) flick.contentY = cursorRectangle.y
                else if (cursorRectangle.y + cursorRectangle.height > flick.contentY + flick.height)
                    flick.contentY = cursorRectangle.y + cursorRectangle.height - flick.height
            }
            Txt {
                visible: edit.text.length === 0 && !edit.preeditText
                text: root.placeholder
                muted: true
                font.pixelSize: root.fontSize
                width: parent.width
                wrapMode: Text.Wrap
            }
        }
    }
    HoverHandler { id: hover }
}
