import QtQuick

// Engine, ratio, resolution and variations, then the fire button. Always pinned at the bottom of the pane.
Rectangle {
    id: root
    property var ctl: prompt              // the controller this dock fires (Prompts or Ecommerce)
    property bool choosable: true         // false: the engine and ratio are fixed (Ecommerce), so their pickers are hidden
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
                visible: root.choosable
                title: "Engine"
                options: ctl.providerNames.map((n, i) => ({ key: i, label: n }))
                current: ctl.providerIndex
                onPicked: key => ctl.providerIndex = key
            }
            LabeledSegment {
                visible: root.choosable
                title: "Ratio" + (ctl.aspectRatio === "auto" ? " (" + ctl.effectiveRatio + ")" : "")
                options: ctl.ratios.map(r => ({ key: r, label: r === "auto" ? "Auto" : r }))
                current: ctl.aspectRatio
                onPicked: key => ctl.aspectRatio = key
            }
            LabeledSegment {
                title: "Resolution"
                options: ctl.resolutions.map(r => ({ key: r, label: r.toUpperCase() }))
                current: ctl.resolution
                onPicked: key => ctl.resolution = key
            }
            LabeledSegment {
                visible: ctl.qualities.length > 0
                title: "Quality"
                options: ctl.qualities.map(q => ({ key: q, label: q === "xhigh" ? "XHIGH" : q === "medium" ? "MED" : q.toUpperCase() }))
                current: ctl.quality
                onPicked: key => ctl.quality = key
            }
            LabeledSegment {
                title: "Variations"
                options: [1, 2, 3, 4].map(n => ({ key: n, label: "×" + n }))
                current: ctl.variations
                onPicked: key => ctl.variations = key
            }
        }

        Btn {
            width: parent.width
            kind: "primary"; size: "l"
            readonly property int st: ctl.fireStatus
            // what this batch will cost (price per image x variations), from `higgsfield generate cost`
            readonly property string price: ctl.costKnown && ctl.fireCost > 0
                ? "  ·  " + (ctl.fireCost % 1 === 0 ? ctl.fireCost : ctl.fireCost.toFixed(2).replace(/0$/, "")) + " cr" : ""
            tint: st === 2 ? Theme.ok : st === 3 ? Theme.danger : "transparent"
            icon: st === 1 ? "" : st === 2 ? "check-circle" : st === 3 ? "warning-circle" : "lightning"
            iconWeight: "fill"
            busy: st === 1
            text: st === 1 ? "Generating" + (ctl.tasks > 1 ? " ×" + ctl.tasks : "") + (enabled ? "  ·  fire again" + price : "")
                : st === 2 ? "Done  ·  fire again" + price
                : st === 3 ? "Error  ·  retry" + price
                : "Fire to " + ctl.providerLabel + (ctl.variations > 1 ? "  ×" + ctl.variations : "") + price
            enabled: ctl.canFire
            hint: "⌘⇧↩"
            tip: st === 1 && enabled ? "The task keeps running in the background. Click to fire another batch in parallel" : "Send to Higgsfield"
            onClicked: ctl.fire()
        }
    }
}
