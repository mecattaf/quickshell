import QtQuick
import QtWebEngine
import QtWebChannel
import Quickshell.WebEngine

/// High-level component combining MaterialSurface with ShellWebEngineView.
/// Use this for the common pattern of glass-behind-web-content.
///
/// Example:
/// ```qml
/// WebSurface {
///     url: "file:///path/to/panel.html"
///     materialLevel: 3
///     tintColor: "#40000000"
///     cornerRadius: 12
///     webChannel: ShellChannel {}
/// }
/// ```
Item {
    id: root

    /// URL to load in the web view.
    property alias url: webView.url

    /// WebChannel for JavaScript/C++ communication.
    property alias webChannel: webView.webChannel

    /// Material intensity level (1-5).
    property alias materialLevel: materialSurface.materialLevel

    /// Tint color for the material surface.
    property alias tintColor: materialSurface.tintColor

    /// Corner radius for rounded edges.
    property alias cornerRadius: materialSurface.cornerRadius

    /// Opacity of the material surface (0.0 = fully transparent, 1.0 = fully opaque).
    property alias materialOpacity: materialSurface.opacity

    /// Whether the web view is currently active.
    property alias active: webView.active

    /// When true, hidden views enter Discarded state (releases most resources, will reload).
    /// When false (default), hidden views enter Frozen state (suspends JS, retains DOM).
    property alias discardWhenHidden: webView.discardWhenHidden

    /// Enable dynamic material regions controlled from JavaScript via shell.registerMaterialRegion().
    /// Requires webChannel to be a ShellChannel. Default: false.
    property bool enableDynamicRegions: false

    /// Whether the web page is currently loading.
    readonly property alias loading: webView.loading

    /// The title of the loaded web page.
    readonly property alias title: webView.title

    /// Access to the underlying ShellWebEngineView.
    readonly property alias webView: webView

    /// Access to the underlying MaterialSurface.
    readonly property alias materialSurface: materialSurface

    // Static material surface renders beneath web content
    MaterialSurface {
        id: materialSurface
        anchors.fill: parent
        z: -1
        materialLevel: 3
        tintColor: "#40000000"
        cornerRadius: 12
    }

    // Dynamic material regions from JavaScript (when enabled)
    // These render above the static material surface but below web content
    Loader {
        active: root.enableDynamicRegions && root.webChannel && root.webChannel.shell
        anchors.fill: parent
        z: -0.5  // Between static MaterialSurface (-1) and WebEngineView (0)
        sourceComponent: MaterialRegionRenderer {
            bridge: root.webChannel.shell
        }
    }

    // Web content on top (z: 0 explicit for clarity, must be above MaterialSurface z: -1)
    ShellWebEngineView {
        id: webView
        anchors.fill: parent
        z: 0
    }
}
