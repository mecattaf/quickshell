# PR 2 Scoping: The Material System

**Date:** 2025-12-12
**Status:** Planned (Post-PR 1)
**Focus:** Adding `MaterialSurface` (RHI-based visual effects) with minimal disruption.

## 1. Complexity Assessment

I have analyzed the staged files in `/docs/PR-2-materials`.

| Component | Complexity | Invasiveness | Notes |
| :--- | :--- | :--- | :--- |
| **MaterialNode** (`QSGRenderNode`) | **High** (RHI) | **Low** (Self-contained) | Uses low-level Qt RHI (Vulkan/OpenGL abstraction). Requires manual buffer management. |
| **MaterialSurface** (`QQuickItem`) | **Low** | **Low** | Standard QML item wrapper. No side effects. |
| **Shaders** (`.vert`, `.frag`) | **Medium** | **Low** | Requires `Qt::ShaderTools` build step. |
| **Build System** | **Medium** | **Low** | Needs `qt6_add_shaders` and private Qt headers. |

**Verdict:** PR 2 is **technically complex** (graphics programming) but **architecturally contained**. Unlike PR 1, it does not touch `launch.cpp`, `main.cpp`, or core initialization logic. It is purely a visual add-on.

## 2. Invasiveness & Risk

**Risk:** The main risk is **build compatibility**.
-   `MaterialNode` includes `<private/qrhi_p.h>` and `<private/qsgrendernode_p.h>`.
-   These are **private Qt headers**. They offer no API guarantees between Qt versions.
-   *Mitigation:* We must ensure the build succeeds across the Qt versions tested in CI (6.6 - 6.10). RHI APIs have been relatively stable in Qt 6, but this is the fragility point.

**Risk:** Shader Compilation.
-   Requires `qt6_add_shaders` (Qt Shader Tools).
-   *Mitigation:* Ensure `Qt::ShaderTools` is added to the `WEBENGINE` build option dependencies.

## 3. "Light on Quickshell" Strategy

To keep PR 2 as non-invasive as possible:

1.  **Strict Isolation:**
    -   Keep all files in `src/webengine`.
    -   Do *not* expose `MaterialSurface` to the global `Quickshell` namespace if possible, or keep it strictly under `Quickshell.WebEngine`.

2.  **Conditional Compilation:**
    -   Ensure `MaterialNode` is *only* compiled when `WEBENGINE=ON`.
    -   The private headers (`Qt::GuiPrivate`) should only be linked when this option is active.

3.  **Fallback Mode:**
    -   If RHI initialization fails (e.g., software rendering), `MaterialNode` should fail gracefully (draw nothing or a simple color) rather than crashing the shell.

## 4. Implementation Plan (Draft)

1.  **Move Files:** Restore files from `docs/PR-2-materials` to `src/webengine`.
2.  **Update CMake:**
    ```cmake
    find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
    # ...
    qt6_add_shaders(quickshell-webengine "webengine_shaders"
        PREFIX "/Quickshell/WebEngine/shaders"
        FILES shaders/material.vert shaders/material.frag
    )
    target_link_libraries(quickshell-webengine PRIVATE Qt::GuiPrivate Qt::ShaderTools)
    ```
3.  **Verify RHI:** Test on Vulkan (Linux standard) and OpenGL.

## 5. Conclusion

PR 2 will be **less invasive** to the codebase structure than PR 1, but **more sensitive** to Qt version changes due to private RHI usage. It fits the "optional module" philosophy perfectly.
