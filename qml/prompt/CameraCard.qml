import QtQuick

// Camera settings of a professional shoot (body, lens, aperture, shutter, ISO, white balance, color look, flash,
// height, distance, optics). Everything left on Auto is chosen by Opus; the choices travel with the prompt request.
Card {
    id: root
    property bool open: false
    readonly property bool active: Object.keys(prompt.camera).length > 0

    Column {
        width: parent.width
        spacing: 10

        Item {
            width: parent.width
            height: 24
            Row {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter
                Txt { text: "Camera"; variant: "heading" }
                Txt { text: root.active ? "your settings" : "auto · Opus chooses"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
            }
            Btn {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                kind: "ghost"; size: "xs"; icon: root.open ? "caret-up" : "sliders-horizontal"; text: root.open ? "Less" : "Fine-tune"
                tip: "Body, lens, aperture, shutter, ISO, white balance, color look, flash, height and distance"
                onClicked: root.open = !root.open
            }
        }

        FlowRow {
            width: parent.width
            Chip { text: "Auto"; active: !root.active; onClicked: prompt.clearCamera() }
            Repeater {
                model: prompt.cameraPresets
                delegate: Chip {
                    required property var modelData
                    text: modelData.label
                    active: prompt.cameraPreset === modelData.id
                    onClicked: prompt.applyCameraPreset(modelData.id)
                }
            }
        }

        Txt {
            visible: root.active
            width: parent.width
            text: prompt.cameraSummary
            variant: "mono"; muted: true; wrapMode: Text.Wrap
        }

        Item {
            width: parent.width
            height: root.open ? fine.implicitHeight : 0
            visible: height > 0
            clip: true
            Behavior on height { NumberAnimation { duration: Theme.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curve } }
            Column {
                id: fine
                width: parent.width
                spacing: 12
                Repeater {
                    model: prompt.cameraSettings
                    delegate: Column {
                        id: row
                        required property var modelData
                        width: fine.width
                        spacing: 6
                        Txt { text: row.modelData.title; variant: "label"; muted: true }
                        FlowRow {
                            width: parent.width
                            Repeater {
                                model: row.modelData.options
                                delegate: Chip {
                                    required property string modelData
                                    text: modelData
                                    active: prompt.camera[row.modelData.id] === modelData
                                    onClicked: prompt.setCameraValue(row.modelData.id, modelData)
                                }
                            }
                        }
                    }
                }
                Txt {
                    width: parent.width
                    text: "Tap a chosen value again to return it to Auto."
                    variant: "caption"; muted: true; wrapMode: Text.Wrap
                }
            }
        }
    }
}
