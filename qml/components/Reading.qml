import QtQuick

// Selectable, read-only body text with comfortable leading (rich text carries the line height).
TextEdit {
    property string plain: ""
    property real leading: 1.5
    function esc(s) { return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/\n/g, "<br>") }
    text: "<div style='line-height:" + Math.round(leading * 100) + "%'>" + esc(plain) + "</div>"
    textFormat: TextEdit.RichText
    readOnly: true
    selectByMouse: true
    wrapMode: TextEdit.Wrap
    font.family: Theme.sans
    font.pixelSize: 14
    color: Theme.text
    selectionColor: Theme.alpha(Theme.accent, 0.45)
    selectedTextColor: Theme.text
}
