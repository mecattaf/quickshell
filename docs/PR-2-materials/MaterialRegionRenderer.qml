import QtQuick
import Quickshell.WebEngine

/// Renders dynamic MaterialSurface instances based on registered material regions.
/// This component listens to a ShellBridge's materialRegionsChanged signal and
/// dynamically creates/updates MaterialSurface instances for each registered region.
///
/// Example usage:
/// ```qml
/// Item {
///     MaterialRegionRenderer {
///         anchors.fill: parent
///         z: -1  // Render beneath web content
///         bridge: shellChannel.shell
///     }
///     ShellWebEngineView {
///         anchors.fill: parent
///         webChannel: shellChannel
///     }
/// }
/// ```
///
/// JavaScript can register regions via:
/// ```javascript
/// shell.registerMaterialRegion({
///     id: "my-region",
///     x: 10, y: 10, width: 200, height: 100,
///     materialLevel: 3,
///     cornerRadius: 12,
///     tintColor: "#40000000"
/// });
/// ```
Item {
    id: root

    /// The ShellBridge instance to listen for material regions.
    property ShellBridge bridge: null

    /// Default material level for regions that don't specify one.
    property int defaultMaterialLevel: 3

    /// Default corner radius for regions that don't specify one.
    property real defaultCornerRadius: 12

    /// Default tint color for regions that don't specify one.
    property color defaultTintColor: "#40000000"

    /// Default opacity for regions that don't specify one.
    property real defaultOpacity: 1.0

    Repeater {
        model: root.bridge ? root.bridge.materialRegions : []

        delegate: MaterialSurface {
            required property var modelData

            x: modelData.x ?? 0
            y: modelData.y ?? 0
            width: modelData.width ?? 100
            height: modelData.height ?? 100

            materialLevel: modelData.materialLevel ?? root.defaultMaterialLevel
            cornerRadius: modelData.cornerRadius ?? root.defaultCornerRadius
            tintColor: modelData.tintColor ?? root.defaultTintColor
            opacity: modelData.opacity ?? root.defaultOpacity
        }
    }
}
