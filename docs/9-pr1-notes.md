# Investigation Notes: PR 1 vs Reference Implementation

**Date:** 2025-12-12
**Reference:** `quickshell` PR #351 (WebView support)

## Executive Summary

The reference implementation takes a **dynamic loading** approach to avoid compile-time dependencies, whereas our current PR 1 implementation uses a **compile-time option (`boption`)** with a C++ wrapper (`ShellWebEngineView`).

While the reference approach (dynamic loading) is preferred by the maintainer to avoid "forcing a dependency," our current approach (C++ wrapper) offers superior functionality (transparency, lifecycle management) that is difficult to replicate with a pure QML/dynamic approach without significant refactoring.

**Recommendation:**
1.  **Adopt the `qArgc = 1` fix immediately.** (Critical for stability)
2.  **Stick to `boption` (Compile-Time) for PR 1.** (Preserves `ShellWebEngineView` benefits)
3.  **Defer Dynamic Loading to a future PR.** (Requires porting `ShellWebEngineView` logic to QML)

---

## Detailed Comparison

| Feature | Our PR 1 Implementation | Reference Implementation | Analysis |
| :--- | :--- | :--- | :--- |
| **Dependency** | **Compile-Time (`boption`)**<br>`-DWEBENGINE=ON` links `Qt6::WebEngineQuick` | **Runtime (Dynamic)**<br>`QLibrary` loads `Qt6WebEngineQuick` | Reference avoids build-time dependency. Ours requires it if enabled, but `boption` makes it optional. |
| **Initialization** | `QtWebEngineQuick::initialize()` called directly. | `initialize()` resolved via `QLibrary` symbol lookup. | Reference is safer for users without WebEngine installed. Ours is standard for optional build features. |
| **C++ Wrapper** | **Yes (`ShellWebEngineView`)**<br>Handles transparency, lifecycle, DevTools. | **No**<br>Uses raw `WebEngineView` in QML? | **Critical Difference.** Our wrapper ensures transparency works (setting bg before URL) and manages memory (lifecycle). Reference likely lacks these robust fixes. |
| **Argc Fix** | `qArgc = 0` (Buggy) | **`qArgc = 1` (Fixed)** | **Must Fix.** Chromium requires `argv[0]`. |
| **Build System** | `src/webengine/CMakeLists.txt` | `src/webengine` only has header + test | Reference implementation is minimal C++ side. |

## Deep Dive: Why `ShellWebEngineView` Matters

The reference implementation likely uses `WebEngineView` directly in QML. As noted in `docs/2-qtwebengine-vs-qtwebview.md`, this has pitfalls:
1.  **Transparency**: Must be set *before* URL. Harder to guarantee in pure QML if user sets properties in arbitrary order. `ShellWebEngineView` enforces this in C++.
2.  **Lifecycle**: `ShellWebEngineView` implements `Active`/`Frozen`/`Discarded` states to manage memory (200MB+ per view). Doing this in pure QML is possible (Qt 6.3+) but our C++ implementation provides a robust, reusable component.
3.  **DevTools**: Our wrapper exposes a clean API for this.

## The "Maintainer Preference" Context

Maintainer (`outfoxxed`) said:
> "I do not want to force a dependency... if the initialization logic is complex, an acceptable solution would be to have a skeleton header... and load the module dynamically."

**Interpretation:**
The maintainer wants to avoid *requiring* users to have `qt6-webengine` installed just to build/run Quickshell.
- **Dynamic Loading**: Solves this by checking at runtime.
- **`boption` (Our approach)**: Solves this by allowing users to build *without* the flag (`-DWEBENGINE=OFF`).

**Conclusion:**
`boption` satisfies the "no forced dependency" requirement at the *build configuration* level. Dynamic loading satisfies it at the *runtime binary* level (one binary works for all).
Given Quickshell is often distributed via package managers (Nix, AUR), `boption` is a standard and acceptable pattern (used for Wayland, Pipewire, etc. in `CMakeLists.txt`).

## Action Plan

1.  **Modify `src/launch/launch.cpp`**:
    - Change `qArgc` from `0` to `1`.
    - Ensure `argv` is valid (pass original `argv` or dummy).
2.  **Keep `ShellWebEngineView`**:
    - Do not delete it. It provides essential quality-of-life features.
3.  **Keep `boption`**:
    - It aligns with existing project patterns (`boption(WAYLAND...)`).
    - Document clearly that it defaults to ON (or OFF?) and requires the dependency.
4.  **Delete `/reference/quickshell`**:
    - We have extracted the necessary value (the `qArgc` fix and the dynamic loading concept).

## Code Snippet for `qArgc` Fix

```cpp
// src/launch/launch.cpp

// ...
    delete coreApplication;

    QGuiApplication* app = nullptr;
    // FIX: Chromium needs argc >= 1
    auto qArgC = 1; 
    // We should ideally pass the real argv[0] or a dummy program name
    // But since we are constructing QGuiApplication, we need a valid argv pointer.
    // The reference implementation just changed the int. 
    // We need to ensure argv is safe to read if qArgC > 0.
    // In launch(), 'argv' is passed in. So we can use it.
    
    if (pragmas.useQtWebEngineQuick) {
        web_engine::init();
    }

    if (pragmas.useQApplication) {
        app = new QApplication(qArgC, argv);
    } else {
        app = new QGuiApplication(qArgC, argv);
    }
// ...
```
