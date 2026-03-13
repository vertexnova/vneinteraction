<p align="center">
  <img src="icons/vertexnova_logo_medallion_with_text.svg" alt="VertexNova Interaction" width="320"/>
</p>

<p align="center">
  <strong>Camera interaction library (behaviors and controllers) for the VertexNova ecosystem</strong>
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

VneInteraction provides composable camera behaviors and high-level controllers for the [VertexNova](https://github.com/vertexnova) stack. It does not implement rendering, windowing, or an event system — it consumes events (e.g. from [vneevents](https://github.com/vertexnova/vneevents)) that your application feeds in, and drives cameras from [vnescene](https://github.com/vertexnova/vnescene).

VneInteraction is a C++20 library offering:

- **Behaviors**: `OrbitArcballBehavior`, `FreeLookBehavior`, `OrthoPanZoomBehavior`, `FollowBehavior` — each implements `ICameraBehavior` and works with `vne::scene::ICamera` (perspective or orthographic).
- **Controllers**: `InspectController`, `Navigation3DController`, `Ortho2DController`, `FollowController` — high-level wrappers that combine behaviors with `InputMapper` and event handling.
- **InputMapper**: Maps mouse, keyboard, and touch events to `CameraActionType` via configurable `InputRule` presets (orbit, FPS, game, CAD, ortho).
- **CameraRig**: Multi-behavior container for custom compositions.
- **Event-based**: Controllers expose `onEvent(event, delta_time)` and `onUpdate(delta_time)`; compatible with vneevents or any event source.

It depends on **vnescene** (and transitively **vnemath**) for cameras and math, and **vneevents** for event types. Tests use Google Test; examples optionally use **vnelogging**.

## Features

- **Behaviors**: `OrbitArcballBehavior`, `FreeLookBehavior`, `OrthoPanZoomBehavior`, `FollowBehavior` — rotation modes (Euler/Quaternion), pivot modes, zoom methods, inertia, fit-to-AABB.
- **Controllers**: `InspectController` (3D inspection), `Navigation3DController` (FPS/Fly/Game modes), `Ortho2DController` (2D slices, maps), `FollowController` (target following).
- **InputMapper**: Presets (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`) and custom `InputRule` configuration.
- **Types**: `CameraActionType`, `CameraCommandPayload`, `InputRule`, `ZoomMethod`, `OrbitPivotMode`, `NavigateMode`, etc. in `interaction_types.h`.
- **Use cases**: Medical 3D/2D inspection, game/editor cameras, robotic simulators.
- **Cross-platform**: Linux, macOS, Windows (and optionally iOS, Android, Web via vnemath).

## Installation

### Option 1: Git Submodule (Recommended)

```bash
git submodule add https://github.com/vertexnova/vneinteraction.git deps/vneinteraction
# Ensure vnescene, vneevents, and vnemath (and optionally vnelogging) are available as dependencies.
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

```bash
git clone --recursive https://github.com/vertexnova/vneinteraction.git
cd vneinteraction
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

In your `CMakeLists.txt` (ensure [vnescene](https://github.com/vertexnova/vnescene), [vneevents](https://github.com/vertexnova/vneevents), and [vnemath](https://github.com/vertexnova/vnemath) are installed first):

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_PREFIX_PATH}/lib/cmake/VneInteraction")
find_package(VneInteraction REQUIRED)
target_link_libraries(your_target PRIVATE vne::interaction)
```

Configure with `-DCMAKE_PREFIX_PATH=/usr/local` (or your install prefix) so the Find module is discovered.

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

## Quick Start

```cpp
#include <vertexnova/interaction/interaction.h>
#include <vertexnova/interaction/version.h>
#include <vertexnova/scene/camera/camera.h>
#include <vertexnova/events/mouse_event.h>
#include <memory>

int main() {
    using namespace vne::interaction;
    using namespace vne::scene;

    const char* ver = get_version();  // e.g. "1.0.0"

    auto camera = CameraFactory::createPerspective(
        PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 5.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    InspectController ctrl;
    ctrl.setCamera(camera);
    ctrl.setViewportSize(1280.0f, 720.0f);

    // Each frame: feed events from your window/event system, then:
    // ctrl.onEvent(mouse_event, dt);
    // ctrl.onEvent(scroll_event, dt);
    ctrl.onUpdate(1.0 / 60.0);

    return 0;
}
```

See [examples/01_library_info](examples/01_library_info) for version and behavior listing, and [examples/02_medical_3d_inspect](examples/02_medical_3d_inspect) for a minimal InspectController example.

## Examples

| Example | Description |
|---------|-------------|
| [01_library_info](examples/01_library_info) | Version info; list all behavior types and presets |
| [02_medical_3d_inspect](examples/02_medical_3d_inspect) | InspectController — arcball, landmark pivot, fitToAABB |
| [03_medical_2d_slices](examples/03_medical_2d_slices) | Ortho2DController — pan, scroll-zoom, ortho camera |
| [04_game_editor_camera](examples/04_game_editor_camera) | Navigation3DController — FPS + orbit modes, WASD |
| [05_robotic_simulator](examples/05_robotic_simulator) | InspectController + Navigation3DController + FollowController |

Build with `-DVNE_INTERACTION_EXAMPLES=ON` or use the dev preset (`-DVNE_INTERACTION_DEV=ON`). Run from `build/bin/examples/` (or `build/shared/Debug/bin/examples/` for Debug).

## Documentation

- [API Documentation](docs/README.md) — Generate with Doxygen (`-DENABLE_DOXYGEN=ON`, then `cmake --build build --target vneinteraction_doc_doxygen`).
- [Architecture & design](docs/vertexnova/interaction/interaction.md) — Module overview, components, and usage.

## Platform Support

| Platform | Status | Notes |
|----------|--------|------|
| Linux | Supported | GCC 10+, Clang 10+ |
| macOS | Supported | Xcode 12+, Apple Clang |
| Windows | Supported | MSVC 2019+, MinGW |
| iOS / visionOS | Supported | Via vnemath/vnescene toolchain |
| Android / Web | Supported | Via vnemath/vnescene |

## Requirements

- C++20
- CMake 3.19+
- [vnescene](https://github.com/vertexnova/vnescene) (required; brings vnemath)
- [vneevents](https://github.com/vertexnova/vneevents) (required; for event types)
- vnelogging (optional; for examples)
- Google Test (for tests; submodule or FetchContent)

## License

Apache License 2.0 — see [LICENSE](LICENSE) for details.

---

Part of the [VertexNova](https://github.com/vertexnova) project.
