## System Prompt: Implementing QtWebEngine, QtWebChannel, and Material System in Quickshell

### Project Overview

You are implementing a WebEngine integration for **Quickshell**, a Qt Quick-based toolkit for building Wayland desktop shells. Quickshell enables developers to create panels, docks, widgets, and lockscreens using QML. Your task is to extend it with web-based UI capabilities while maintaining its architectural integrity.

**What Quickshell Is:**
- A Wayland **client** (NOT a compositor) that uses wlr-layer-shell protocol
- A QML runtime that loads user-written shell configurations
- A toolkit with modular services (Pipewire, MPRIS, UPower, Notifications, etc.)
- Built on Qt6 with extensive use of Qt Quick scene graph

**What You Are Building:**
- QtWebEngine integration allowing WebEngineView in shell configurations
- QtWebChannel bridge for JavaScript ↔ QML/C++ communication  
- QSGRenderNode-based material system for blur/glass effects beneath web content
- Foundation for web-technology-based desktop shell development

**Design Philosophy:**
The goal is enabling developers to build shell UIs using HTML/CSS/JavaScript while leveraging Quickshell's native system integration. Web content handles presentation and interaction; C++/QML handles system access and GPU effects. This is the "thick client" pattern — heavy computation stays in C++, JavaScript receives results reactively.

---

### Critical Technical Requirements

#### 1. QtWebEngine Initialization Sequence

QtWebEngine has strict initialization requirements that cannot be violated:

**Mandatory Order:**
1. Parse command-line arguments and pragmas
2. Create temporary QCoreApplication for pragma processing
3. Delete QCoreApplication
4. Set `Qt::AA_ShareOpenGLContexts` attribute
5. Call `QtWebEngineQuick::initialize()`
6. Create QGuiApplication

**Why This Matters:**
- WebEngine requires OpenGL context sharing between GUI and renderer processes
- Initialization after QGuiApplication causes rendering failures or crashes
- The context sharing attribute must be set before any Qt GUI objects exist

**The argc Problem:**
Chromium's `CommandLine::Init()` requires at least one argument (argv[0] must contain the program path). If argc equals zero, Chromium cannot determine the browser subprocess path, causing renderer process spawn failures. Validate this before WebEngine initialization.

#### 2. WebEngineView Transparency

For web content to composite correctly over shell backgrounds:

- `backgroundColor` property must be set to `"transparent"` **before** the `url` property is assigned
- Setting transparency after initial page load is silently ignored
- HTML content must also have transparent body: `body { background: transparent; }`
- The window/surface must support transparency (layer-shell surfaces do)

#### 3. Scene Graph Integration

QtWebEngine renders via `DelegatedFrameNode`, which converts Chromium's compositor output into `QSGNode` texture nodes. This means:

- Web content participates in Qt Quick's scene graph (unlike QtWebView's foreign window approach)
- QML items can be layered above or below web content
- Shader effects can be applied (with performance considerations)
- Transparency and compositing work correctly

Your MaterialSurface must render **beneath** (z-order) the WebEngineView to create the glass-behind-content effect.

#### 4. QtWebChannel Mechanics

QtWebChannel provides bidirectional IPC between C++ and JavaScript:

**C++ Side:**
- Register QObjects with the channel via `registeredObjects`
- Expose methods with `Q_INVOKABLE`
- Expose properties with `Q_PROPERTY` and change signals
- Properties are cached on JS side, updated via signal batching (50ms default)

**JavaScript Side:**
- Load `qrc:///qtwebchannel/qwebchannel.js`
- Connect via `new QWebChannel(qt.webChannelTransport, callback)`
- Access objects via `channel.objects.objectId`
- All calls are asynchronous — use callbacks or connect to signals

**Transport:** In QtWebEngine, `qt.webChannelTransport` is injected automatically. This uses Chromium's internal IPC, not WebSockets.

---

### Architecture

#### Module Structure

Create a new module within Quickshell's source tree:

```
src/webengine/
├── module.md                    # Module documentation (Quickshell convention)
├── CMakeLists.txt               # Build configuration
│
├── Initialization
│   └── (integrate with launch.cpp for WebEngine init)
│
├── Core Components
│   ├── ShellWebEngineView       # Wrapped WebEngineView with shell defaults
│   └── ShellBridge              # QtWebChannel bridge object
│
├── Material System
│   ├── MaterialSurface          # QQuickItem wrapper
│   ├── MaterialNode             # QSGRenderNode implementation
│   └── shaders/                 # GLSL shaders (compiled to QSB)
│
└── QML Components
    ├── qmldir                   # Module definition
    ├── WebSurface.qml           # High-level Material + WebEngine combo
    └── ShellChannel.qml         # WebChannel convenience wrapper
```

#### Component Relationships

```
┌─────────────────────────────────────────────────────────────┐
│ User's shell.qml                                            │
│                                                             │
│   PanelWindow {                                             │
│       WebSurface {              ← High-level component      │
│           materialLevel: 3                                  │
│           url: "panel.html"                                 │
│           webChannel: ShellChannel { }                      │
│       }                                                     │
│   }                                                         │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ WebSurface.qml (internal composition)                       │
│                                                             │
│   Item {                                                    │
│       MaterialSurface { z: -1 }    ← Renders beneath        │
│       ShellWebEngineView { }        ← Web content on top    │
│   }                                                         │
└─────────────────────────────────────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    ▼                   ▼
┌───────────────────────────┐ ┌───────────────────────────────┐
│ MaterialSurface           │ │ ShellWebEngineView            │
│                           │ │                               │
│ QQuickItem that creates   │ │ Wrapped QQuickWebEngineView   │
│ MaterialNode in           │ │ with:                         │
│ updatePaintNode()         │ │ - Transparent background      │
│                           │ │ - Lifecycle management        │
│ Properties:               │ │ - DevTools access             │
│ - materialLevel (1-5)     │ │ - WebChannel integration      │
│ - tintColor               │ │                               │
│ - cornerRadius            │ └───────────────────────────────┘
│ - opacity                 │
└───────────────────────────┘
            │
            ▼
┌───────────────────────────┐
│ MaterialNode              │
│                           │
│ QSGRenderNode that:       │
│ - Creates RHI resources   │
│ - Compiles shaders        │
│ - Renders material quad   │
│                           │
│ POC 1: Tinted rounded rect│
│ Future: Multi-pass blur   │
└───────────────────────────┘
```

#### Data Flow: Theme Tokens

```
Quickshell Config/Theme System
            │
            ▼
    ShellBridge (C++)
    - Reads theme state
    - Exposes Q_PROPERTYs
            │
            │ QtWebChannel
            ▼
    JavaScript Runtime
    - Receives property values
    - Subscribes to change signals
    - Updates CSS custom properties
            │
            ▼
    Web UI renders with theme
```

#### Data Flow: Material Regions (Future)

```
Web Content
- Declares regions via ShellBridge.registerMaterialRegion()
- Sends: { id, bounds, materialLevel, cornerRadius }
            │
            │ QtWebChannel
            ▼
    ShellBridge (C++)
    - Maintains region registry
    - Emits materialRegionsChanged
            │
            │ QML Binding
            ▼
    QML Repeater creates MaterialSurface per region
    - Positioned at declared bounds
    - Renders appropriate material level
```

---

### Implementation Guidelines

#### Build System Integration

Follow Quickshell's existing patterns:

**Feature Toggle:**
- Add `boption(WEBENGINE "QtWebEngine" ON)` following existing boption patterns
- Use `WEBENGINE_effective` for conditional compilation
- Define `QS_WEBENGINE` preprocessor symbol when enabled

**Dependencies:**
- Qt6::WebEngineQuick
- Qt6::WebChannel  
- Qt6::GuiPrivate (for QRhi access in MaterialNode)
- Qt6::ShaderTools (for shader compilation)

**Graceful Degradation:**
- Build must succeed with `-DWEBENGINE=OFF`
- Missing WebEngine at runtime should warn, not crash
- Shell configurations not using WebEngine should work normally

#### QML Module Registration

Register types under `Quickshell.WebEngine` URI:

- `ShellWebEngineView` — Core web view component
- `MaterialSurface` — Material effect layer
- `ShellBridge` — Channel bridge object (for use in WebChannel)
- QML files: `WebSurface`, `ShellChannel`

Follow Quickshell's existing module patterns in `src/wayland/` and `src/services/`.

#### ShellWebEngineView Design

This wraps `QQuickWebEngineView` with shell-optimized defaults:

**Default Behaviors:**
- Background color transparent by default (critical for shells)
- Lifecycle state tracks visibility (Active when visible, Frozen when hidden)
- DevTools port configurable (disabled by default, enable for development)

**Properties to Expose:**
- All essential WebEngineView properties (url, title, loading, etc.)
- `devToolsEnabled` and `devToolsPort` for debugging
- Convenient access to the underlying page for advanced use

**Lifecycle Management:**
WebEngineView consumes ~200MB per instance. Implement automatic lifecycle transitions:
- `Active` — Fully running, visible to user
- `Frozen` — Suspended JS execution, DOM retained, minimal CPU
- `Discarded` — Resources released, will reload on activation

#### ShellBridge Design

The bridge object exposes shell functionality to JavaScript:

**Theme Integration:**
- `darkMode` property with change signal
- `accentColor` property with change signal
- `getThemeTokens()` method returning color/spacing/radius values

**Logging:**
- `log(level, message)` method that outputs to Quickshell's console
- Levels: debug, info, warn, error

**Material Region Registration (Foundation for Future):**
- `registerMaterialRegion(regionData)` 
- `unregisterMaterialRegion(id)`
- `updateMaterialRegion(id, updates)`
- `materialRegions` property (list of active regions)

**Design Principles:**
- All methods are async from JS perspective
- Use signals for push updates, not polling
- Return QVariantMap for complex data (auto-serializes to JSON)
- Never expose blocking operations

#### MaterialSurface and MaterialNode

**MaterialSurface (QQuickItem):**
- Exposes declarative properties: materialLevel, tintColor, cornerRadius, opacity
- Implements `updatePaintNode()` to create/update MaterialNode
- Handles geometry changes and property updates

**MaterialNode (QSGRenderNode):**
- Creates RHI resources lazily in `render()` (context only valid there)
- Implements required overrides: `render()`, `releaseResources()`, `flags()`, `rect()`
- Uses Qt's shader compilation (GLSL → SPIR-V via qsb tool)

**POC 1 Scope:**
For the initial implementation, MaterialNode renders a simple tinted rounded rectangle. The shader structure should anticipate future multi-pass blur:
- Uniform buffer with material parameters
- Rounded corner SDF calculation
- Tint color with alpha blending

**Future Expansion:**
- Dual-Kawase blur passes
- Backdrop texture sampling
- Vibrancy/saturation adjustment
- Rim lighting effects

#### Shader Compilation

Use Qt's shader tools:

**Build-time Compilation:**
```cmake
qt6_add_shaders(target "shaders"
    PREFIX "/quickshell/webengine"
    FILES
        shaders/material.vert
        shaders/material.frag
)
```

**Shader Requirements:**
- Target GLSL 440 for Vulkan compatibility
- Include fallbacks: `--glsl "100es,120,150" --hlsl 50 --msl 12`
- Output to QSB format (cross-platform shader bundles)

---

### Patterns to Follow (from Quickshell Codebase)

**Lazy Loading:**
Use `Loader` with `active` binding for heavy components. WebEngineView should only instantiate when actually needed.

**State Singletons:**
Follow the `Runtime.qml` pattern — global state in singletons, UI components bind reactively.

**Configuration Persistence:**
If adding persistent settings, use Quickshell's `FileView` + `JsonAdapter` pattern with debounced writes.

**Focus Management:**
For popups/overlays containing web content, integrate with Quickshell's focus grab system (`HyprlandFocusGrab` or equivalent).

**Error Handling:**
Emit warnings via `qWarning()` for recoverable issues. Never crash on WebEngine problems — gracefully degrade.

---

### Pitfalls to Avoid

1. **Initialization Order** — WebEngine init after QGuiApplication causes silent rendering failures or crashes. This is the most common mistake.

2. **Transparency Timing** — Setting backgroundColor after url assignment is silently ignored. Always set transparency first.

3. **Synchronous Assumptions** — QtWebChannel is fundamentally async. JavaScript code expecting immediate return values will break.

4. **RHI Context Scope** — QRhi resources can only be created within `render()`. Attempting to create pipelines or buffers elsewhere causes crashes.

5. **Z-Order** — MaterialSurface must have lower z-order than WebEngineView. Without explicit `z: -1`, the material renders on top, hiding web content.

6. **Layer Effects on WebEngineView** — Setting `layer.enabled: true` on WebEngineView forces render-to-texture, severely impacting performance. Avoid unless absolutely necessary.

7. **Memory Leaks** — WebEngine has known memory leak issues. Implement proper cleanup: set parent to null, hide, delete page, then delete view. Return to event loop for full cleanup.

8. **DevTools Requirement** — Shell UI debugging is impossible without DevTools access. Always provide a way to enable remote debugging.

---

### Success Criteria (POC 1)

The implementation is complete when:

1. **Build System**
   - Quickshell builds successfully with `-DWEBENGINE=ON`
   - Quickshell builds successfully with `-DWEBENGINE=OFF`
   - No WebEngine symbols leak into non-WebEngine builds

2. **QML Integration**
   - `import Quickshell.WebEngine` works in shell configurations
   - All documented components are accessible

3. **WebEngineView Functionality**
   - Web content renders with transparent background
   - Layer-shell positioning works correctly (panels anchor properly)
   - Lifecycle states transition based on visibility

4. **MaterialSurface Functionality**
   - Renders a tinted rounded rectangle
   - Positioned correctly beneath WebEngineView
   - Properties (materialLevel, tintColor, cornerRadius) affect rendering

5. **WebChannel Communication**
   - JavaScript successfully connects to channel
   - `shell.log()` messages appear in Quickshell console
   - Theme properties are accessible from JavaScript
   - Property change signals trigger JavaScript callbacks

6. **Stability**
   - No crashes on shell startup
   - No crashes on shell shutdown
   - Memory usage reasonable (~200-300MB for single WebEngineView)

---

### Reference Materials

**Quickshell Source:**
- Module structure: examine `src/wayland/`, `src/services/`
- Build patterns: examine root `CMakeLists.txt` and module CMakeLists
- QML registration: examine existing plugin patterns

**Qt Documentation:**
- QSGRenderNode: https://doc.qt.io/qt-6/qsgrendernode.html
- QRhi (Rendering Hardware Interface): https://doc.qt.io/qt-6/qrhi.html
- WebEngineView: https://doc.qt.io/qt-6/qml-qtwebengine-webengineview.html
- WebChannel: https://doc.qt.io/qt-6/qtwebchannel-index.html

**Qt Examples:**
- Custom Render Node: `qtdeclarative/examples/quick/scenegraph/customrendernode`
- Vulkan Under QML: `qtdeclarative/examples/quick/scenegraph/vulkanunderqml`

---

### Deliverables

1. **Source Files** — All C++ headers, implementations, QML files, and shaders
2. **CMake Integration** — Module CMakeLists.txt and modifications to parent CMakeLists
3. **Module Documentation** — module.md following Quickshell conventions
4. **Test Configuration** — Example shell.qml and panel.html demonstrating functionality
5. **Build Instructions** — Any special build steps or dependencies

