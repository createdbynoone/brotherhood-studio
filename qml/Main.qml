import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

ApplicationWindow {
    id: win
    visible: true
    readonly property var gs: (typeof grabSize === "string" && grabSize.indexOf("x") > 0) ? grabSize.split("x").map(Number) : [1560, 940]
    width: gs[0]; height: gs[1]
    minimumWidth: studio.layout === "split" ? 1100 : 860
    minimumHeight: 660
    color: Theme.bg
    title: "Brotherhood Studio"

    property bool splashDone: false

    // Native title-bar drag (Mac.installTitleBarDrag): which points are empty space?
    function titleBarHit(x, y) {
        if (!splashDone || !studio.unlocked) return y < 60
        return studioLoader.item ? studioLoader.item.dragAt(x, y) : false
    }
    Component.onCompleted: { Theme.reduceMotion = studio.reduceMotion; Theme.brand = brandFamily }

    Loader {
        id: studioLoader
        anchors.fill: parent
        active: win.splashDone && studio.unlocked
        sourceComponent: StudioView {}
        opacity: active ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.slow } }
    }
    LockScreen {
        anchors.fill: parent
        visible: win.splashDone && !studio.unlocked
    }
    Splash {
        anchors.fill: parent
        visible: !win.splashDone
        onFinished: win.splashDone = true
    }
}
