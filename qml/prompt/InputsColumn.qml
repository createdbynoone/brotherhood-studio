import QtQuick
import QtQuick.Dialogs

// Reference, product, background, camera and brief, then the Generate button.
Column {
    id: root
    spacing: Theme.s12

    function labelRow(title, hint) { return title }

    Card {
        width: parent.width
        Column {
            width: parent.width
            spacing: 10
            Row {
                spacing: 8
                Txt { text: "Reference"; variant: "heading" }
                Txt { text: "one image · scene, light, mood"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
            }
            DropTile {
                width: parent.width
                mode: "single"
                placeholder: "Drop the reference image"
                files: prompt.refs
                onDropped: urls => prompt.addRefs(urls)
                onRemoved: prompt.removeRef(0)
            }
            Item {
                width: parent.width
                height: prompt.refs.length > 0 ? 58 : 0
                visible: height > 0
                clip: true
                Behavior on height { NumberAnimation { duration: Theme.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.curve } }
                Column {
                    width: parent.width
                    spacing: 6
                    Txt { text: "What to take from it"; variant: "label"; dim: true }
                    Field {
                        id: specs
                        width: parent.width
                        placeholder: "Light, angle, setting"
                        text: prompt.refSpecs
                        onEdited: prompt.refSpecs = text
                    }
                }
            }
        }
    }

    Card {
        width: parent.width
        Column {
            width: parent.width
            spacing: 10
            Row {
                spacing: 8
                Txt { text: "Product"; variant: "heading" }
                Txt { text: "Brotherhood garment"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
            }
            DropTile {
                width: parent.width
                mode: "multi"
                placeholder: "Drop the product photos"
                files: prompt.products
                onDropped: urls => prompt.addProducts(urls)
                onRemoved: index => prompt.removeProduct(index)
            }
        }
    }

    FileDialog {
        id: libraryPicker
        title: "Look library"
        currentFolder: Theme.fileUrl(prompt.lookPath)
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp *.heic)"]
        onAccepted: prompt.addBackground(selectedFiles)
    }

    Card {
        width: parent.width
        Column {
            width: parent.width
            spacing: 10
            Item {
                width: parent.width
                height: 24
                Row {
                    spacing: 8
                    anchors.verticalCenter: parent.verticalCenter
                    Txt { text: "Background"; variant: "heading" }
                    Txt { text: "optional · recommended"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
                }
                Btn {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "ghost"; size: "xs"; icon: "folder-open"; text: "Library"
                    tip: "Pick a location from your look library"
                    onClicked: libraryPicker.open()
                }
            }
            DropTile {
                width: parent.width
                mode: "single"
                emptyHeight: 96; filledHeight: 180
                placeholder: "Drop a photo of the real location"
                hint: prompt.backgrounds.length === 0 ? "Without one, the scene is invented from the brief" : "or click to browse"
                files: prompt.backgrounds
                onDropped: urls => prompt.addBackground(urls)
                onRemoved: prompt.removeBackground()
            }
            Txt {
                visible: prompt.backgrounds.length > 0
                width: parent.width
                text: "The garment is placed inside this exact place and lit by its light."
                variant: "caption"; muted: true; wrapMode: Text.Wrap
            }
        }
    }

    CameraCard { width: parent.width }

    Card {
        width: parent.width
        Column {
            width: parent.width
            spacing: 10
            Item {
                width: parent.width
                height: 24
                Row {
                    spacing: 8
                    anchors.verticalCenter: parent.verticalCenter
                    Txt { text: "Brief"; variant: "heading" }
                    Txt { text: "optional · Sonnet writes it if empty"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
                }
                Btn {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "ghost"; size: "xs"; icon: "sparkle"; iconWeight: "fill"; text: "Auto-write"
                    busy: prompt.briefLoading
                    enabled: prompt.canAutoBrief
                    tip: "Sonnet writes the brief from the reference, product and specs"
                    onClicked: prompt.autoWriteBrief()
                }
            }
            TextBox {
                width: parent.width
                implicitHeight: 104
                placeholder: "Leave empty and Sonnet writes it from the reference and the product"
                text: prompt.brief
                onEdited: if (prompt.brief !== text) prompt.brief = text
            }
        }
    }

    Btn {
        width: parent.width
        kind: "primary"; size: "l"
        icon: prompt.generateStatus === 1 ? "" : "magic-wand"; iconWeight: "bold"
        text: prompt.generateStatus === 1 ? (prompt.generateStage.length ? prompt.generateStage : "Generating") : "Generate prompt"
        busy: prompt.generateStatus === 1
        enabled: prompt.canGenerate && prompt.generateStatus !== 1 && !prompt.briefLoading
        hint: "⌘↩"
        tip: prompt.canGenerate ? "Write the prompt with Opus" : "Add a reference and a product first"
        onClicked: prompt.generate()
    }

    Rectangle {
        visible: prompt.generateError.length > 0
        width: parent.width
        height: errText.implicitHeight + 20
        radius: Theme.rControl
        color: Theme.alpha(Theme.danger, 0.08)
        border.color: Theme.alpha(Theme.danger, 0.3)
        Txt {
            id: errText
            anchors.fill: parent; anchors.margins: 10
            text: prompt.generateError
            variant: "small"; color: Theme.danger
            wrapMode: Text.Wrap
        }
    }
}
