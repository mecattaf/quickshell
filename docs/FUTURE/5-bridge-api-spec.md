# Future Architecture: The Bridge API Spec

**Status:** Approved Vision
**Technology:** QtWebChannel
**Purpose:** System Access for Web Apps

## 1. The Bridge Concept

Web apps in the shell are "unprivileged" by default. They cannot access the filesystem, spawn processes, or talk to Wayland directly. They must go through the **Bridge**.

The Bridge is a set of QObjects exposed via `QtWebChannel`.

## 2. API Namespaces

The API is organized into namespaces to keep it clean and versionable.

### `channel.system`
*   `getBatteryStatus()`: Returns battery % and state.
*   `setBrightness(level)`: Controls screen brightness.
*   `getVolume()` / `setVolume(level)`: Audio control.
*   `powerOff()` / `reboot()` / `suspend()`: Power management.

### `channel.shell`
*   `openLauncher()`: Triggers the app launcher surface.
*   `toggleNotificationCenter()`: Toggles the notification panel.
*   `setSurfaceSize(width, height)`: Requests a resize of the current surface.
*   `requestFocus()`: Asks the compositor for keyboard focus.

### `channel.settings`
*   `getTheme()`: Returns current theme (dark/light, accent color).
*   `setTheme(themeId)`: Switches the global shell theme.
*   `get(key)` / `set(key, value)`: Persistent key-value storage (backed by SQLite/JSON).

### `channel.apps`
*   `launch(appId)`: Launches a desktop application.
*   `listRunning()`: Returns list of open windows (for taskbar).
*   `activate(windowId)`: Focuses a specific window.

## 3. Implementation Pattern

### QML Side
```qml
WebChannel {
    registeredObjects: [
        SystemAPI { id: systemApi },
        ShellAPI { id: shellApi }
    ]
}
```

### JS Side (The Client)
```javascript
// In the SolidJS App
import { useBridge } from '@shell/bridge';

const { system } = useBridge();
const battery = await system.getBatteryStatus();
```

## 4. Security Model
The Bridge acts as a firewall. We only expose safe, high-level methods. We do **not** expose raw `QProcess` or arbitrary file access. This ensures that even if a web app is compromised (e.g., via XSS), it cannot destroy the host system.
