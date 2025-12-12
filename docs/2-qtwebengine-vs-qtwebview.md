# QtWebEngine and QtWebView architecture for desktop shell integration

Integrating web content into a Quickshell-based desktop shell requires navigating two fundamentally different Qt web modules—**QtWebView** (a thin abstraction over platform-native browsers) and **QtWebEngine** (full Chromium embedding)—each with distinct initialization requirements, process models, and scene graph integration strategies. This documentation provides the technical foundation for building a QtWebEngine-powered shell that properly handles layer-shell surfaces, transparency, and efficient resource management on Wayland.

---

## QtWebView's plugin-based platform abstraction

QtWebView takes a lightweight approach by abstracting over **platform-native web engines** rather than bundling its own. The module uses Qt's `QFactoryLoader` to dynamically load backend plugins at runtime based on the target platform.

### Platform backend selection

| Platform | Backend Engine | Selection Mechanism |
|----------|---------------|---------------------|
| **macOS/iOS** | WKWebView (WebKit) | Darwin plugin via `qdarwinwebview.mm` |
| **Android** | `android.webkit.WebView` | JNI integration |
| **Windows/Linux** | QtWebEngine (Chromium) | Falls back to webengine plugin |

The factory logic in `qwebviewfactory.cpp` checks the `QT_WEBVIEW_PLUGIN` environment variable first, defaulting to `"native"` if unset. On platforms without native web views (Windows/Linux), QtWebView essentially becomes a wrapper around QtWebEngine—making it unsuitable as a "lighter" alternative on desktop.

### Class hierarchy and QML boundary

The architecture separates public API from backend implementation through three layers:

```
QWebViewPlugin (Abstract Factory Interface)
    └── creates QWebViewPrivate (Abstract Backend)
             ├── QDarwinWebViewPrivate (macOS/iOS)
             ├── QAndroidWebViewPrivate (Android)  
             └── QWebEngineWebViewPrivate (Windows/Linux)

QWebView (Public C++ API) ──wraps──> QWebViewPrivate
QQuickWebView (QML Component) ──exposes as──> WebView { }
```

The QML `WebView` type exposes properties like `url`, `title`, `loadProgress`, and methods including `goBack()`, `goForward()`, `reload()`, and critically `runJavaScript(string script, variant callback)` for JavaScript bridge communication.

### Foreign window embedding limits compositing

QtWebView renders via **foreign window embedding** using `QWindow::fromWinId()` to wrap the native web view handle, then parenting it to the Qt window hierarchy. This approach means the web content exists as a separate OS window layered on top—**not integrated into the Qt Quick scene graph**.

The critical limitation: *"Due to platform limitations, overlapping the WebView with other QML components is not supported."* Z-ordering conflicts occur because the native web view doesn't participate in Qt's composition pipeline. For desktop shells requiring overlapping UI elements or shader effects on web content, QtWebView is architecturally unsuitable.

---

## QtWebEngine's Chromium integration architecture

QtWebEngine embeds the full Chromium engine through several bridge layers that adapt Chromium's Content API to Qt's application model. Unlike QtWebView, it renders web content **into the Qt Quick scene graph as texture nodes**, enabling proper compositing with other QML elements.

### Key integration bridge classes

| Bridge Class | Responsibility |
|-------------|----------------|
| `WebContentsAdapter` | Primary Qt ↔ `content::WebContents` connection |
| `WebContentsDelegateQt` | Routes Chromium events to Qt signals |
| `RenderWidgetHostViewQt` | Manages rendering integration |
| `ContentBrowserClientQt` | Customizes browser process behavior |
| `OzonePlatformQt` | Linux platform abstraction for Wayland/X11 |

QtWebEngine uses Chromium's **Blink** (rendering), **V8** (JavaScript), **Skia** (2D graphics), and the full **compositor** stack. Notably, it does **not** use Qt's networking—Chromium's own network stack handles all requests.

### Multi-process architecture

QtWebEngine inherits Chromium's security-focused process isolation:

**Browser Process** (main application): Manages lifecycle, handles GPU thread (`Chrome_InProcGPUThread`), coordinates other processes. Only one instance runs.

**Renderer Processes** (`QtWebEngineProcess`): Separate executables running Blink and V8, sandboxed for security. Roughly one per tab/view. Located in `QTDIR/libexec` (Linux) or `QTDIR\bin` (Windows).

**GPU Thread** (in-process): Unlike Chrome's separate GPU process, QtWebEngine uses an in-process thread. Renderer processes stream GL commands via `GLES2CommandEncoder` → `CommandBuffer` → `GLES2CommandDecoder` in the browser process.

Inter-process communication uses **Mojo**, Chromium's async messaging API. This architecture means renderer crashes don't bring down the shell, but memory overhead is significant—expect **200MB+ per WebEngineView**.

### Scene graph texture integration

The rendering pipeline flows through `DelegatedFrameNode`:

1. Chromium renders → uploads textures (GPU) or bitmaps (software fallback)
2. `RenderWidgetHostViewQt::OnSwapCompositorFrame` receives frame data
3. Frame data saved, scene graph repaint triggered
4. `DelegatedFrameNode::commit` creates `QSGNode` hierarchy from Chromium's RenderPasses/DrawQuads
5. `DelegatedFrameNode::preprocess` fetches textures via GPU thread
6. Qt scene graph renders the assembled nodes

**Qt 6.5+ RHI integration** enables non-OpenGL backends:
- **Windows**: D3D11 via DXGI shared buffers
- **macOS**: Metal via IOSurface
- **Linux**: GBM (Generic Buffer Management) buffers

This removes dependence on buggy Windows OpenGL drivers and enables hardware video acceleration on Linux.

---

## QtWebChannel bridges C++ to JavaScript

QtWebChannel provides the mechanism for bidirectional communication between shell logic (C++/QML) and web content (JavaScript). It uses Qt's meta-object system to introspect registered `QObject`s and serialize their API as JSON.

### Transport mechanisms

**In QtWebEngine**, the transport is exposed automatically as `qt.webChannelTransport` in JavaScript, using Chromium's internal IPC rather than WebSockets. This is more efficient than network-based transports.

```javascript
new QWebChannel(qt.webChannelTransport, function(channel) {
    var shellApi = channel.objects.shellApi;
    shellApi.openApplication("firefox");
});
```

The C++ side implements `QWebChannelAbstractTransport` with `sendMessage()` slot and `messageReceived()` signal. For QtWebEngine, `QWebEnginePage::setWebChannel()` handles setup.

### Q_INVOKABLE and Q_PROPERTY mapping

**Method exposure** (all calls are asynchronous):
```cpp
// C++
Q_INVOKABLE QString getSystemInfo(const QString &key);

// JavaScript - callback receives return value
shellApi.getSystemInfo("hostname", function(result) {
    console.log(result);
});
```

**Property mapping** with change notifications:
```cpp
// C++
Q_PROPERTY(bool darkMode READ darkMode NOTIFY darkModeChanged)

// JavaScript - cached locally, updated via NOTIFY signal
console.log(shellApi.darkMode);
shellApi.darkModeChanged.connect(function() {
    console.log("Theme changed to:", shellApi.darkMode);
});
```

Properties are **cached on the JavaScript side** with updates batched (default **50ms** interval via `propertyUpdateInterval`). For high-frequency updates, use `channel->setBlockUpdates(true/false)` to batch changes.

### Type conversion boundaries

Only JSON-serializable types work automatically: strings, numbers, booleans, arrays, objects. For custom types, register converters via `QMetaType::registerConverter<MyType, QJsonValue>()`. Overloaded methods can be explicitly selected: `foo["foo(int)"](42)`.

---

## Initialization order requirements

The most critical integration detail: **`QtWebEngineQuick::initialize()` must be called before `QGuiApplication` is constructed**.

### What initialization actually does

1. **OpenGL context sharing**: Sets up contexts shareable between GUI and renderer processes (equivalent to `Qt::AA_ShareOpenGLContexts`)
2. **Chromium command-line setup**: Passes argc/argv to Chromium's initialization
3. **Process model configuration**: Prepares multi-process architecture
4. **Cleanup registration**: Registers `qAddPostRoutine()` for shutdown

```cpp
int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("MyShell");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QtWebEngineQuick::initialize();  // BEFORE QGuiApplication
    QGuiApplication app(argc, argv);
    // ...
}
```

If called late, Qt emits a deprecation warning and context sharing may fail, causing rendering issues or crashes.

### The qArgC = 0 problem in Quickshell

Quickshell crash reports reveal a critical issue: Chromium's `CommandLine::Init()` requires **at least one argument** (argv[0] must be the program name). When `qArgC = 0`, Chromium receives no program path, causing:

- Invalid `browser-subprocess-path`
- Renderer process spawn failures
- Crashes in `content::ContentMainRunnerImpl::Initialize`

The fix requires ensuring `argc >= 1` before WebEngine initialization, with `argv[0]` pointing to the executable path.

### Dynamic loading patterns for optional WebEngine

For shells that want WebEngine as optional:

```cpp
// Before QGuiApplication - tricky timing
QLibrary webEngineLib("Qt6WebEngineQuick");
if (webEngineLib.load()) {
    typedef void (*InitFunc)();
    // Symbol mangling varies by compiler
    InitFunc initialize = (InitFunc)webEngineLib.resolve(
        "_ZN16QtWebEngineQuick10initializeEv");
    if (initialize) initialize();
}
```

**Challenge**: Initialization must happen before QGuiApplication but library loading typically happens after. Build-time conditional compilation is often more practical:

```cmake
option(ENABLE_WEBENGINE "Enable QtWebEngine support" ON)
if(ENABLE_WEBENGINE)
    find_package(Qt6 COMPONENTS WebEngineQuick)
    target_compile_definitions(myshell PRIVATE HAS_WEBENGINE)
endif()
```

---

## Layer-shell integration for Wayland shells

Desktop shells on Wayland require the **wlr-layer-shell** protocol for positioning panels, overlays, and backgrounds. KDE's **layer-shell-qt** provides the cleanest Qt integration.

### Setting up layer-shell surfaces

```cpp
// Before creating any windows
LayerShellQt::Shell::useLayerShell();

// Per-window configuration
QQuickView *panel = new QQuickView();
LayerShellQt::Window *layerWindow = LayerShellQt::Window::get(panel);
layerWindow->setLayer(LayerShellQt::Window::LayerTop);
layerWindow->setAnchors(LayerShellQt::Window::AnchorTop);
layerWindow->setExclusiveZone(32);
```

Environment requirement: `QT_WAYLAND_SHELL_INTEGRATION=layer-shell`

### Layer-shell and WebEngineView conflicts

Key challenges:
- Layer-shell properties must be set **before** showing windows
- WebEngineView's multi-process model spawns separate GPU/renderer processes with their own Wayland connections
- **QtQuick-based approaches work better** than QWidget for layer-shell + WebEngine

Workaround pattern for widgets: `show()` → `hide()` → configure layer-shell → `show()` again.

---

## Transparency and shader effects on WebEngineView

### Achieving transparent backgrounds

**Critical**: Set `backgroundColor` **before** the first page load, or transparency is ignored.

```qml
WebEngineView {
    backgroundColor: "transparent"  // Must be set BEFORE url assignment
    url: "file:///shell-ui.html"
}
```

Required CSS in loaded content:
```css
body { background: rgba(0,0,0,0); }
```

Window attributes for C++:
```cpp
window.setAttribute(Qt::WA_TranslucentBackground, true);
window.setWindowFlags(Qt::FramelessWindowHint);
page->setBackgroundColor(Qt::transparent);
```

### Shader effects require layer capture

WebEngineView renders through Chromium's compositor—you cannot directly apply `ShaderEffect`. Instead, enable layer rendering to capture to texture first:

```qml
WebEngineView {
    id: webView
    layer.enabled: true  // Render to offscreen texture
    layer.effect: ShaderEffect {
        fragmentShader: "blur.frag.qsb"
    }
}
```

**Performance warning**: Qt documentation states *"In most cases, using a ShaderEffectSource will decrease performance, and in all cases, it will increase video memory usage."* For desktop shells, disable `layer.enabled` when effects aren't active.

---

## Performance optimization for always-on shells

### Memory management challenges

- QtWebEngine memory leaks are widely reported, often marked "won't fix" in Qt bug tracker
- `deleteLater()` only releases memory when returning to the top-level event loop
- Multiple `QtWebEngineProcess` instances persist (typically 2-3)

**Cleanup pattern**:
```cpp
webEngineView->setParent(nullptr);
webEngineView->hide();
webEngineView->page()->deleteLater();
webEngineView->setPage(nullptr);
delete webEngineView;
// Must return to event loop for full cleanup
```

### Lifecycle states for hidden components (Qt 6+)

```qml
WebEngineView {
    lifecycleState: visible ? 
        WebEngineView.Active : WebEngineView.Frozen
    // Frozen: Low CPU, most DOM/JS suspended
    // Discarded: Minimal resources, auto-reloads on activation
}
```

### Lazy loading via Loader

```qml
Loader {
    active: false  // Don't instantiate until needed
    sourceComponent: WebEngineView {
        backgroundColor: "transparent"
    }
}
```

---

## Source code reference structure

### QtWebView repository (qt/qtwebview)

```
src/webview/
├── qwebview.cpp/h           # Public QWebView class
├── qwebviewfactory.cpp      # Plugin loading via QFactoryLoader
├── qquickwebview.cpp        # QML WebView type
└── qtwebviewfunctions.cpp   # QtWebView::initialize()

src/plugins/
├── darwin/qdarwinwebview.mm # WKWebView backend
├── android/                  # JNI WebView backend
└── webengine/               # WebEngine fallback
```

### QtWebEngine repository (qt/qtwebengine)

```
src/core/
├── web_contents_adapter.cpp     # Primary Qt ↔ Chromium bridge
├── render_widget_host_view_qt.cpp  # Rendering integration
├── delegated_frame_node.cpp     # Scene graph node management
└── ozone_platform_qt.cpp        # Wayland/X11 abstraction

src/webenginequick/
└── qquickwebengineview.cpp      # QML WebEngineView

src/3rdparty/chromium/           # Flattened Chromium source
```

### QtWebChannel repository (qt/qtwebchannel)

```
src/webchannel/
├── qwebchannel.cpp              # Main channel class
├── qwebchannelabstracttransport.cpp  # Transport interface
├── qmetaobjectpublisher_p.cpp   # Object introspection & JSON serialization
└── qwebchannel.js               # JavaScript client library
```

---

## Recommended integration pattern for Quickshell

Based on the architectural analysis, this pattern addresses the key requirements:

```qml
// shell-panel.qml
import QtQuick
import QtWebEngine
import Quickshell.Wayland

PanelWindow {
    layer: "top"
    anchor: "bottom"
    exclusiveZone: 48
    color: "transparent"
    
    WebEngineView {
        id: webView
        anchors.fill: parent
        backgroundColor: "transparent"  // Set BEFORE url
        
        // Lifecycle management
        lifecycleState: parent.visible ? 
            WebEngineView.Active : WebEngineView.Frozen
        
        // WebChannel for shell API
        webChannel: WebChannel {
            id: channel
            registeredObjects: [shellApi]
        }
        
        url: "file:///shell/panel.html"
        
        // Avoid layer effects unless absolutely needed
        layer.enabled: false
    }
    
    QtObject {
        id: shellApi
        WebChannel.id: "shell"
        
        signal notificationReceived(string app, string message)
        
        Q_INVOKABLE function launchApp(desktopFile) {
            // Implementation
        }
        
        property bool darkMode: SystemPalette.dark
    }
}
```

**Main initialization**:
```cpp
int main(int argc, char *argv[]) {
    // Ensure argc >= 1 for Chromium
    Q_ASSERT(argc >= 1);
    
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
    
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QtWebEngineQuick::initialize();  // Before QGuiApplication
    
    QGuiApplication app(argc, argv);
    // ...
}
```

---

## Conclusion

Building a QtWebEngine-powered desktop shell requires understanding the fundamental architectural differences between Qt's web modules. **QtWebView's foreign window embedding** makes it unsuitable for shells requiring compositing flexibility—it exists primarily for mobile platforms with native web views. **QtWebEngine's texture-node integration** enables proper scene graph participation but demands careful attention to initialization order (before QGuiApplication), the `argc >= 1` requirement for Chromium, and memory management for long-running processes.

For Wayland shells, combining **layer-shell-qt** with WebEngineView is viable but requires setting transparency before first page load and avoiding `layer.effect` unless absolutely necessary. The **QtWebChannel** bridge provides clean C++/JavaScript communication with batched property updates suitable for shell APIs. Most critically, lifecycle states (`Frozen`/`Discarded`) should be used aggressively for hidden shell components to manage the substantial memory overhead inherent in Chromium embedding.
