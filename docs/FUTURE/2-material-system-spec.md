# Future Architecture: The Material System Spec

**Status:** Approved Vision
**Inspiration:** Apple `NSVisualEffectView` / `NSMaterial`

## 1. The Material Level Concept

Instead of arbitrary blur values, we define a strict semantic scale of **Material Levels (M1-M12)**. This ensures consistency across the OS. A "Material" is a combination of:
*   Blur Radius
*   Saturation / Vibrancy
*   Tint Color & Opacity
*   Noise Texture
*   Rim Lighting Intensity

## 2. The Scale (M1 - M12)

| Level | Name | Use Case | Visual Characteristics |
| :--- | :--- | :--- | :--- |
| **M1** | `UltraThin` | HUDs, Tooltips | Very subtle blur, high transparency. |
| **M2** | `Thin` | Context Menus | Light blur, distinct border. |
| **M3** | `Medium` | Headers, Titlebars | Standard "frosted glass". |
| **M4** | `Thick` | Sidebars | Heavy blur, strong vibrancy. |
| **M5** | `Chrome` | Main Window Background | Opaque-ish, deep depth. |
| **M6** | `Light` | Light-mode specific | Bright, airy, ceramic feel. |
| **M7** | `Dark` | Dark-mode specific | Deep, slate-like, high contrast. |
| **M8** | `Platinum` | High-value surfaces | Metallic sheen, anisotropic noise. |
| **M9** | `Overlay` | Modals, Alerts | Background dimming + blur. |
| **M10** | `Sheet` | Slide-over panels | High elevation shadows. |
| **M11** | `Acrylic` | Windows-style | Noise-heavy, cross-hatch texture. |
| **M12** | `Vibrant` | Media overlays | High saturation boost (for album art). |

## 3. Implementation Contract

### QML Side (The Provider)
The `MaterialEngine` exposes these levels as an enum or string union.
```qml
MaterialSurface {
    level: "M4" // Automatically configures blur: 64, tint: #22000000, etc.
}
```

### Web Side (The Consumer)
Web components do **not** define blur. They request a semantic level.
```html
<!-- Adam Morse's Headless Component -->
<Surface level="M4">
  <SidebarContent />
</Surface>
```

## 4. Theme Token Injection
When a surface uses `M4`, QML injects corresponding CSS tokens into that specific WebEngineView:
*   `--surface-text-primary`
*   `--surface-border-color`
*   `--surface-accent`
*   `--surface-radius`

This ensures the HTML content (text, borders) visually matches the glass material rendered behind it.
