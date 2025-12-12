# Future Architecture: The Compositor Model

**Status:** Approved Vision
**Core Concept:** Quickshell is not a browser wrapper; it is a **compositor**.

## 1. The Paradigm Shift

The fundamental realization is that we are not building a "web app inside a Qt window." We are building a **Wayland-style compositor** where the "clients" happen to be internal WebEngineView instances.

| Traditional Web App | Our Compositor Architecture |
| :--- | :--- |
| Single DOM Tree | **Multiple Independent Surfaces** |
| CSS `z-index` layering | **Scene Graph Layering** |
| DOM-based Blur (Backdrop Filter) | **Compositor-Level Blur (ShaderEffect)** |
| Single Process | **Distributed Processes (Multi-Renderer)** |

## 2. The Multi-WebEngineView Strategy

To achieve true desktop-class visuals (blur, vibrancy, parallax) without patching QtWebEngine, we treat each distinct UI region as a separate **Surface**.

### The Surface Unit
A "Surface" consists of:
1.  **MaterialLayer**: A QML `ShaderEffect` providing the background (blur, tint, rim).
2.  **ContentLayer**: A `WebEngineView` rendering the UI (HTML/JS).

```qml
// Conceptual QML Structure
Item {
    id: surfaceRoot
    
    MaterialSurface {
        level: Material.Level3 // The "Glass"
        anchors.fill: parent
    }
    
    WebEngineView {
        url: "http://shell-ui/panel.html" // The "Content"
        anchors.fill: parent
        backgroundColor: "transparent"
    }
}
```

## 3. Why This "Feels Weird" But Is Correct

It feels inefficient to spawn multiple browser engines. However, this mirrors exactly how a Wayland compositor works:
*   **wlroots** manages multiple client surfaces (windows).
*   **SceneFX** applies blur *behind* those surfaces.
*   **Clients** (Firefox, Terminal) render their content on top.

In our case:
*   **Qt Quick** is the Compositor (wlroots).
*   **MaterialEngine** is the Effects Layer (SceneFX).
*   **WebEngineViews** are the Clients.

## 4. Benefits
1.  **True Z-Ordering**: We can interleave native QML effects between web surfaces.
2.  **Performance**: UI threads are isolated. A heavy animation in the "Panel" won't stutter the "Bar".
3.  **Stability**: If the "Widgets" renderer crashes, the "Lockscreen" stays alive.
4.  **No DOM Hacks**: We don't need to reconstruct the DOM in QML. The DOM lives in Chromium; the Surface lives in Qt.

## 5. Resource Management
*   **Memory**: Each view costs ~150-200MB. For a shell with 5 surfaces (Bar, Panel, Notifications, Launcher, OSD), this is ~1GB. Acceptable for modern desktops.
*   **Optimization**: We will use a shared `QtWebEngine` profile and process model where possible to deduplicate resources.
