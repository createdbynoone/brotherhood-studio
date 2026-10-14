import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

// One settings window for the whole app (⌘,).
Popup {
    id: root
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: 600
    height: Math.min(640, (Overlay.overlay ? Overlay.overlay.height : 700) - 80)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    Overlay.modal: Rectangle { color: Theme.scrim }
    background: Rectangle { radius: Theme.rPanel; color: Theme.surface; border.color: Theme.hairlineStrong }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.base } }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.fast } }

    FolderDialog {
        id: outputPicker
        title: "Choose the output folder"
        currentFolder: "file://" + prompt.outputPath
        onAccepted: prompt.setOutputPath(selectedFolder.toString().replace("file://", ""))
    }

    component Section: Column {
        property string heading: ""
        default property alias rows: body.data
        width: parent.width
        spacing: 4
        Txt { text: heading; variant: "overline"; muted: true; bottomPadding: 6 }
        Rectangle {
            width: parent.width
            height: body.implicitHeight
            radius: Theme.rCard
            color: Theme.raised
            border.color: Theme.hairline
            Column { id: body; width: parent.width }
        }
    }
    component Row_: Item {
        property string label: ""
        property string value: ""
        property bool mono: false
        default property alias actions: act.data
        width: parent.width
        height: 44
        Txt { id: l; text: label; anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter }
        Row {
            id: act
            anchors.right: parent.right; anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Txt {
                visible: value.length > 0
                text: value
                variant: mono ? "mono" : "small"
                dim: true
                elide: Text.ElideMiddle
                width: Math.min(implicitWidth, 280)
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Hairline { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: 14; opacity: 0.8 }
    }

    contentItem: Column {
        Item {
            width: parent.width; height: 56
            Txt { text: "Settings"; variant: "title"; anchors.left: parent.left; anchors.leftMargin: 20; anchors.verticalCenter: parent.verticalCenter }
            Btn { anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; kind: "ghost"; size: "s"; icon: "x"; onClicked: root.close() }
        }
        Hairline {}
        Flickable {
            width: parent.width; height: parent.height - 57
            contentWidth: width
            contentHeight: sections.implicitHeight + 40
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: StudioScrollBar {}
            Column {
                id: sections
                x: 20; y: 20
                width: parent.width - 40
                spacing: 22

                Section {
                    heading: "Output"
                    Row_ {
                        label: "Output folder"
                        value: studio.abbreviate(prompt.outputPath); mono: true
                        Btn { kind: "secondary"; size: "xs"; text: "Choose"; onClicked: outputPicker.open() }
                        Btn { kind: "ghost"; size: "xs"; text: "Open"; onClicked: studio.openFolder(prompt.outputPath) }
                    }
                    Row_ { label: "Higgsfield images are saved here, and the Sorter pane watches the same folder"; height: 38 }
                }
                Section {
                    heading: "BMP"
                    Row_ { label: "claude"; value: studio.toolPath("claude") || "not found"; mono: true }
                    Row_ { label: "higgsfield"; value: studio.toolPath("higgsfield") || "not found"; mono: true }
                    Row_ {
                        label: "Higgsfield credits"
                        value: prompt.hasCredits ? (prompt.credits % 1 === 0 ? prompt.credits + " cr" : prompt.credits.toFixed(1) + " cr") + (prompt.plan ? "  ·  " + prompt.plan : "") : "Not signed in"
                        Btn { kind: "secondary"; size: "xs"; text: "Refresh"; onClicked: prompt.refreshCredits() }
                        Btn { kind: "secondary"; size: "xs"; text: "Sign in"; onClicked: prompt.login() }
                    }
                    Row_ {
                        label: "Prompts stored"
                        value: prompt.memoryTotal + "  ·  " + prompt.memoryFired + " fired"
                        Btn { kind: "ghost"; size: "xs"; text: "Show file"; onClicked: studio.revealFile(studio.memoryPath()) }
                    }
                }
                Section {
                    heading: "Sorter"
                    Row_ {
                        label: "Watched folder"
                        value: studio.abbreviate(triage.watchPath); mono: true
                        Btn { kind: "secondary"; size: "xs"; text: "Rescan"; onClicked: triage.rescan() }
                    }
                    Row_ { label: "Library (dropped files)"; Btn { kind: "ghost"; size: "xs"; text: "Show"; onClicked: studio.openFolder(studio.libraryPath()) } }
                    Row_ {
                        label: "Missing files"; value: String(triage.missingCount)
                        Btn { kind: "secondary"; size: "xs"; text: "Purge"; enabled: triage.missingCount > 0; onClicked: triage.purgeMissing() }
                    }
                    Row_ { label: "Thumbnail cache"; Btn { kind: "secondary"; size: "xs"; text: "Clear"; onClicked: studio.clearThumbnails() } }
                    Row_ { label: "Database"; Btn { kind: "ghost"; size: "xs"; text: "Show file"; onClicked: studio.revealFile(studio.dbPath()) } }
                }
                Section {
                    heading: "Security"
                    Row_ { label: "Access key"; Btn { kind: "secondary"; size: "xs"; text: "Ask again next launch"; onClicked: studio.forgetKey() } }
                    Row_ { label: "The key is asked once on this Mac and remembered in the Keychain"; height: 38 }
                }
                Section {
                    heading: "Updates"
                    Row_ { label: "Version"; value: "v" + studio.version; mono: true }
                    Row_ {
                        label: "Latest release"
                        value: studio.updateAvailable ? "v" + studio.updateVersion + " available" : "Up to date"
                        Btn { kind: "secondary"; size: "xs"; text: "Check"; enabled: !studio.checkingUpdate; onClicked: studio.checkForUpdate() }
                        Btn { visible: studio.updateAvailable; kind: "primary"; size: "xs"; text: studio.updating ? "Downloading" : "Install and restart"; busy: studio.updating; enabled: !studio.updating; onClicked: studio.installUpdate() }
                    }
                }
            }
        }
    }
}
