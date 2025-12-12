name = "Quickshell.WebEngine"
description = "QtWebEngine integration for web-based shell UIs"
headers = [
	"shellwebengineview.hpp",
	"shellbridge.hpp",
]
-----

# Quickshell.WebEngine

This module provides QtWebEngine integration for building web-based desktop shell UIs.
It enables developers to create shell components using HTML/CSS/JavaScript while
leveraging Quickshell's native system integration.

## Requirements

Add the following pragma at the top of your shell.qml file:

```qml
//@ pragma EnableWebEngine
```

## Components

### ShellWebEngineView
A WebEngineView wrapper with shell-optimized defaults:
- Transparent background enabled by default
- Lifecycle state management (Active/Frozen/Discarded)
- Automatic visibility-based resource management

### ShellBridge
Bridge object for QtWebChannel communication between C++ and JavaScript.
Exposes logging, theme properties, and material region APIs.

### ShellChannel
WebChannel convenience wrapper with pre-configured ShellBridge.

## Performance Warnings

> [!CAUTION]
> **Do NOT set `layer.enabled: true` on ShellWebEngineView** unless absolutely
> necessary. This forces render-to-texture, severely impacting performance:
> - Significant GPU overhead
> - Increased VRAM usage (~200MB+)
> - Potential frame drops
>
> If you need shader effects on web content, consider alternatives like
> CSS-based effects within the HTML content itself.

> [!WARNING]
> Each WebEngineView instance consumes approximately 200MB of RAM due to
> Chromium's multi-process architecture. Use the `discardWhenHidden` property
> aggressively for views that are rarely shown.

## DevTools Debugging

To enable Chrome DevTools for debugging web content:

```bash
QS_WEBENGINE_DEVTOOLS_PORT=9222 quickshell
```

Then open `http://localhost:9222` in a Chromium-based browser.

## Transparency Requirements

For proper transparency, your HTML content must have:

```css
html, body {
    background: transparent;
}
```

The ShellWebEngineView sets transparent background automatically, but HTML
content with opaque backgrounds will still appear opaque.

## Example Usage

```qml
//@ pragma EnableWebEngine
import QtQuick
import Quickshell
import Quickshell.Wayland
import Quickshell.WebEngine

ShellRoot {
    PanelWindow {
        anchors { bottom: true; left: true; right: true }
        height: 48
        color: "transparent"
        
        ShellWebEngineView {
            anchors.fill: parent
            url: Qt.resolvedUrl("panel.html")
            webChannel: ShellChannel {}
        }
    }
}
```

## JavaScript API

Connect to the shell via QtWebChannel:

```javascript
new QWebChannel(qt.webChannelTransport, function(channel) {
    var shell = channel.objects.shell;
    
    // Logging
    shell.log("info", "Hello from web!");
    
    // Theme properties (reactive)
    console.log("Dark mode:", shell.darkMode);
    shell.darkModeChanged.connect(function() {
        console.log("Theme changed!");
    });
    
    // Get all theme tokens
    shell.getThemeTokens(function(tokens) {
        console.log(tokens.colors.accent);
    });
});
```
