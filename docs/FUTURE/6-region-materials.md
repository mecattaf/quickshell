# Future Architecture: Region-Based Materials

**Status:** Approved Vision (Phase 2)
**Concept:** Intra-surface material overlays
**Inspiration:** macOS Sidebar / Header blending

## 1. The Limitation of "One Surface, One Material"

The Multi-WebEngineView architecture (Phase 1) assigns one material level to an entire web app (e.g., the whole Panel is M3).

However, complex apps (like a File Manager or Music Player) often need **mixed materials**:
*   Sidebar: M4 (Thick)
*   Content: M2 (Thin)
*   Header: M3 (Medium)

## 2. The Solution: Region Reporting

To achieve this without spawning 3 separate WebEngineViews for one app, we introduce **Region Reporting**.

### The Mechanism
1.  **Marking**: The Web App marks specific DOM elements with data attributes.
    ```html
    <div data-material-region="sidebar" data-level="M4">...</div>
    ```
2.  **Reporting**: A `ResizeObserver` in the Content Script tracks the geometry (x, y, w, h) of these marked elements relative to the viewport.
3.  **Sync**: The script sends this geometry to QML via `QtWebChannel` (batched, e.g., every 16ms or on change).
4.  **Rendering**: QML creates *additional* `MaterialSurface` items overlaid on top of the base surface, matching the reported geometry.

## 3. The "Virtual Subsurface"

This effectively creates "Virtual Subsurfaces".

*   **Web View**: Renders the content (text, icons) transparently over the region.
*   **QML**: Renders a specific blur rectangle *behind* that specific DOM region.

## 4. Performance & Constraints

*   **Latency**: There is a 1-frame lag between DOM layout changes and QML shader updates.
*   **Usage Rule**: Only use this for **major structural regions** (Sidebars, Headers). Do not use for small items (Buttons, Cards) - those should use CSS styling tokens.
*   **Scroll Sync**: If the region scrolls, the reporter must track scroll offsets. Ideally, material regions are `position: fixed` or `sticky`.

## 5. Why Not Just Use CSS Backdrop Filter?

CSS `backdrop-filter: blur()` is performant but:
1.  It cannot blur the *desktop wallpaper* behind the window (it only blurs DOM content).
2.  It doesn't support the advanced RHI effects (noise, vibrancy, rim lighting) that define our design system.
3.  It creates GPU layer explosion in Chromium.

Region Reporting gives us the "Apple Look" (Vibrancy + Noise) which CSS cannot do.
