import QtQuick

// Garment photos, optional layout reference and notes, then the Analyze button.
Column {
    id: root
    spacing: Theme.s12

    Card {
        width: parent.width
        Column {
            width: parent.width
            spacing: 10
            Row {
                spacing: 8
                Txt { text: "Product photos"; variant: "heading" }
                Txt { text: "angles and details"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
            }
            DropTile {
                width: parent.width
                mode: "multi"
                placeholder: "Drop the product photos"
                hint: "front, back, other angles and close-ups · up to 8"
                files: ecom.photos
                onDropped: urls => ecom.addPhoto(urls)
                onRemoved: index => ecom.removePhoto(index)
            }
            Row {
                visible: ecom.photos.length > 0
                spacing: 8
                Chip {
                    visible: ecom.productType.length > 0
                    text: ecom.productType
                    active: true
                }
                Txt { visible: ecom.photoInfo.length > 0; text: ecom.photoInfo; variant: "mono"; muted: true; anchors.verticalCenter: parent.verticalCenter }
                Txt { visible: ecom.productType.length === 0 && ecom.photos.length > 0; text: "type is detected on Analyze"; variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter }
            }

            // Fire the back view too (garments only; caps, hats and bags have no back shot)
            Item {
                id: backRow
                visible: ecom.photos.length > 0
                width: parent.width
                height: 30
                opacity: ecom.backAllowed ? 1 : 0.45
                Row {
                    spacing: 10
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        width: 18; height: 18; radius: 5
                        anchors.verticalCenter: parent.verticalCenter
                        color: ecom.shootBack && ecom.backAllowed ? Theme.accent : "transparent"
                        border.color: ecom.shootBack && ecom.backAllowed ? Theme.accent : (backHover.hovered ? Theme.textDim : Theme.hairlineStrong)
                        Behavior on color { ColorAnimation { duration: Theme.fast } }
                        Icon { anchors.centerIn: parent; name: "check"; weight: "bold"; size: 12; color: "#0B0B0A"; visible: ecom.shootBack && ecom.backAllowed }
                    }
                    Txt { text: "Shoot the back too"; variant: "small"; anchors.verticalCenter: parent.verticalCenter }
                    Txt {
                        text: !ecom.backAllowed ? "caps, hats and bags have no back shot"
                            : ecom.shootBack ? "2 images per variation" : ""
                        variant: "caption"; muted: true; anchors.verticalCenter: parent.verticalCenter
                    }
                }
                HoverHandler { id: backHover; cursorShape: ecom.backAllowed ? Qt.PointingHandCursor : Qt.ArrowCursor }
                TapHandler { enabled: ecom.backAllowed; onTapped: ecom.shootBack = !ecom.shootBack }
            }
            Txt {
                visible: ecom.shootBack && ecom.backAllowed && (ecom.backStale || ecom.backSource.length > 0)
                width: parent.width
                wrapMode: Text.Wrap
                variant: "caption"
                color: ecom.backStale ? Theme.accent : Theme.textMuted
                text: ecom.backStale ? "Analyze again to write the back view"
                    : ecom.backSource === "photo" ? "Back view measured from your photos"
                    : "No photo shows the back: it is inferred from the front"
            }
        }
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
                    Txt { text: "Layout reference"; variant: "heading" }
                    Txt { text: "optional"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
                }
                Btn {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "ghost"; size: "xs"; icon: "folder-open"; text: "Style library"
                    tip: ecom.styleCount + " catalog photos set the house style. Add new drops here"
                    onClicked: { studio.openFolder(ecom.stylePath); ecom.refreshStyle() }
                }
            }
            DropTile {
                width: parent.width
                mode: "single"
                emptyHeight: 96; filledHeight: 180
                placeholder: "Drop a catalog shot to override the house style"
                hint: ecom.layoutRefs.length === 0 ? "Default: Brotherhood house style · " + ecom.styleCount + " photos" : "or click to browse"
                files: ecom.layoutRefs
                onDropped: urls => ecom.addLayoutRef(urls)
                onRemoved: ecom.removeLayoutRef()
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
                Txt { text: "Notes"; variant: "heading" }
                Txt { text: "optional"; variant: "caption"; muted: true; anchors.baseline: parent.children[0].baseline }
            }
            TextBox {
                width: parent.width
                implicitHeight: 84
                placeholder: "Details to protect, exact colors, anything the photo does not show"
                text: ecom.notes
                onEdited: if (ecom.notes !== text) ecom.notes = text
            }
        }
    }

    Btn {
        width: parent.width
        kind: "primary"; size: "l"
        icon: ecom.analyzeStatus === 1 ? "" : "magic-wand"; iconWeight: "bold"
        text: ecom.analyzeStatus === 1 ? "Reading the photos" : ecom.json.length > 0 ? "Analyze again" : "Analyze photos"
        busy: ecom.analyzeStatus === 1
        enabled: ecom.canAnalyze
        hint: "⌘↩"
        tip: ecom.photos.length > 0 ? "Opus detects the product and writes the JSON" : "Add the product photos first"
        onClicked: ecom.analyze()
    }

    Rectangle {
        visible: ecom.analyzeError.length > 0
        width: parent.width
        height: errText.implicitHeight + 20
        radius: Theme.rControl
        color: Theme.alpha(Theme.danger, 0.08)
        border.color: Theme.alpha(Theme.danger, 0.3)
        Txt {
            id: errText
            anchors.fill: parent; anchors.margins: 10
            text: ecom.analyzeError
            variant: "small"; color: Theme.danger
            wrapMode: Text.Wrap
        }
    }
}
