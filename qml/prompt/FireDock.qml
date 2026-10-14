import QtQuick

// Engine, ratio, resolution and variations, then the fire button. Always pinned at the bottom of the pane.
Rectangle {
    id: root
    color: Theme.surface
    implicitHeight: col.implicitHeight + Theme.s12 * 2
    Hairline { anchors.top: parent.top }

    Column {
        id: col
        x: Theme.s16; y: Theme.s12
        width: parent.width - Theme.s16 * 2
        spacing: Theme.s12

        Flow {
            width: parent.width
            spacing: Theme.s16
            LabeledSegment {
                title: "Engine"
                options: prompt.providerNames.map((n, i) => ({ key: i, label: n }))
                current: prompt.providerIndex
                onPicked: key => prompt.providerIndex = key
            }
            LabeledSegment {
                title: "Ratio"
                options: prompt.ratios.map(r => ({ key: r, label: r }))
                current: prompt.aspectRatio
                onPicked: key => prompt.aspectRatio = key
            }
            LabeledSegment {
                title: "Resolution"
                options: prompt.resolutions.map(r => ({ key: r, label: r.toUpperCase() }))
                current: prompt.resolution
                onPicked: key => prompt.resolution = key
            }
            LabeledSegment {
                title: "Variations"
                options: [1, 2, 3, 4].map(n => ({ key: n, label: "×" + n }))
                current: prompt.variations
                onPicked: key => prompt.variations = key
            }
        }

        Btn {
            width: parent.width
            kind: "primary"; size: "l"
            readonly property int st: prompt.fireStatus
            tint: st === 2 ? Theme.ok : st === 3 ? Theme.danger : "transparent"
            icon: st === 1 ? "" : st === 2 ? "check-circle" : st === 3 ? "warning-circle" : "lightning"
            iconWeight: "fill"
            busy: st === 1
            text: st === 1 ? "Generating" + (prompt.tasks > 1 ? " ×" + prompt.tasks : "") + (enabled ? "  ·  fire again" : "")
                : st === 2 ? "Done  ·  fire again"
                : st === 3 ? "Error  ·  retry"
                : "Fire to " + prompt.providerLabel + (prompt.variations > 1 ? "  ×" + prompt.variations : "")
            enabled: prompt.canFire
            hint: "⌘⇧↩"
            tip: st === 1 && enabled ? "The task keeps running in the background. Click to fire another batch in parallel" : "Send the prompt to Higgsfield"
            onClicked: prompt.fire()
        }
    }
}
