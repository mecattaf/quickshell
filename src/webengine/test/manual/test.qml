//@ pragma EnableWebEngine
import QtQuick
import Quickshell
import Quickshell.Wayland
import Quickshell.WebEngine

/// Test configuration demonstrating WebEngine integration.
/// Run with: quickshell -c src/webengine/test/manual/test.qml
ShellRoot {
    PanelWindow {
        id: panel

        anchors {
            bottom: true
            left: true
            right: true
        }

        height: 64
        color: "transparent"

        ShellWebEngineView {
            anchors.fill: parent
            url: Qt.resolvedUrl("panel.html")

            webChannel: ShellChannel {
                id: channel
            }
        }
    }

    // Optional: Floating widget to test transparency
    PanelWindow {
        id: widget

        anchors {
            top: true
            right: true
        }

        margins {
            top: 20
            right: 20
        }

        width: 300
        height: 200
        color: "transparent"

        ShellWebEngineView {
            anchors.fill: parent
            url: Qt.resolvedUrl("widget.html")

            webChannel: ShellChannel {}
        }
    }
}
