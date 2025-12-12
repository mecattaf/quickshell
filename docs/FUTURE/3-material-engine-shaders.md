# Future Architecture: MaterialEngine & Shaders

**Status:** Approved Vision
**Technology:** Qt RHI / Vulkan / ShaderEffect
**Reference:** `eqsh` / `SceneFX`

## 1. The Engine Core

The `MaterialEngine` is the QML subsystem responsible for rendering the "glass" effects. It replaces the need for C++ `QSGRenderNode` by leveraging the standard `ShaderEffect` type, which compiles to Vulkan/Metal/Direct3D automatically.

## 2. Shader Pipeline

The rendering pipeline for a single `MaterialSurface` consists of multiple passes:

### Pass 1: Backdrop Sampling
Captures the screen content behind the surface.
*   *Optimization*: Use `ShaderEffectSource` with `live: false` for static backgrounds, or `recursive: false` to avoid feedback loops.

### Pass 2: Gaussian Blur (Dual-Pass)
A separable Gaussian blur (Horizontal + Vertical).
*   **Kernel Size**: Dynamic based on Material Level.
*   **Downsampling**: Render to a smaller texture (1/2 or 1/4 size) for heavy blurs (M4+) to save GPU bandwidth.

### Pass 3: Composition & Effects (The "Uber-Shader")
Combines the blurred texture with:
1.  **Tint Layer**: `mix(blur, tintColor, opacity)`
2.  **Noise Layer**: Adds film grain/dither to prevent banding and add texture.
3.  **Vibrancy**: Increases saturation of the background (essential for the "Apple" look).
4.  **Masking**: Applies the SDF rounded corner mask.
5.  **Rim Lighting**: Adds the inner glow based on light direction vectors (derived from `eqsh`).

## 3. The `MaterialSurface` Component

```qml
// src/webengine/MaterialSurface.qml
Item {
    id: root
    property string level: "M3"
    
    // Internal logic maps 'level' to specific shader uniforms
    property real _blurRadius: MaterialConfig.getBlur(level)
    property color _tint: MaterialConfig.getTint(level)
    property real _noise: MaterialConfig.getNoise(level)

    ShaderEffect {
        anchors.fill: parent
        fragmentShader: "qrc:/shaders/material.frag.qsb"
        // ... uniforms ...
    }
}
```

## 4. Advanced Visuals (Vulkan Powered)

By using `ShaderEffect` in Qt 6, we tap into the RHI (Rendering Hardware Interface). This allows us to implement effects that go beyond standard CSS:
*   **Anisotropic Noise**: For "brushed metal" looks.
*   **Chromatic Aberration**: Subtle edge fringing for "glass" refraction.
*   **SDF Shadows**: Ray-marched shadows for perfect rounded corners without texture assets.

This is the "Vulkan-accelerated" layer that differentiates Quickshell from a standard Electron app.
