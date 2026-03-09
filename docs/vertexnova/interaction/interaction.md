# Interaction Module

## Overview

The Interaction module provides camera manipulators and a thin controller that forwards input to the active manipulator. It is designed for use with [vnescene](https://github.com/vertexnova/vnescene) cameras and does not implement windowing, event systems, or rendering.

**Key characteristics:**

- **Input-agnostic**: Your application obtains input (mouse, keyboard, touch) from any source (GLFW, SDL, vneevents, native APIs) and calls the controller’s `handle*` methods with raw values. No dependency on a specific event library.
- **Manipulators**: Each manipulator implements `ICameraManipulator`, holds a `std::shared_ptr<vne::scene::ICamera>`, and updates the camera from input (orbit, arcball, FPS, fly, ortho pan/zoom, follow).
- **Single controller**: `CameraSystemController` holds one manipulator and forwards all input to it; you can swap manipulators at runtime.

## Architecture

- **Public API** (`include/vertexnova/interaction/`): `camera_manipulator.h`, `camera_manipulator_factory.h`, `camera_system_controller.h`, `interaction_types.h`, `orbit_manipulator.h`, `arcball_manipulator.h`, `fps_manipulator.h`, `fly_manipulator.h`, `ortho_pan_zoom_manipulator.h`, `follow_manipulator.h`, `interaction.h` (umbrella), `version.h`.
- **Implementation** (`src/vertexnova/interaction/`): One `.cpp` per manipulator plus factory, controller, and version.
- **Dependencies**: **vnescene** (cameras, `ICamera`, `PerspectiveCamera`, `OrthographicCamera`, `CameraFactory`), **vnemath** (vectors, quaternions, matrices; pulled in via vnescene).

## Key Components

### Controller

- **camera_system_controller.h**: `CameraSystemController` — `setManipulator`, `getManipulator`, `setViewportSize`, `update(delta_time)`, and input forwarding: `handleMouseMove`, `handleMouseButton`, `handleMouseScroll`, `handleKeyboard`, `handleTouchPan`, `handleTouchPinch`.

### Manipulators

- **camera_manipulator.h**: `ICameraManipulator` — interface: `setCamera`, `setViewportSize`, `handleMouseMove`, `handleMouseButton`, `handleMouseScroll`, `handleKeyboard`, `handleTouchPan`, `handleTouchPinch`, `getSceneScale`, `resetState`, `fitToAabb`, and support queries (`supportsPerspective`, `supportsOrthographic`).
- **orbit_manipulator.h**: `OrbitManipulator` — orbit around a center of interest; rotation, pan, zoom (dolly / scene scale / FOV).
- **arcball_manipulator.h**: `ArcballManipulator` — arcball rotation, pan, zoom.
- **fps_manipulator.h**: `FpsManipulator` — first-person style: yaw/pitch from mouse, move from keys.
- **fly_manipulator.h**: `FlyManipulator` — fly-through: move and look, sprint/slow modifiers.
- **ortho_pan_zoom_manipulator.h**: `OrthoPanZoomManipulator` — orthographic camera only; pan and zoom.
- **follow_manipulator.h**: `FollowManipulator` — follow a target (static or from provider), optional offset and zoom.

### Factory and Types

- **camera_manipulator_factory.h**: `CameraManipulatorFactory::create(CameraManipulatorType)` — creates `eOrbit`, `eArcball`, `eFps`, `eFly`, `eOrthoPanZoom`, `eFollow`.
- **interaction_types.h**: `CameraManipulatorType`, `CenterOfInterestSpace`, `ViewDirection`, `ZoomMethod`, `UpAxis`, `MouseButton`, `TouchPan`, `TouchPinch`.

### Version

- **version.h**: `get_version()` — returns project version string (e.g. `"1.0.0"`).

## Usage

### Minimal: controller + orbit + camera

```cpp
#include <vertexnova/interaction/interaction.h>
#include <vertexnova/scene/scene.h>

int main() {
    using namespace vne::interaction;
    using namespace vne::scene;
    using namespace vne::math;

    auto camera = CameraFactory::createPerspective(
        PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(Vec3f(0.0f, 2.0f, 5.0f));

    auto factory = std::make_shared<CameraManipulatorFactory>();
    auto orbit = factory->create(CameraManipulatorType::eOrbit);
    orbit->setCamera(camera);
    orbit->setViewportSize(1280.0f, 720.0f);

    CameraSystemController controller;
    controller.setManipulator(orbit);
    controller.setViewportSize(1280.0f, 720.0f);

    // Game loop: feed input then update
    controller.handleMouseMove(x, y, dx, dy, dt);
    controller.handleMouseButton(static_cast<int>(MouseButton::eLeft), true, x, y, dt);
    controller.handleMouseScroll(0.0f, scroll_delta, x, y, dt);
    controller.handleKeyboard(key_code, pressed, dt);
    controller.update(dt);
    return 0;
}
```

### Bridging from an event library

If you use [vneevents](https://github.com/vertexnova/vneevents) or similar, register a listener that maps events to controller calls:

```cpp
// Pseudocode: on MouseMovedEvent -> controller.handleMouseMove(e.x(), e.y(), e.dx(), e.dy(), dt);
//            on KeyPressedEvent  -> controller.handleKeyboard(e.keyCode(), true, dt);
//            on KeyReleasedEvent -> controller.handleKeyboard(e.keyCode(), false, dt);
// etc.
```

No dependency on vneevents is required; the controller only needs raw numbers.

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `VNE_INTERACTION_TESTS` | ON | Build unit tests |
| `VNE_INTERACTION_EXAMPLES` | OFF | Build examples |
| `VNE_INTERACTION_DEV` | ON (top-level) | Dev preset: tests + examples ON |
| `VNE_INTERACTION_CI` | OFF | CI preset: tests ON, examples OFF |
| `VNE_INTERACTION_LIB_TYPE` | shared | `static` or `shared` |
| `ENABLE_DOXYGEN` | OFF | Generate Doxygen API docs |

## Static vs shared

- **Static** (`-DVNE_INTERACTION_LIB_TYPE=static`): Single executable, no runtime lib to ship.
- **Shared** (default): For plugins or multiple apps sharing one lib. On Windows, use `VNE_INTERACTION_API` for DLL export/import.
