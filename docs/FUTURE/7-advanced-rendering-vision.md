# Future Architecture: Advanced Rendering Vision

**Status:** Long-Term Research & RHI Strategy
**Goal:** Beyond SceneFX - The "Holy Grail" of Compositing

## 1. The Power of Qt RHI (Vulkan/Metal)

Quickshell's secret weapon is the **Qt Rendering Hardware Interface (RHI)**. Unlike Electron (which relies on CSS/Skia), Qt Quick renders via a modern 3D API (Vulkan on Linux, Metal on macOS).

This allows us to implement **SceneFX-class effects** directly in the shell:
*   **Dual-Source Blending**: For perfect subpixel text rendering on transparent glass.
*   **Compute Shaders**: For expensive blurs (Gaussian/Bokeh) that would choke a CSS renderer.
*   **SDF Rendering**: For resolution-independent rounded corners and shadows.

We are effectively building a **Game Engine for UI**.

## 2. The "Nuclear" Option (Option A)

In our architectural analysis, we identified **Option A**: *Patching QtWebEngine to expose Chromium Compositor Layers*.

### The Concept
Currently, `QtWebEngine` flattens the entire web page into one texture.
The "Holy Grail" is to modify `DelegatedFrameNode` in Qt's source code to **expose the internal RenderPasses** as separate `QSGNode`s.

### Why do this?
If we achieve this, we can interleave QML effects *inside* the DOM structure without splitting the web app into multiple views.
*   *Layer 1*: DOM Background -> **QML Blur** -> *Layer 2*: DOM Content -> **QML Particle Effect** -> *Layer 3*: DOM Modal.

### Status
*   **Current Strategy**: Option C (Multi-WebEngineView) - Pragmatic, works now.
*   **Future Research**: Option A - High risk, high reward. We will monitor Qt 6.8+ for better RHI integration hooks, but we will not block shipping on this.

## 3. Beyond SceneFX

SwayFX/SceneFX is limited to what wlroots exposes (windows).
Our architecture allows **Intra-Surface Effects**:

*   **Interactive Lighting**: Mouse cursor acting as a point light source for rim lighting (like Windows Acrylic but smoother).
*   **Physics-Based Materials**: Glass that distorts background content (refraction) based on "thickness".
*   **Reactive Vibrancy**: Saturation boosts that react to the *audio* playing in the system (visualizer integration).

This is possible because we control the **entire render loop** in QML.
