# WebChannel Considerations & Future Implementation Strategy

**Date:** 2025-12-12
**Status:** Planning / Analysis
**Context:** Post-PR1 Refinement

---

## 1. Fix: `launch.cpp` Inconsistency

**Issue:**
The `src/launch/launch.cpp` file contained a compilation error and redundant logic.
1.  It referenced `pragmas.useQtWebEngineQuick`, but the struct member was named `enableWebEngine`.
2.  It attempted to call `web_engine::init()` (a leftover from a previous implementation) while `QtWebEngineQuick::initialize()` was already being called correctly inside the `#ifdef QS_WEBENGINE` block.

**Resolution:**
The redundant block has been removed. The initialization logic is now consolidated:

```cpp
#ifdef QS_WEBENGINE
    if (pragmas.enableWebEngine) {
        // ... argv checks ...
        // ... DevTools port setup ...
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
        QtWebEngineQuick::initialize();
        qInfo() << "WebEngine initialized";
    }
#endif
```

This ensures a clean, single point of initialization that respects the compile-time flag (`-DWEBENGINE=ON`) and the runtime pragma (`//@ pragma EnableWebEngine`).

---

## 2. Future Implementation Analysis (PR 1+ and Beyond)

This section outlines the implementation strategy for **Compositor Architecture** (`FUTURE/1-*`) and **Bridge API** (`FUTURE/5-*`), specifically focusing on the constraint: **"Minimal changes to the Quickshell source code."**

### A. The "Bolt-On" Philosophy

To minimize core changes, we treat Quickshell as a **Platform**, not a **Product**. The core C++ codebase should provide *capabilities*, while the specific logic (Compositor behavior, Bridge APIs) should live in *Modules* or *QML*.

| Component | Core Change (C++) | Bolt-On (QML / Modules) |
| :--- | :--- | :--- |
| **WebEngine** | `ShellWebEngineView` (Wrapper), Initialization | `ShellChannel`, `webshell.js` |
| **Compositor** | **None** (Ideally) | `MaterialSurface`, `ShaderEffect`, Scene Graph |
| **Bridge API** | `ShellBridge` (Generic Transport) | Specific APIs (`SystemAPI`, `AppsAPI`) |

---

### B. Implementing `FUTURE/1-compositor-architecture.md`

**Goal:** Treat WebEngineViews as "Surfaces" layered with QML effects.

**Core Changes Required:**
*   **Zero.** The current PR 1 implementation of `ShellWebEngineView` is sufficient.
*   It exposes `active` and `discardWhenHidden` properties which are essential for the "Multi-Surface" resource management strategy.
*   Transparency is already handled.

**Bolt-On Implementation:**
*   **`MaterialSurface`**: This can be implemented entirely as a QML component (using `ShaderEffect` and `ShaderEffectSource`) in a separate module (e.g., `Quickshell.Materials`).
*   **Compositing Logic**: The "Scene Graph Layering" described in the doc is just standard QML parenting.
    ```qml
    // User's shell.qml
    Item {
        MaterialSurface { ... } // Background
        ShellWebEngineView { ... } // Content
    }
    ```
*   **Conclusion:** The "Compositor Architecture" is a **pattern**, not a C++ feature. It requires no further C++ changes.

---

### C. Implementing `FUTURE/5-bridge-api-spec.md`

**Goal:** Expose system functionality (Battery, Brightness, Launcher) to Web Apps via `QtWebChannel`.

**Core Changes Required (Minimal):**
*   **`ShellBridge` Expansion:** The current `ShellBridge` is a good starting point, but it's monolithic. To support the "Namespaces" (`channel.system`, `channel.apps`), we need a way to register multiple QObjects.
*   **Generic Registry:** Instead of hardcoding every API in `ShellBridge`, we should make `ShellBridge` (or a new `BridgeRegistry`) capable of accepting *any* QObject from QML.

**Bolt-On Implementation:**
*   **API Implementation in QML:**
    We can implement the actual logic (Battery, Brightness) using **existing Quickshell modules** in QML, and expose them to WebChannel.
    *   *Example:* `channel.system.getBatteryStatus()`
    *   *Implementation:* A QML object that reads from `Quickshell.SystemInfo.Battery` and exposes a method/property.
*   **The "Bridge" Component:**
    We can create a generic `WebChannel` wrapper in QML that takes a list of objects.
    ```qml
    // ShellChannel.qml (Enhanced)
    WebChannel {
        property list<QtObject> apis
        registeredObjects: apis
    }
    ```
*   **Usage:**
    ```qml
    ShellChannel {
        apis: [
            SystemAPI { id: system }, // QML component wrapping Quickshell internals
            LauncherAPI { id: launcher } // QML component wrapping Process/Window logic
        ]
    }
    ```

**Verdict:**
*   We do **NOT** need to write C++ classes for `SystemAPI`, `LauncherAPI`, etc.
*   We can leverage the existing rich QML API of Quickshell.
*   **Action Item:** Ensure `ShellWebEngineView` or `ShellChannel` supports registering arbitrary QML objects as WebChannel transports. (Standard `QtWebChannel` already does this).

---

## 3. Summary of "Minimal Core Changes"

1.  **Launch System**: Done (PR 1).
2.  **WebEngine View**: Done (PR 1).
3.  **WebChannel Transport**: Done (PR 1).

**Everything else** (Materials, System APIs, Window Management) can be built as **QML Components** that use the existing Quickshell primitives. We do not need to touch `src/` C++ code for these features.

*   **Materials**: `ShaderEffect` + `Quickshell.Wayland` (for surface handles).
*   **Bridge**: `QtWebChannel` + `Quickshell.Services.*` (exposed via QML wrappers).

This approach keeps the C++ core lean and pushes complexity into the user-modifiable QML layer, perfectly aligning with the project's philosophy.
