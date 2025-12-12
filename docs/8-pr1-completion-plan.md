# PR 1 Completion Plan

**Date:** 2025-12-12
**Status:** Planning

---

## 1. Review of Conventions & Discrepancies

Based on the analysis of the reference PR thread (#351) and the current implementation, here are the findings:

### ✅ Aligned
- **Initialization**: We call `QtWebEngineQuick::initialize()` and set `Qt::AA_ShareOpenGLContexts`.
- **Pragma**: We use a pragma (`//@ pragma EnableWebEngine`) to control activation.
- **Tests**: We have a manual test suite (`src/webengine/test/manual`).

### ❌ Discrepancies (To Be Addressed)

#### 1. `qArgc` Value
- **Reference PR**: Changed `qArgc` from `0` to `1` in `launch.cpp` because Chromium expects `argv[0]` (program name).
- **Current State**: `qArgc` is still `0` in `src/launch/launch.cpp`.
- **Action**: **MUST FIX**. Change `qArgc` to `1` (and ensure `argv` is valid).

#### 2. Dependency Handling (Build vs Runtime)
- **Reference PR**: The maintainer explicitly requested **no forced dependency**. The contributor implemented **dynamic loading** (loading the symbol at runtime) and removed CMake options.
- **Current State**: We use a **compile-time option** (`-DWEBENGINE=ON`) which links `Qt6::WebEngineQuick` and `Qt6::WebChannel`.
- **Analysis**:
    - Our implementation introduces `ShellWebEngineView` (C++ class inheriting `QQuickWebEngineView`). This makes dynamic loading extremely difficult (requires a full plugin architecture).
    - The reference PR likely used the stock `WebEngineView` QML type, which allows the C++ binary to be agnostic if the QML module is loaded dynamically.
    - **Decision**: For PR 1, we will stick with the **Compile-Time Option (`boption`)**.
    - **Justification**:
        - Quickshell uses `boption` for all other optional dependencies (Wayland, Hyprland, Pipewire, etc.).
        - `ShellWebEngineView` provides critical fixes (transparency, lifecycle) that justify the C++ dependency.
        - Refactoring to a plugin is out of scope for PR 1.
        - We will document this deviation clearly in the PR description.

---

## 2. Action Items

### Step 1: Fix `qArgc` in `launch.cpp`
Modify `src/launch/launch.cpp`:
```cpp
// Current
auto qArgC = 0;

// New
auto qArgC = 1; // Required for WebEngine/Chromium
```

### Step 2: Verify `ShellWebEngineView` Necessity
Confirm that `ShellWebEngineView` is indeed required for transparency.
- If `WebEngineView { backgroundColor: "transparent" }` works in QML without the C++ wrapper, we *could* drop the C++ wrapper and potentially switch to dynamic loading later.
- *Current Assumption*: It is required (based on `5-first-implementation-report.md`). We will proceed with the C++ wrapper.

### Step 3: Final Build & Test
1. **Clean Build (WebEngine ON)**:
   ```bash
   rm -rf build
   cmake -B build -DWEBENGINE=ON -DCMAKE_BUILD_TYPE=Debug
   cmake --build build -j$(nproc)
   ```
2. **Clean Build (WebEngine OFF)**:
   ```bash
   rm -rf build
   cmake -B build -DWEBENGINE=OFF -DCMAKE_BUILD_TYPE=Debug
   cmake --build build -j$(nproc)
   ```
3. **Manual Test**:
   Run the test case with the WebEngine-enabled build:
   ```bash
   ./build/src/quickshell -c src/webengine/test/manual/test.qml
   ```
   - Verify transparency.
   - Verify WebChannel communication.

---

## 3. Next Steps (Execution)

1.  **Apply `qArgc` fix**.
2.  **Run verification builds**.
3.  **Update PR Description** (in `7-pr1-wip.md`) to acknowledge the build option choice.
4.  **Submit PR**.
