import QtQuick
import QtQuick.Shapes

Item {
    id: ring
    property real fraction: 0
    width: 14; height: 14
    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.hairlineStrong; strokeWidth: 2; fillColor: "transparent"
            PathAngleArc { centerX: 7; centerY: 7; radiusX: 6; radiusY: 6; startAngle: 0; sweepAngle: 360 }
        }
        ShapePath {
            strokeColor: Theme.accent; strokeWidth: 2; fillColor: "transparent"; capStyle: ShapePath.RoundCap
            PathAngleArc { centerX: 7; centerY: 7; radiusX: 6; radiusY: 6; startAngle: -90; sweepAngle: 360 * Math.min(1, Math.max(0.001, ring.fraction)) }
        }
    }
}
