# QtWebEngine POC 2 Implementation Report

**Date:** 2025-12-12  
**Status:** Implementation Complete, Awaiting Manual Verification

---

## Summary

This report documents the first implementation of QtWebEngine, QtWebChannel, and MaterialSurface integration for Quickshell. The goal was to enable web-based UI capabilities while maintaining Quickshell's architectural integrity, following the specifications in `0-system-prompt.md`.

---

## Files Modified

### Build System

#### [CMakeLists.txt](file:///home/tom/mecattaf/quickshell/CMakeLists.txt)

**Changes:**
1. Added `boption(WEBENGINE "QtWebEngine" ON)` feature toggle (line 49)
2. Added conditional Qt dependencies when WEBENGINE is enabled:
   - `WebEngineQuick` and `WebChannel` to `QT_FPDEPS`
   - `GuiPrivate` to `QT_PRIVDEPS` (for QRhi access in MaterialNode)
3. Added `target_link_libraries` for WebEngine dependencies
4. Added `QS_WEBENGINE` preprocessor definition

```diff
+boption(WEBENGINE "QtWebEngine" ON)

+if (WEBENGINE_effective)
+    list(APPEND QT_FPDEPS WebEngineQuick WebChannel)
+    list(APPEND QT_PRIVDEPS GuiPrivate)
+endif()

+if (WEBENGINE_effective)
+    target_link_libraries(quickshell PRIVATE Qt6::WebEngineQuick Qt6::WebChannel)
+    target_compile_definitions(quickshell PRIVATE QS_WEBENGINE)
+endif()
```

---

#### [src/CMakeLists.txt](file:///home/tom/mecattaf/quickshell/src/CMakeLists.txt)

**Changes:**
- Added conditional `add_subdirectory(webengine)` at end of file

```diff
+if (WEBENGINE_effective)
+    add_subdirectory(webengine)
+endif()
```

---

### Launch System

#### [src/launch/launch.cpp](file:///home/tom/mecattaf/quickshell/src/launch/launch.cpp)

**Changes:**
1. Added conditional include for QtWebEngine header
2. Added `enableWebEngine` pragma field
3. Added `EnableWebEngine` pragma parsing
4. Added WebEngine initialization block (after `delete coreApplication`, before `QGuiApplication` creation)

```diff
+#ifdef QS_WEBENGINE
+#include <QtWebEngineQuick/qtwebenginequickglobal.h>
+#endif

 struct {
     bool useQApplication = false;
+    bool enableWebEngine = false;
     // ...
 } pragmas;

 if (pragma == "UseQApplication") pragmas.useQApplication = true;
+else if (pragma == "EnableWebEngine") pragmas.enableWebEngine = true;

 delete coreApplication;

+#ifdef QS_WEBENGINE
+    if (pragmas.enableWebEngine) {
+        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
+        QtWebEngineQuick::initialize();
+        qInfo() << "WebEngine initialized";
+    }
+#endif

 QGuiApplication* app = nullptr;
```

---

## Files Created

### New Module: `src/webengine/`

#### Module Configuration

| File | Purpose |
|------|---------|
| [CMakeLists.txt](file:///home/tom/mecattaf/quickshell/src/webengine/CMakeLists.txt) | Build configuration with shader compilation |
| [module.md](file:///home/tom/mecattaf/quickshell/src/webengine/module.md) | Module documentation (Quickshell convention) |

---

#### C++ Components

| File | Class | Purpose |
|------|-------|---------|
| [shellwebengineview.hpp](file:///home/tom/mecattaf/quickshell/src/webengine/shellwebengineview.hpp) | `ShellWebEngineView` | Header for wrapped WebEngineView |
| [shellwebengineview.cpp](file:///home/tom/mecattaf/quickshell/src/webengine/shellwebengineview.cpp) | — | Implementation with transparent background, lifecycle management |
| [shellbridge.hpp](file:///home/tom/mecattaf/quickshell/src/webengine/shellbridge.hpp) | `ShellBridge` | Header for QtWebChannel bridge |
| [shellbridge.cpp](file:///home/tom/mecattaf/quickshell/src/webengine/shellbridge.cpp) | — | Implementation with logging, theme tokens, material regions |
| [materialsurface.hpp](file:///home/tom/mecattaf/quickshell/src/webengine/materialsurface.hpp) | `MaterialSurface` | Header for QQuickItem wrapper |
| [materialsurface.cpp](file:///home/tom/mecattaf/quickshell/src/webengine/materialsurface.cpp) | — | Implementation with updatePaintNode() |
| [materialnode.hpp](file:///home/tom/mecattaf/quickshell/src/webengine/materialnode.hpp) | `MaterialNode` | Header for QSGRenderNode |
| [materialnode.cpp](file:///home/tom/mecattaf/quickshell/src/webengine/materialnode.cpp) | — | RHI-based rendering implementation |

---

#### Shaders

| File | Purpose |
|------|---------|
| [shaders/material.vert](file:///home/tom/mecattaf/quickshell/src/webengine/shaders/material.vert) | Vertex shader for material quad |
| [shaders/material.frag](file:///home/tom/mecattaf/quickshell/src/webengine/shaders/material.frag) | Fragment shader with SDF rounded rectangle |

**Shader Features:**
- GLSL 440 targeting Vulkan
- SDF-based rounded rectangle rendering
- Aspect ratio correction for proper corner rendering
- Material level modulation (1-5 intensity)
- Premultiplied alpha blending

---

#### QML Components

| File | Purpose |
|------|---------|
| [WebSurface.qml](file:///home/tom/mecattaf/quickshell/src/webengine/WebSurface.qml) | High-level combo of MaterialSurface + ShellWebEngineView |
| [ShellChannel.qml](file:///home/tom/mecattaf/quickshell/src/webengine/ShellChannel.qml) | WebChannel convenience wrapper with pre-configured ShellBridge |

---

#### Test Configuration

| File | Purpose |
|------|---------|
| [test/manual/test.qml](file:///home/tom/mecattaf/quickshell/src/webengine/test/manual/test.qml) | Demo shell with panel and floating widget |
| [test/manual/panel.html](file:///home/tom/mecattaf/quickshell/src/webengine/test/manual/panel.html) | Example taskbar-style panel |
| [test/manual/widget.html](file:///home/tom/mecattaf/quickshell/src/webengine/test/manual/widget.html) | Example floating widget with rounded corners |
| [test/manual/webshell.js](file:///home/tom/mecattaf/quickshell/src/webengine/test/manual/webshell.js) | JavaScript WebChannel client library |

---

## Component Architecture

```
User's shell.qml
    │
    ├── //@ pragma EnableWebEngine  ← Required pragma
    │
    └── WebSurface
            │
            ├── MaterialSurface (z: -1)
            │       │
            │       └── MaterialNode (QSGRenderNode)
            │               └── material.vert/frag shaders
            │
            └── ShellWebEngineView
                    │
                    └── WebChannel ← ShellChannel
                            │
                            └── ShellBridge
                                    ├── log(level, message)
                                    ├── darkMode property
                                    ├── accentColor property
                                    └── getThemeTokens()
```

---

## Key Implementation Details

### 1. WebEngine Initialization Sequence

WebEngine initialization happens in `launch.cpp` with this critical ordering:
1. Parse pragmas from config file
2. Delete temporary `QCoreApplication`
3. **Set `Qt::AA_ShareOpenGLContexts`** (must be before any Qt GUI)
4. **Call `QtWebEngineQuick::initialize()`**
5. Create `QGuiApplication`

This order is **mandatory** per Qt documentation.

### 2. Transparent Background

`ShellWebEngineView` sets `backgroundColor` to `Qt::transparent` in its constructor, **before** any URL is loaded. This is critical—setting transparency after page load is silently ignored.

### 3. Lifecycle Management

`ShellWebEngineView` automatically transitions between lifecycle states:
- **Active**: When visible and `active` property is true
- **Frozen**: When hidden or `active` is false (reduces CPU usage)

### 4. MaterialNode RHI Rendering

`MaterialNode` uses Qt's RHI (Rendering Hardware Interface) for cross-platform GPU rendering:
- Resources created lazily in `render()` (only valid context)
- Uniform buffer contains: MVP matrix, tint color, corner radius, material level, size
- SDF-based rounded rectangle with anti-aliased edges

### 5. ShellBridge QtWebChannel API

JavaScript-accessible methods:
```javascript
shell.log(level, message)     // Log to Quickshell console
shell.getThemeTokens()        // Get theme colors/spacing/radius
shell.darkMode                // Property (auto-updates via signal)
shell.accentColor             // Property (auto-updates via signal)
shell.registerMaterialRegion(data)   // Future: dynamic blur regions
shell.unregisterMaterialRegion(id)
shell.updateMaterialRegion(id, updates)
```

---

## Build Commands

### Build with WebEngine enabled:
```bash
cmake -B build -DWEBENGINE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Build with WebEngine disabled:
```bash
cmake -B build -DWEBENGINE=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Run test configuration:
```bash
./build/src/quickshell -c src/webengine/test/manual/test.qml
```

---

## Usage in Shell Configurations

### Minimal Example

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
        
        WebSurface {
            anchors.fill: parent
            materialLevel: 3
            tintColor: "#60000000"
            url: Qt.resolvedUrl("panel.html")
            webChannel: ShellChannel {}
        }
    }
}
```

### HTML Requirements

Web content must have transparent background:
```css
html, body {
    background: transparent;
}
```

### JavaScript WebChannel Connection

```javascript
// Load Qt's WebChannel library
// <script src="qrc:///qtwebchannel/qwebchannel.js"></script>

new QWebChannel(qt.webChannelTransport, function(channel) {
    var shell = channel.objects.shell;
    shell.log('info', 'Connected!');
    console.log('Dark mode:', shell.darkMode);
});
```

Or use the provided `webshell.js` helper:
```javascript
initWebShell().then(shell => {
    shell.info('Connected!');
    shell.getThemeTokens().then(tokens => {
        applyThemeTokens(tokens);
    });
});
```

---

## Known Limitations & Future Work

### POC 2 Limitations
1. MaterialNode renders simple tinted rectangle only (no blur)
2. Theme properties (darkMode, accentColor) are not connected to Quickshell's actual theme system
3. Material regions are registered but not rendered dynamically

### Future Iterations
1. **Dual-Kawase blur** in MaterialNode
2. **Backdrop texture sampling** for true glass effects
3. **Integration with Quickshell theme system**
4. **Dynamic material regions** from JavaScript
5. **Lazy loading patterns** with conditional Loaders for heavy widgets

---

## Refinements from Reference Analysis

After reviewing `docs/1-quickshell-webview-patterns.md` and `docs/2-qtwebengine-vs-qtwebview.md`, the following refinements were made:

### argv Validation (Critical)
Chromium's `CommandLine::Init()` requires `argv[0]` to be the program path. Added validation in `launch.cpp`:
```cpp
if (argv == nullptr || argv[0] == nullptr) {
    qWarning() << "WebEngine: argv[0] is null, Chromium subprocess may fail";
}
```

### DevTools Port Moved to Launch Time
Setting `QTWEBENGINE_REMOTE_DEBUGGING` per-view is too late—it must be set before `QtWebEngineQuick::initialize()`. Now controlled via:
```bash
QS_WEBENGINE_DEVTOOLS_PORT=9222 quickshell
```

### Lifecycle States: Discarded Support
Added `discardWhenHidden` property to `ShellWebEngineView`:
- `false` (default): Hidden views enter **Frozen** state (~50MB savings, fast resume)
- `true`: Hidden views enter **Discarded** state (~200MB savings, will reload)

### Destructor Cleanup Pattern
Implemented proper cleanup in `~ShellWebEngineView()` following Qt bug tracker recommendations:
```cpp
setParent(nullptr);
hide();
setLifecycleState(Discarded);
```

### WebChannel Property Batching
Exposed `propertyUpdateInterval` in `ShellChannel.qml` with 50ms default. Users can adjust for responsiveness vs efficiency trade-off.

### layer.enabled Warning
Added documentation warning against enabling `layer.enabled` on WebEngineView—it significantly impacts performance and VRAM usage.

---

## Technical Notes

### Pragma Naming: `EnableWebEngine`

The pragma is named `EnableWebEngine` (not `EnableQtWebEngine`). This follows the existing pattern of short pragma names:
- `UseQApplication` (not `UseQtApplication`)
- `NativeTextRendering`
- `IgnoreSystemSettings`

### Uniform Buffer Layout Verification

The C++ struct `MaterialUniforms` (112 bytes) matches the shader's `std140` layout:

| Field | Size | Offset | Notes |
|-------|------|--------|-------|
| `mat4 mvp` | 64 | 0 | 4x4 float matrix |
| `vec4 tintColor` | 16 | 64 | RGBA floats |
| `vec4 params` | 16 | 80 | cornerRadius, materialLevel, opacity, unused |
| `vec2 size` | 8 | 96 | width, height |
| `vec2 padding` | 8 | 104 | Alignment to 16-byte boundary |
| **Total** | **112** | | Verified with `static_assert` |

### Shader Compilation Configuration

The `qt6_add_shaders` call follows existing Quickshell patterns from `src/widgets/CMakeLists.txt`:

```cmake
qt6_add_shaders(quickshell-webengine "webengine-material"
    NOHLSL NOMSL BATCHABLE PRECOMPILE OPTIMIZED QUIET
    PREFIX "/Quickshell/WebEngine"
    FILES
        shaders/material.vert
        shaders/material.frag
    OUTPUTS
        shaders/material.vert.qsb
        shaders/material.frag.qsb
)
```

- `NOHLSL NOMSL`: Skip HLSL/Metal shader variants (Linux/Wayland only)
- `BATCHABLE`: Enable Qt Quick batch rendering compatibility
- `PRECOMPILE`: Pre-compile shaders at build time
- `OPTIMIZED QUIET`: Optimization flags
- Shader resource path: `:/Quickshell/WebEngine/shaders/...`

### QML Module Registration

The module is registered as `Quickshell.WebEngine` URI:

```cmake
qt_add_qml_module(quickshell-webengine
    URI Quickshell.WebEngine
    VERSION 0.1
    DEPENDENCIES QtQuick
    QML_FILES
        WebSurface.qml
        ShellChannel.qml
)
```

This exposes:
- C++ types: `ShellWebEngineView`, `ShellBridge`, `MaterialSurface`
- QML types: `WebSurface`, `ShellChannel`

### Test Path Notes

The test command `./build/src/quickshell -c src/webengine/test/manual/test.qml` is for development testing from the source directory. For installed quickshell:
- Tests are not installed (they're in the source tree)
- Users would use their own shell configs with `import Quickshell.WebEngine`

---

## File Tree

```
src/webengine/
├── CMakeLists.txt
├── module.md
├── shellwebengineview.hpp
├── shellwebengineview.cpp
├── shellbridge.hpp
├── shellbridge.cpp
├── materialsurface.hpp
├── materialsurface.cpp
├── materialnode.hpp
├── materialnode.cpp
├── WebSurface.qml
├── ShellChannel.qml
├── shaders/
│   ├── material.vert
│   └── material.frag
└── test/
    └── manual/
        ├── test.qml
        ├── panel.html
        ├── widget.html
        └── webshell.js
```

---

## Verification Checklist

- [ ] Build succeeds with `-DWEBENGINE=ON`
- [ ] Build succeeds with `-DWEBENGINE=OFF`
- [ ] `import Quickshell.WebEngine` works in QML
- [ ] WebEngineView renders with transparent background
- [ ] MaterialSurface renders tinted rounded rectangle
- [ ] WebChannel communication works (`shell.log()` messages appear)
- [ ] No crashes on startup/shutdown
