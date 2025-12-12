# Future Work: Theme System Integration

This document tracks the planned integration between `ShellBridge` and Quickshell's theme system.

## Current State (POC 1)

The `ShellBridge` currently uses hardcoded defaults:

```cpp
mDarkMode = false;
mAccentColor = "#007AFF";
```

Theme tokens returned by `getThemeTokens()` are also hardcoded with iOS-style defaults.

## Planned Integration

### 1. Connect to Quickshell's Theme System

When Quickshell exposes a theme/appearance API, `ShellBridge` should:

```cpp
// In ShellBridge constructor
connect(QuickshellTheme::instance(), &QuickshellTheme::darkModeChanged,
        this, [this]() {
    setDarkMode(QuickshellTheme::instance()->darkMode());
});

connect(QuickshellTheme::instance(), &QuickshellTheme::accentColorChanged,
        this, [this]() {
    setAccentColor(QuickshellTheme::instance()->accentColor().name());
});
```

### 2. Dynamic Theme Token Generation

The `getThemeTokens()` method should pull from the system:

- **Colors**: From Quickshell's color scheme or user configuration
- **Spacing**: From a centralized spacing scale
- **Radius**: From widget/panel corner radius configuration

### 3. CSS Custom Property Injection

Consider automatically injecting theme tokens as CSS custom properties
when the page loads, rather than requiring explicit JavaScript calls:

```javascript
// Automatic injection in WebEngineView
document.documentElement.style.setProperty('--color-accent', tokens.colors.accent);
```

### 4. Signal Batching

Theme changes should respect `propertyUpdateInterval` to avoid excessive
updates during rapid theme changes (e.g., during wallpaper-based color
extraction animations).

## Integration with eqsh Patterns

From the `docs/3-eqsh-analysis.md` reference:

### AccentColor Singleton Pattern
```qml
// eqsh uses ColorQuantizer to extract accent from wallpaper
AccentColor.qml {
    property color accent: ColorQuantizer.extract(wallpaper)
    property color preferredTextColor: computeAccessibleColor(accent)
}
```

ShellBridge should expose similar computed properties for accessible
text colors based on accent.

### Config Persistence
Theme preferences should persist via Quickshell's `FileView` + `JsonAdapter`
pattern with debounced writes.

## Material You Integration

From `docs/1-quickshell-webview-patterns.md`:

The color generation pipeline could be exposed to JS:

```javascript
shell.generateMaterialPalette(sourceColor).then(palette => {
    // palette.primary, palette.surface, palette.onSurface, etc.
});
```

This would enable web content to use Material You theming consistent
with the native shell.

## Priority

- **P1**: Connect darkMode/accentColor to actual system state
- **P2**: Dynamic theme token generation
- **P3**: Material You palette generation
- **P4**: Automatic CSS injection
