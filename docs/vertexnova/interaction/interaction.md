# Interaction Module

## Overview

The Interaction module provides composable camera behaviors and high-level controllers for camera interaction. It is designed for use with [vnescene](https://github.com/vertexnova/vnescene) cameras and [vneevents](https://github.com/vertexnova/vneevents) for event types. It does not implement windowing or rendering.

**Key characteristics:**

- **Event-based**: Controllers consume `vne::events::Event` objects (mouse, keyboard, touch) and expose `onEvent(event, delta_time)` and `onUpdate(delta_time)`.
- **Behaviors**: Each behavior implements `ICameraBehavior`, holds a `std::shared_ptr<vne::scene::ICamera>`, and processes `CameraActionType` actions (orbit, pan, zoom, move, look, etc.).
- **Controllers**: High-level wrappers (`Inspect3DController`, `Navigation3DController`, `Ortho2DController`, `FollowController`) combine behaviors with `InputMapper` and event handling.
- **Composable**: `CameraRig` holds multiple behaviors; each receives every action independently. Enables hybrid setups (e.g. orbit + free-look for game editors).

## Architecture

- **Public API** (`include/vertexnova/interaction/`): `camera_behavior.h`, `camera_rig.h`, `orbital_camera_behavior.h`, `free_look_behavior.h`, `ortho_2d_behavior.h`, `follow_behavior.h`, `input_mapper.h`, `inspect_3d_controller.h`, `navigation_3d_controller.h`, `ortho_2d_controller.h`, `follow_controller.h`, `interaction_types.h`, `interaction.h` (umbrella), `version.h`.
- **Implementation** (`src/vertexnova/interaction/`): One `.cpp` per behavior and controller plus `input_mapper.cpp`, `camera_rig.cpp`, and `version.cpp`.
- **Dependencies**: **vnescene** (cameras, `ICamera`, `PerspectiveCamera`, `OrthographicCamera`, `CameraFactory`), **vneevents** (event types), **vnemath** (vectors, quaternions, matrices; pulled in via vnescene).

## Key Components

### Behaviors (ICameraBehavior)

- **camera_behavior.h**: `ICameraBehavior` — interface: `onAction`, `onUpdate`, `setCamera`, `onResize`, `resetState`, `isEnabled`, `setEnabled`.
- **orbital_camera_behavior.h**: `OrbitalCameraBehavior` — orbit around a center of interest; Euler or Quaternion rotation; pivot modes (COI, ViewCenter, Fixed); pan, zoom, inertia, fitToAABB.
- **free_look_behavior.h**: `FreeLookBehavior` — FPS or Fly mode; WASD movement, mouse look, sprint/slow modifiers.
- **ortho_2d_behavior.h**: `Ortho2DBehavior` — orthographic camera only; pan, zoom-to-cursor, optional in-plane rotation; inertia.
- **follow_behavior.h**: `FollowBehavior` — follow a target (static or from provider); smooth damping; offset.

### Controllers

- **inspect_3d_controller.h**: `Inspect3DController` — 3D inspection (medical, CAD); wraps `OrbitalCameraBehavior` + `InputMapper` with orbit preset; pivot, DOF toggles, fitToAABB.
- **navigation_3d_controller.h**: `Navigation3DController` — 3D environment traversal; FPS or Fly mode; wraps `FreeLookBehavior` + `InputMapper` (`fpsPreset`).
- **ortho_2d_controller.h**: `Ortho2DController` — 2D orthographic viewports (slices, maps); wraps `Ortho2DBehavior` + `InputMapper` with ortho preset.
- **follow_controller.h**: `FollowController` — target-follow camera; wraps `FollowBehavior`; no user input required.

### Input and Rig

- **input_mapper.h**: `InputMapper` — maps mouse/keyboard/touch events to `CameraActionType` via `InputRule`; presets: `orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`.
- **camera_rig.h**: `CameraRig` — multi-behavior container; `onAction`, `onUpdate`, `setCamera`, `onResize`, `resetState`; factory methods: `makeOrbit`, `makeTrackball`, `makeFps`, `makeFly`, `makeOrtho2D`, `makeFollow`, `make2D`.

### Types

- **interaction_types.h**: `CameraActionType`, `CameraCommandPayload`, `InputRule`, `ZoomMethod`, `OrbitPivotMode`, `OrbitRotationMode`, `NavigateMode`, `UpAxis`, `MouseButton`, `TouchPan`, `TouchPinch`.

### Version

- **version.h**: `get_version()` — returns project version string (e.g. `"1.0.0"`).

## Usage

### Minimal: Inspect3DController + camera

```cpp
#include <vertexnova/interaction/interaction.h>
#include <vertexnova/scene/camera/camera.h>
#include <vertexnova/events/mouse_event.h>

int main() {
    using namespace vne::interaction;
    using namespace vne::scene;

    auto camera = CameraFactory::createPerspective(
        PerspectiveCameraParameters(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 5.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    Inspect3DController ctrl;
    ctrl.setCamera(camera);
    ctrl.onResize(1280.0f, 720.0f);

    // Game loop: feed events then update
    vne::events::MouseMovedEvent move(640.0, 360.0);
    ctrl.onEvent(move, 0.016);
    vne::events::MouseScrolledEvent scroll(0.0, 1.0);
    ctrl.onEvent(scroll, 0.016);
    ctrl.onUpdate(0.016);

    return 0;
}
```

### Bridging from vneevents

If you use [vneevents](https://github.com/vertexnova/vneevents), register a listener that forwards events to the controller:

```cpp
// Pseudocode: on any Event -> controller.onEvent(event, dt);
// Each frame: controller.onUpdate(dt);
```

The controller's `onEvent` handles `MouseMovedEvent`, `MouseButtonPressedEvent`, `MouseButtonReleasedEvent`, `MouseScrolledEvent`, `KeyPressedEvent`, `KeyReleasedEvent`, etc.

### Use-case mapping

| Use case | Controller | Notes |
|----------|------------|-------|
| Medical 3D inspection | `Inspect3DController` | Euler orbit default when rotation enabled; `setRotationMode(eTrackball)` optional; `setPivotMode(eFixed)` for landmark-centred |
| Medical 2D slices | `Ortho2DController` | Pan + zoom; optional in-plane rotate via `setRotationEnabled(true)` |
| Game / editor camera | `Navigation3DController` + `Inspect3DController` | FPS/Fly for world nav; `Inspect3DController` for orbit inspection |
| Robotic simulator | `Inspect3DController` + `Navigation3DController` + `FollowController` | Inspect robot; navigate environment; follow end-effector |

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
