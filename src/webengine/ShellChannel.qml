import QtQuick
import QtWebChannel
import Quickshell.WebEngine

/// Convenience wrapper for WebChannel with a pre-configured ShellBridge.
/// The bridge is automatically registered as "shell" for JavaScript access.
///
/// Property updates from C++/QML to JavaScript are batched for efficiency.
/// The default interval is 50ms. For real-time updates (e.g., animations),
/// reduce this interval or use blockUpdates.
///
/// Example:
/// ```qml
/// ShellWebEngineView {
///     url: "panel.html"
///     webChannel: ShellChannel {
///         id: channel
///         // Access the bridge: channel.shell.darkMode
///     }
/// }
/// ```
///
/// JavaScript usage:
/// ```javascript
/// new QWebChannel(qt.webChannelTransport, function(channel) {
///     var shell = channel.objects.shell;
///     shell.log("info", "Hello from web!");
/// });
/// ```
WebChannel {
    id: channel

    /// Interval in ms for batching property updates to JavaScript.
    /// Lower values = more responsive, higher CPU. Default: 50ms.
    /// Set to 0 for immediate updates (not recommended for frequent changes).
    property alias propertyUpdateInterval: channel.propertyUpdateInterval

    /// The ShellBridge instance registered with this channel.
    /// Access theme properties and logging from QML or JavaScript.
    readonly property alias shell: shellBridge

    // Default batching interval of 50ms balances responsiveness and efficiency
    Component.onCompleted: {
        channel.propertyUpdateInterval = 50;
    }

    ShellBridge {
        id: shellBridge
        WebChannel.id: "shell"
    }

    registeredObjects: [shellBridge]
}

