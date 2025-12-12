# PR 2 Scoping: The Material System (Revised)

**Date:** 2025-12-12
**Status:** Planned (Post-PR 1)
**Focus:** Adding `MaterialSurface` using **Pure QML + ShaderEffect**.

## 1. Discovery & Strategy Shift

Initial analysis suggested a C++ RHI implementation (`MaterialNode.cpp`) was needed for custom shaders. However, deep analysis of `src/widgets/ClippingRectangle.qml` and the `eqsh` reference architecture reveals that Quickshell already supports custom shaders via the standard `ShaderEffect` QML type.

**Decision:** Switch from C++ RHI (`QSGRenderNode`) to QML `ShaderEffect`.

## 2. Complexity Assessment (Revised)

| Component | Old Approach (C++ RHI) | New Approach (QML ShaderEffect) |
| :--- | :--- | :--- |
| **Language** | C++ (Low-level RHI) | QML + GLSL |
| **Dependencies** | Private Qt Headers (`<private/qrhi_p.h>`) | Public Qt Quick API |
| **Risk** | High (ABI breaks, version fragility) | **Zero** (Standard API) |
| **Invasiveness** | Medium (Custom C++ class) | **Low** (Just a QML file) |

**Verdict:** The new approach eliminates the biggest risk (private headers) and drastically simplifies the implementation.

## 3. Implementation Plan

### A. Files
1.  `src/webengine/MaterialSurface.qml`:
    ```qml
    import QtQuick
    ShaderEffect {
        property color tintColor: "#40000000"
        property real cornerRadius: 12
        property real materialLevel: 3
        fragmentShader: "qrc:/Quickshell/WebEngine/shaders/material.frag.qsb"
    }
    ```
2.  `src/webengine/shaders/material.frag`:
    -   Existing SDF shader, slightly adapted to use `ShaderEffect` uniforms (which map directly to QML properties).

### B. Build System
We still need to compile the shaders, but we don't need to link private libraries.

```cmake
# In src/webengine/CMakeLists.txt
find_package(Qt6 REQUIRED COMPONENTS ShaderTools)

qt6_add_shaders(quickshell-webengine "webengine_shaders"
    PREFIX "/Quickshell/WebEngine/shaders"
    FILES shaders/material.frag
)

target_link_libraries(quickshell-webengine PRIVATE Qt::ShaderTools)
```

## 4. Conclusion

PR 2 is now trivial. It is purely additive, uses standard APIs, and carries no maintenance burden regarding Qt versions.
