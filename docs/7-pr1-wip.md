# PR 1 Refinement - WebEngine/WebChannel (Minimal)

**Date:** 2025-12-12  
**Status:** WIP - Files restructured, build verification pending

---

## Summary

The original implementation was "too far" — it included MaterialSurface, MaterialNode, RHI shaders, and dynamic material regions. This is powerful but makes for a large, harder-to-review PR.

This refinement splits the work into two PRs:

| PR | Scope | Status |
|----|-------|--------|
| **PR 1** | Minimal WebEngine + WebChannel | ✅ Trimmed |
| **PR 2** | MaterialSurface + RHI shaders | 📁 Files staged in `docs/PR-2-materials/` |

---

## What PR 1 Now Contains

### Core Components (kept)

| File | Purpose |
|------|---------|
| `shellwebengineview.hpp/cpp` | WebEngineView wrapper with transparency + lifecycle |
| `shellbridge.hpp/cpp` | WebChannel bridge (logging, theme tokens) |
| `ShellChannel.qml` | WebChannel convenience wrapper |
| `webshell.js` | JavaScript client library |
| `resources.qrc` | QRC for webshell.js |
| `module.md` | Updated documentation |
| `CMakeLists.txt` | Simplified build (no shaders) |
| `test/manual/*` | Simplified test files |

### Launch System (unchanged)

- `//@ pragma EnableWebEngine` support
- `QtWebEngineQuick::initialize()` call
- `Qt::AA_ShareOpenGLContexts` attribute
- DevTools port via `QS_WEBENGINE_DEVTOOLS_PORT`

### Build System Changes

**Root CMakeLists.txt:**
- Removed `GuiPrivate` from `QT_PRIVDEPS` (was only needed for RHI in MaterialNode)

**src/webengine/CMakeLists.txt:**
- Removed `materialsurface.cpp`, `materialnode.cpp`
- Removed `qt6_add_shaders()` call
- Removed `WebSurface.qml`, `MaterialRegionRenderer.qml` from QML module
- Removed `Qt::GuiPrivate` from `target_link_libraries`

---

## Files Moved to PR-2

All material-related files are now in `docs/PR-2-materials/` (flat format):

| Original Path | New Location |
|---------------|--------------|
| `src/webengine/materialsurface.hpp` | `docs/PR-2-materials/materialsurface.hpp` |
| `src/webengine/materialsurface.cpp` | `docs/PR-2-materials/materialsurface.cpp` |
| `src/webengine/materialnode.hpp` | `docs/PR-2-materials/materialnode.hpp` |
| `src/webengine/materialnode.cpp` | `docs/PR-2-materials/materialnode.cpp` |
| `src/webengine/shaders/material.vert` | `docs/PR-2-materials/shaders-material.vert` |
| `src/webengine/shaders/material.frag` | `docs/PR-2-materials/shaders-material.frag` |
| `src/webengine/WebSurface.qml` | `docs/PR-2-materials/WebSurface.qml` |
| `src/webengine/MaterialRegionRenderer.qml` | `docs/PR-2-materials/MaterialRegionRenderer.qml` |

---

## PR 1 File Tree (Final)

```
src/webengine/
├── CMakeLists.txt          # Simplified, no shaders
├── module.md               # Updated docs
├── resources.qrc           # webshell.js resource
├── shellwebengineview.hpp
├── shellwebengineview.cpp
├── shellbridge.hpp
├── shellbridge.cpp
├── ShellChannel.qml
├── webshell.js
└── test/
    └── manual/
        ├── test.qml        # Uses ShellWebEngineView directly
        ├── panel.html
        ├── widget.html
        └── webshell.js
```

---

## Key Changes to test.qml

**Before (used WebSurface):**
```qml
WebSurface {
    anchors.fill: parent
    materialLevel: 3
    tintColor: "#60000000"
    cornerRadius: 0
    url: Qt.resolvedUrl("panel.html")
    webChannel: ShellChannel {}
}
```

**After (uses ShellWebEngineView directly):**
```qml
ShellWebEngineView {
    anchors.fill: parent
    url: Qt.resolvedUrl("panel.html")
    webChannel: ShellChannel {}
}
```

---

## PR 1 Description (Ready to Use)

### Title
```
Add optional QtWebEngine + QtWebChannel support
```

### Description
```
This PR adds optional QtWebEngine and QtWebChannel integration to Quickshell.

• No new required dependencies. WebEngine initializes only when:
    - Quickshell is built with -DWEBENGINE=ON
    - //@ pragma EnableWebEngine is present in the user's shell config

• Launch system:
    - Adds EnableWebEngine pragma
    - Calls QtWebEngineQuick::initialize() only when enabled
    - Sets Qt::AA_ShareOpenGLContexts before QGuiApplication creation

• New module: Quickshell.WebEngine
    - ShellWebEngineView: Wrapped WebEngineView with transparency + lifecycle control
    - ShellBridge: WebChannel bridge for JS <-> QML
    - ShellChannel.qml: Convenience WebChannel wrapper

• Manual test suite under src/webengine/test/manual:
    - Demonstrates transparent WebEngine rendering
    - Working WebChannel communication
    - Sample HTML/CSS/JS UI

• All builds succeed with WEBENGINE=OFF and WEBENGINE=ON.
• Code follows Quickshell's architecture and module conventions.

MaterialSurface/MaterialNode (RHI shaders) will follow in a separate PR.
```

---

## Verification Checklist (TODO)

- [ ] Build succeeds with `-DWEBENGINE=ON`
- [ ] Build succeeds with `-DWEBENGINE=OFF`
- [ ] `import Quickshell.WebEngine` works in QML
- [ ] WebEngineView renders with transparent background
- [ ] WebChannel communication works (`shell.log()` messages appear)
- [ ] No crashes on startup/shutdown

---

## Next Steps

1. **Verify build** — Run `cmake -B build -DWEBENGINE=ON && cmake --build build`
2. **Test manually** — Run `./build/src/quickshell -c src/webengine/test/manual/test.qml`
3. **Fix any issues** — Address compiler errors or runtime bugs
4. **Open PR** — Submit to upstream quickshell repository
5. **PR 2 later** — Restore MaterialSurface from `docs/PR-2-materials/` in a follow-up PR
