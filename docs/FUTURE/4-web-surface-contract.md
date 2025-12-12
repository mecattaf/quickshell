# Future Architecture: The Web Surface Contract

**Status:** Approved Vision
**Role:** Adam Morse / Design System Team
**Technology:** SolidJS / Kobalte / Headless Components

## 1. The Division of Labor

To scale development, we separate the **Compositor** (Qt/C++) from the **Content** (Web/JS).

| Responsibility | Owner | Technology |
| :--- | :--- | :--- |
| **Physics & Materials** | Tom (Compositor) | QML / Shaders |
| **Semantics & Layout** | Adam (Content) | SolidJS / CSS |
| **System API** | Bridge | QtWebChannel |

## 2. The "CF" Primitive Equivalent

Adam's goal is to build a **Headless Component Library** that acts as the "Standard Library" for the shell. These components do **not** implement blur or window management. They implement **semantics**.

### Core Primitives
*   `<Surface>`: The root of any shell app. Declares the intended material level.
*   `<Panel>`: A structural container for content.
*   `<Popover>`: A floating element that requests a higher Z-index.
*   `<ButtonPrimitive>`: A semantic button that consumes theme tokens.

## 3. The Surface Contract

Every web application running in the shell must adhere to this contract:

1.  **Root Declaration**: The app must wrap its content in a `<Surface>` provider.
    ```jsx
    // App.jsx
    <Surface role="panel" level="M3">
      <Content />
    </Surface>
    ```
2.  **Token Consumption**: The app must strictly use CSS variables provided by the shell for all structural colors.
    ```css
    .button {
      background: var(--surface-accent); /* Provided by QML */
      color: var(--surface-text-primary); /* Provided by QML */
      border-radius: var(--surface-radius); /* Provided by QML */
    }
    ```
3.  **No Native Blur**: The app must **never** use `backdrop-filter: blur()`. It relies entirely on the QML compositor to render the glass behind it.

## 4. The "Surface RFC"

Adam defines the valid "Surface Roles" that an app can request. This ensures that a "Panel" looks like a Panel and a "Tooltip" looks like a Tooltip across the entire OS.

*   `role="bar"` -> M2
*   `role="panel"` -> M3
*   `role="overlay"` -> M5
*   `role="toast"` -> M9
