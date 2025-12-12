# Next Steps: Packaging and Usage

**Date:** 2025-12-12
**Status:** Planning

## 1. Packaging for Fedora COPR

Once your PR is stable (or even before, for testing), you can package `quickshell-web` for Fedora.

### Prerequisites
- Fedora Account (FAS)
- `fedora-packager` installed locally
- `copr-cli` configured

### Steps to Create COPR

1.  **Create Project:**
    ```bash
    copr-cli create quickshell-web --chroot fedora-43-x86_64
    ```

2.  **Prepare Spec File (`quickshell.spec`):**
    You will need to modify the standard Quickshell spec file to include the WebEngine build option.

    ```spec
    %global forgeurl https://github.com/mecattaf/quickshell
    # ... standard preamble ...

    BuildRequires: cmake
    BuildRequires: gcc-c++
    BuildRequires: ninja-build
    BuildRequires: qt6-qtbase-devel
    BuildRequires: qt6-qtdeclarative-devel
    BuildRequires: qt6-qtwayland-devel
    # NEW DEPENDENCIES
    BuildRequires: qt6-qtwebengine-devel
    BuildRequires: qt6-qtwebchannel-devel

    %build
    %cmake \
        -DWEBENGINE=ON \
        -DCMAKE_BUILD_TYPE=Release
    %cmake_build

    %install
    %cmake_install
    ```

3.  **Build Package:**
    ```bash
    # Assuming you have a SRPM or using SCM
    copr-cli build-scm quickshell-web \
        --clone-url https://github.com/mecattaf/quickshell \
        --commit <your-branch-or-commit-hash> \
        --spec quickshell.spec
    ```

## 2. Internal Testing & Iteration

1.  **Enable the COPR:**
    ```bash
    sudo dnf copr enable mecattaf/quickshell-web
    ```

2.  **Install:**
    ```bash
    sudo dnf install quickshell
    ```

3.  **Verify:**
    Run your shell config. If `//@ pragma EnableWebEngine` is present, it should now launch with full web support.

## 3. Future Iteration (PR 2+)

Once the base PR is merged/stable:
1.  **Material System:** Re-introduce `MaterialSurface` and `MaterialNode` (shaders).
2.  **Dynamic Loading:** If the maintainer insists, refactor `ShellWebEngineView` to be a pure QML wrapper around a dynamically loaded plugin, or use the `QLibrary` approach for the C++ component itself (complex).
