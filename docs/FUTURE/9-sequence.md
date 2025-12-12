# Future Architecture: The Implementation Sequence

**Status:** Detailed Execution Guide
**Philosophy:** "Thick Config, Thin Core"
**Goal:** Maximize visual impact, minimize upstream Quickshell PRs.

## 1. The "Upstream vs. Downstream" Split

This is the most critical architectural decision. We want to avoid turning Quickshell into "The Tom Shell". Quickshell should remain a generic toolkit.

| Component | Location | PR Required? | Why? |
| :--- | :--- | :--- | :--- |
| `WebEngineView` Support | **Upstream** (`src/webengine`) | **YES (PR 1)** | Core capability. Needs C++ init. |
| `QtWebChannel` Support | **Upstream** (`src/webengine`) | **YES (PR 1)** | Core capability. Needs C++ injection. |
| `MaterialSurface` (QML) | **Downstream** (User Config) | **NO** | Specific to your design system. |
| `SurfaceManager` (QML) | **Downstream** (User Config) | **NO** | Your specific compositor logic. |
| `ShaderEffect` (.frag) | **Downstream** (User Config) | **NO** | Your specific visual look. |
| `Bridge` APIs (JS/QML) | **Downstream** (User Config) | **NO** | Your specific system integrations. |

**Conclusion:** After PR 1 (WebEngine + WebChannel), **zero** additional C++ PRs are strictly required to achieve the "Apple Look". Everything else can be built in your local `~/.config/quickshell` using the exposed primitives.

---

## 2. The Material Levels (M1-M12) Explained

We define 12 levels of material "heaviness". These are not just opacity changes; they are fundamentally different shader configurations.

### Group A: The "Air" Materials (M1-M4)
*   **Target:** HUDs, Tooltips, Context Menus.
*   **Pipeline:** `Backdrop -> FastBlur (Radius 8-16px) -> Tint`.
*   **Cost:** Low.
*   **Visuals:** Crisp, lightweight, preserves background legibility.

### Group B: The "Glass" Materials (M5-M8)
*   **Target:** Sidebars, Panels, Headers.
*   **Pipeline:** `Backdrop -> Downsample (1/2) -> GaussianBlur (Radius 32-64px) -> Noise -> Vibrancy -> Rim Light`.
*   **Cost:** Medium (due to downsampling).
*   **Visuals:** The classic "Apple" frosted glass. High saturation boost.

### Group C: The "Structural" Materials (M9-M12)
*   **Target:** Main Window Backgrounds, Modals, Sheets.
*   **Pipeline:** `Backdrop -> Downsample (1/4) -> HeavyBlur (Radius 128px+) -> Anisotropic Noise -> Deep Tint`.
*   **Cost:** Medium-High.
*   **Visuals:** Opaque-ish, ceramic or metallic feel. Background is abstract color only.

---

## 3. The Shader Pipeline (The "Secret Sauce")

To achieve the "non-standard" look (i.e., better than standard CSS blur), we implement this pipeline in a single `.frag.qsb` shader used by `ShaderEffect`.

### Stage 1: The Read (Backdrop)
We use `ShaderEffectSource` to capture the screen behind the surface.
*   *Optimization:* For M5-M12, we set `sourceRect` to a downsampled grid to save bandwidth.

### Stage 2: The Blur (The Expensive Part)
Standard Gaussian blur is O(R^2). We use a **Dual-Pass Separable Blur**.
1.  **Pass X:** Blur horizontally. Write to temporary buffer.
2.  **Pass Y:** Blur vertically. Read from temp buffer.
*   *Note:* In QML `ShaderEffect`, this often requires two `ShaderEffect` items chained together.

### Stage 3: The Composite (The "Vibe")
This is where we beat CSS. We apply these operations per-pixel:
1.  **Vibrancy:** `color = mix(grayscale(color), color, 1.5)` (Boost saturation of background).
2.  **Tint:** `color = mix(color, tintColor, tintAlpha)`.
3.  **Noise:** `color += (random(uv) - 0.5) * noiseIntensity` (Dithering to fix banding).
4.  **Rim Light:** Calculate distance to edge. If < `rimWidth`, add `rimColor * rimIntensity`.
5.  **Mask:** `alpha *= smoothstep(radius, radius - 1.0, sdf_rounded_box(uv))`.

---

## 4. The Implementation Sequence

### Level 0: The Foundation (PR 1)
*   **Action:** Submit PR 1 to Quickshell.
*   **Result:** You have `WebEngineView` and `WebChannel` available in QML.
*   **Verification:** Run a "Hello World" HTML page that logs to QML console.

### Level 1: The First Surface (Downstream)
*   **Action:** Create `~/.config/quickshell/SurfaceManager.qml`.
*   **Logic:** Instantiate *one* `WebEngineView` loading `http://localhost:3000` (your Solid dev server).
*   **Result:** A floating web window managed by Quickshell.

### Level 2: The First Material (Downstream)
*   **Action:** Create `~/.config/quickshell/MaterialSurface.qml`.
*   **Logic:** Add a `ShaderEffect` with a simple `FastBlur` + `RectangularMask`. Place it *behind* the WebEngineView.
*   **Result:** Your web app now has a real glass background.

### Level 3: The Bridge (Downstream)
*   **Action:** Create `~/.config/quickshell/bridge/SystemAPI.qml`.
*   **Logic:** Expose a function `getBattery()` that returns `System.battery.percent`. Register it in `WebChannel`.
*   **Result:** Your Solid app can display real system data.

### Level 4: Multi-Surface Compositor (Downstream)
*   **Action:** Update `SurfaceManager.qml` to handle a `ListModel` of surfaces.
*   **Logic:**
    *   Surface A: "Bar" (Top anchor, M5 material).
    *   Surface B: "Panel" (Center anchor, M8 material).
*   **Result:** You are now running a compositor.

### Level 5: The "Apple" Shaders (Downstream)
*   **Action:** Replace the simple `FastBlur` in Level 2 with the complex `.frag` shader (Noise, Vibrancy, Rim).
*   **Result:** The visual fidelity jumps from "Linux Rice" to "macOS/Windows Acrylic".

### Level 6: Region Reporting (Downstream - Advanced)
*   **Action:** Implement the `ResizeObserver` loop in JS and the "Virtual Subsurface" spawner in QML.
*   **Result:** Your single "Panel" web app now has a distinct sidebar material separate from its main content.

---

## 5. Summary of "Required PRs"

*   **PR 1 (WebEngine + WebChannel):** **REQUIRED.**
*   **PR 2 (Material System):** **CANCELLED.** We realized we can do this entirely in user config using `ShaderEffect`.
*   **PR 3 (Advanced RHI):** **OPTIONAL.** Only if we hit performance walls with `ShaderEffect` and need raw Vulkan compute shaders (unlikely for < 10 surfaces).

**You are unblocked.** You can build 95% of this system *today* in your config directory, once PR 1 lands.

---

## Addendum: The RHI & Vulkan Reality

**Q: You mentioned "Advanced RHI" as a future step. Does that mean the current `ShaderEffect` approach is not GPU accelerated?**

**A: No. The current `ShaderEffect` approach IS fully GPU accelerated via Vulkan.**

Here is the technical reality of Qt 6:
1.  **Qt 6 uses RHI (Rendering Hardware Interface)** by default. It abstracts Vulkan, Metal, and Direct3D.
2.  When you write a `ShaderEffect` in QML, the `.qsb` shader compiler translates your GLSL into **SPIR-V (for Vulkan)**, MSL (for Metal), and HLSL (for D3D).
3.  Therefore, your "standard" `ShaderEffect` pipeline (Blur + Noise + Vibrancy) **runs natively on the GPU via Vulkan** on Linux. It is extremely fast.

**So what is "Advanced RHI"?**
When we say "Advanced RHI" or "PR 3", we refer to **Compute Shaders** and **Raw Render Nodes**.
*   **Standard `ShaderEffect`**: Runs in the fragment pipeline. Good for visual effects.
*   **Compute Shaders**: Run generic math on the GPU. Necessary for *massive* blurs (radius > 200px) or complex physics simulations that would choke a fragment shader.

**Verdict:** You do **not** need "Advanced RHI" to achieve the Apple look. The standard `ShaderEffect` (running on Vulkan) is more than powerful enough for M1-M12 materials.
