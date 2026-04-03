<p align="center">
  <img src="icons/vertexnova_logo_medallion_with_text.svg" alt="VertexNova Interaction" width="320"/>
</p>

<p align="center">
  <strong>Camera interaction library (manipulators, orbit/trackball behaviors, and controllers) for the VertexNova ecosystem</strong>
</p>

<p align="center">
  <a href="https://github.com/vertexnova/vneinteraction/actions/workflows/ci.yml">
    <img src="https://github.com/vertexnova/vneinteraction/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI"/>
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++ Standard"/>
  <a href="https://codecov.io/gh/vertexnova/vneinteraction">
    <img src="https://codecov.io/gh/vertexnova/vneinteraction/branch/main/graph/badge.svg" alt="Coverage"/>
  </a>
  <img src="https://img.shields.io/badge/license-Apache%202.0-green.svg" alt="License"/>
</p>

---

## About

**VneInteraction** is a C++20 library that turns input into camera motion for the [VertexNova](https://github.com/vertexnova) stack. It does **not** provide rendering, windowing, or a platform event loop — your app feeds events (for example from [vneevents](https://github.com/vertexnova/vneevents)), and manipulators update [vnescene](https://github.com/vertexnova/vnescene) cameras via `vne::scene::ICamera`.

It sits above **vnescene** / **vnemath** (cameras and math) and **vneevents** (event types). **vnelogging** is optional; examples use it the same way as other VertexNova repos (see [vnelogging README](https://github.com/vertexnova/vnelogging) when this repo is checked out with submodules).

## Features

- **Manipulators**: `OrbitalCameraManipulator`, `FreeLookManipulator`, `Ortho2DManipulator`, `FollowManipulator` — each implements `ICameraManipulator`; orbit math is composed from `OrbitBehavior` / `TrackballBehavior` (Euler and virtual trackball).
- **Controllers**: `Inspect3DController`, `Navigation3DController`, `Ortho2DController`, `FollowController` — combine `InputMapper`, `CameraRig`, and manipulators with `onEvent` / `onUpdate`.
- **InputMapper**: Presets (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`) and custom `InputRule` rows mapping to `CameraActionType` / `CameraCommandPayload`.
- **CameraRig**: Multicast lifecycle and actions across multiple manipulators.
- **Behavior**: Rotation modes, pivot modes, zoom methods, inertia, fit-to-AABB; types such as `ZoomMethod`, `OrbitPivotMode`, `FreeLookMode` live in `interaction_types.h`.
- **Use cases**: Medical 3D/2D inspection, game/editor cameras, robotic simulators.
- **Cross-platform**: Linux, macOS, Windows; mobile and Web follow vnescene / vnemath toolchains where those targets are enabled.

System, class, component, and runtime pipeline diagrams live in [Architecture & usage](docs/vertexnova/interaction/interaction.md) (sources under [`docs/vertexnova/interaction/diagrams/`](docs/vertexnova/interaction/diagrams/)).

## Installation

### Option 1: Git Submodule (Recommended)

```bash
git submodule add https://github.com/vertexnova/vneinteraction.git deps/vneinteraction
# Ensure vnescene, vneevents, and transitive deps are available (this repo uses deps/internal + vnecmake).
```

In your `CMakeLists.txt`:

```cmake
add_subdirectory(deps/vneinteraction)
target_link_libraries(your_target PRIVATE vne::interaction)
```

### Option 2: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    vneinteraction
    GIT_REPOSITORY https://github.com/vertexnova/vneinteraction.git
    GIT_TAG main
)
set(VNE_INTERACTION_EXAMPLES OFF)
FetchContent_MakeAvailable(vneinteraction)
target_link_libraries(your_target PRIVATE vne::interaction)
```

### Option 3: System Install

Installs headers and the library for the **`vneinteraction` CMake install component** only (internal deps use their own install rules if you install the full tree).

```bash
git clone --recursive https://github.com/vertexnova/vneinteraction.git
cd vneinteraction
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build --component vneinteraction
```

There is **no** shipped `find_package(VneInteraction)` config module yet. For CMake targets and transitive usage of `vne::interaction`, prefer **add_subdirectory** or **FetchContent**. After a plain install, link the installed library and add the install include directory manually, or point your project at the same dependency layout this repo uses under `deps/internal`.

## Building

```bash
git clone --recursive https://github.com/vertexnova/vneinteraction.git
cd vneinteraction
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

For local development (examples + tests enabled):

```bash
cmake -B build -DVNE_INTERACTION_DEV=ON
cmake --build build
```

Helper scripts (Linux, macOS, Windows): see [scripts/README.md](scripts/README.md).

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `VNE_INTERACTION_TESTS` | `ON` | Build the test suite |
| `VNE_INTERACTION_EXAMPLES` | `OFF` | Build example applications |
| `VNE_INTERACTION_DEV` | `ON` (top-level) | Dev preset: tests and examples ON |
| `VNE_INTERACTION_CI` | `OFF` | CI preset: tests ON, examples OFF |
| `VNE_INTERACTION_LIB_TYPE` | `shared` | Library type: `static` or `shared` |
| `ENABLE_DOXYGEN` | `OFF` | Build API documentation (Doxygen) |
| `ENABLE_COVERAGE` | `OFF` | Enable code coverage reporting |
| `ENABLE_ASAN` | `OFF` | AddressSanitizer + UBSan (GCC/Clang, Linux/macOS) |
| `WARNINGS_AS_ERRORS` | `OFF` | Treat warnings as errors |

## Library type

Default is **`shared`** (`VNE_INTERACTION_LIB_TYPE=shared`). Use **`static`** for a single binary with no separate `vneinteraction` shared object. Internal VertexNova dependencies are pulled in via `add_subdirectory` / `VneUseDep` when you build this repo as a tree.

## Quick Start

```cpp
#include <vertexnova/interaction/interaction.h>
#include <vertexnova/interaction/version.h>
#include <vertexnova/scene/camera/camera.h>

int main() {
    using namespace vne::interaction;
    using namespace vne::scene;

    const char* ver = get_version();  // e.g. "1.5.5"

    auto camera = CameraFactory::createPerspective(
        PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 5.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    Inspect3DController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(1280.0f, 720.0f);

    // Each frame: forward vne::events::Event from your windowing API, then:
    // ctrl.onEvent(event, dt);
    ctrl.onUpdate(1.0 / 60.0);

    return 0;
}
```

See [examples/01_library_info](examples/01_library_info) for version and preset listing, and [examples/README.md](examples/README.md) for the full numbered index.

## Examples (headless)

These programs use **simulated** `vne::events::Event` streams (no window or GPU). Each folder has `NN_example.h` / `NN_example.cpp` plus a thin `main.cpp`.

| Example | Description |
|---------|-------------|
| [01_library_info](examples/01_library_info) | Version; manipulator types, presets, and factory smoke tests |
| [02_medical_3d_inspect](examples/02_medical_3d_inspect) | `Inspect3DController` — orbit / trackball, pivots, zoom methods, `fitToAABB`, view presets |
| [03_medical_2d_slices](examples/03_medical_2d_slices) | `Ortho2DController` — pan, scroll zoom, rebinding, ortho tuning |
| [04_game_editor_camera](examples/04_game_editor_camera) | `Navigation3DController` — FPS / Fly, WASD + look, sprint/slow, rebinding |
| [05_robotic_simulator](examples/05_robotic_simulator) | Multi-controller switching: Inspect + Navigation + Follow |
| [06_custom_input_bindings](examples/06_custom_input_bindings) | `InputMapper` — presets, `bindGesture` / `bindScroll` / keys, direct `InputRule` workflow |
| [07_camera_rig_composition](examples/07_camera_rig_composition) | `CameraRig` — factories, hybrid stacks, hot-swap, `setEnabled`, zoom ownership |
| [08_camera_state_save_restore](examples/08_camera_state_save_restore) | Orbit / trackball / free-look state blobs and bookmark-style save/restore |

Build examples with:

```bash
cmake -B build -DVNE_INTERACTION_EXAMPLES=ON
cmake --build build
```

Run binaries from **`<your-cmake-binary-dir>/bin/examples/`** (for example `build/bin/examples/` after `cmake -B build`, or the same path under `build/<lib_type>/<config>/…` when using [scripts/README.md](scripts/README.md) platform scripts).

## Interactive demo (VertexNova Testbed)

For a **real GLFW + OpenGL** application with mouse, keyboard, ImGui tuning, and runtime switching across all controller types (inspect orbit/trackball, navigation FPS/Fly/Game, ortho 2D, follow), use the VertexNova **vnetestbed** sample:

**[vnetestbed — `samples/glfw_opengl/03_test_interaction`](https://github.com/vertexnova/vnetestbed/tree/main/samples/glfw_opengl/03_test_interaction)**

<p align="center">
  <a href="https://github.com/vertexnova/vnetestbed/tree/main/samples/glfw_opengl/03_test_interaction">
    <img src="docs/vertexnova/interaction/screenshots/interactive_demo_03_test_interaction.png" alt="vnetestbed 03_test_interaction: viewport with teapot on grid, RGB axes, and ImGui settings (zoom method, view directions, camera state, mesh browser)" width="900"/>
  </a>
</p>

<p align="center"><em>Screenshot — VertexNova Testbed <code>03_test_interaction</code>: viewport, interaction tuning, camera state, and mesh browser.</em></p>

That target links against `vne::interaction` and shares the same build/run workflow as the other testbed samples. Configure vnetestbed with samples enabled (for example `-DVNE_TESTBED_SAMPLES=ON` or `-DVNE_TESTBED_DEV=ON`). Details, optional mesh IO (`VNE_TESTBED_VNEIO`), and layer structure are in the sample [README](https://github.com/vertexnova/vnetestbed/blob/main/samples/glfw_opengl/03_test_interaction/README.md).

If you clone vnetestbed locally, the sources live at `samples/glfw_opengl/03_test_interaction/` (e.g. `demo_test_interaction.cpp`). For headless API exercises without a window, use the **[examples/](examples/README.md)** in this repo.

## Documentation

- [API documentation](docs/README.md) — Doxygen (`ENABLE_DOXYGEN`, target `vneinteraction_doc_doxygen`) and `scripts/generate-docs.sh`.
- [Architecture & usage](docs/vertexnova/interaction/interaction.md) — Module design, diagrams, integration.
- [Review & roadmap](docs/vertexnova/tasks_plan/review_and_roadmap.md), [third-person follow (future)](docs/vertexnova/tasks_plan/third_person_follow_camera_future.md) — Planning notes.
- [Coding guidelines](CODING_GUIDELINES.md) — Project conventions (aligned with other VertexNova libraries such as [vnelogging](https://github.com/vertexnova/vnelogging)).

## Platform Support

| Platform | Status | Notes |
|----------|--------|------|
| Linux | Supported | GCC 10+, Clang 10+ |
| macOS | Supported | Xcode 12+, Apple Clang |
| Windows | Supported | MSVC 2019+, MinGW |
| iOS / visionOS | Supported | Via vnescene / vnemath toolchain |
| Android / Web | Supported | Via vnescene / vnemath |

## Requirements

- **C++20**
- **CMake** 3.19+
- **Compiler**: GCC 10+, Clang 10+, MSVC 2019+
- **[vnescene](https://github.com/vertexnova/vnescene)** (required; brings vnemath)
- **[vneevents](https://github.com/vertexnova/vneevents)** (required; event types)
- **[vnelogging](https://github.com/vertexnova/vnelogging)** (optional; examples and diagnostics)
- **Google Test** (tests; vendored via this repo’s dependency setup)

## License

Apache License 2.0 — see [LICENSE](LICENSE) for details.

---

<p align="center">
  Part of the <a href="https://github.com/vertexnova">VertexNova</a> project
</p>
