# Future Architecture: Implementation Roadmap

**Status:** Execution Plan
**Timeline:** 3-6 Months

## Phase 1: The Compositor Core (Months 1-2)
**Goal:** Ship a functional shell with Multi-WebEngineView architecture.

*   [ ] **Compositor**: Build `SurfaceManager.qml` to handle stacking of `WebEngineView` + `MaterialSurface`.
*   [ ] **Material Engine**: Implement the `ShaderEffect` pipeline (Blur + Tint + Noise) for M1-M5.
*   [ ] **Bridge**: Implement basic `QtWebChannel` API (`system`, `shell`, `settings`).
*   [ ] **Content**: Build the first "Headless" web surface (e.g., The Bar) using SolidJS.
*   [ ] **Integration**: Verify Wayland Layer-Shell anchoring for the Bar surface.

## Phase 2: The Ecosystem (Months 3-4)
**Goal:** Expand to a full desktop environment.

*   [ ] **Surfaces**: Add Panel, Notification Center, and Launcher surfaces.
*   [ ] **Materials**: Expand scale to M1-M12. Tune "Apple-like" vibrancy.
*   [ ] **Bridge**: Add `apps` namespace (launching, window management).
*   [ ] **Design System**: Adam Morse delivers the full `Surface` / `Panel` / `Button` primitive library.

## Phase 3: Polish & Advanced Features (Months 5-6)
**Goal:** "Beyond SceneFX" visuals.

*   [ ] **Region Reporting**: Implement the `ResizeObserver` bridge for intra-surface materials (Sidebars).
*   [ ] **Advanced Shaders**: Add Rim Lighting and SDF Shadows.
*   [ ] **Performance**: Optimize memory usage (shared QtWebEngine profile).
*   [ ] **Research**: Begin investigation into Option A (QtWebEngine layer exposure) as a background task.

## Phase 4: Long Term (Year 1+)
*   [ ] **Vulkan Compute**: Move heavy blur passes to Compute Shaders.
*   [ ] **Full Environment**: Replace Hyprland/Sway components (Lockscreen, OSD) entirely with Quickshell surfaces.
