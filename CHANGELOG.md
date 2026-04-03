# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.6.0](https://github.com/vertexnova/vneinteraction/compare/v1.5.5...v1.6.0) (2026-04-03)


### Features

* updated api, architecture, documents, updated examples, improved tests and refactored code ([#35](https://github.com/vertexnova/vneinteraction/issues/35)) ([49259a1](https://github.com/vertexnova/vneinteraction/commit/49259a1f07f613cc196a9ea38544163779491019))

## [1.5.5](https://github.com/vertexnova/vneinteraction/compare/v1.5.4...v1.5.5) (2026-04-01)


### Bug Fixes

* Optimize camera behavior synchronization and improve code clarity ([#32](https://github.com/vertexnova/vneinteraction/issues/32)) ([e344f14](https://github.com/vertexnova/vneinteraction/commit/e344f140fcfed51b93ec05977e17422b1f6ee64f))

## [1.5.4](https://github.com/vertexnova/vneinteraction/compare/v1.5.3...v1.5.4) (2026-03-31)


### Bug Fixes

* Fixing navigation 3d controller direction issue. ([#30](https://github.com/vertexnova/vneinteraction/issues/30)) ([a2969df](https://github.com/vertexnova/vneinteraction/commit/a2969df3216888ce13d22b4a141895f609de9ae7))

## [Unreleased]

### Removed

* **`Navigation3DController::freeLookBehavior()`** — use **`freeLookManipulator()`** only (temporary compatibility shim removed).

### Breaking

* **Manipulator naming (Trackball / Orbit stay `*Behavior`)**: Composable camera types are renamed from `*Behavior` to `*Manipulator`; `ICameraBehavior` → `ICameraManipulator`; `CameraBehaviorBase` → `CameraManipulatorBase`. Headers: `camera_manipulator.h`, `camera_manipulator_base.h`, `orbital_camera_manipulator.h`, `free_look_manipulator.h`, `ortho_2d_manipulator.h`, `follow_manipulator.h`. Internal helpers: `manipulator_utils.*` (was `behavior_utils.*`). **`TrackballBehavior`** / **`OrbitBehavior`** and headers `trackball_behavior.h` / `orbit_behavior.h` are unchanged.
* **`CameraRig`**: `addBehavior` / `removeBehavior` / `clearBehaviors` / `behaviors()` → `addManipulator` / `removeManipulator` / `clearManipulators` / `manipulators()`.
* **Controllers**: `Inspect3DController::orbitalCameraBehavior()` → `orbitalCameraManipulator()`; `Navigation3DController::freeLookBehavior()` → `freeLookManipulator()`; `orbitalCameraBehavior()` → `orbitalCameraManipulator()`; `Ortho2DController::ortho2DBehavior()` → `ortho2DManipulator()`; `FollowController::followBehavior()` → `followManipulator()`.

### Changed

* **`Inspect3DController`**: LMB orbit rotation **on** by default (`OrbitalRotationMode::eOrbit`); use `setRotationEnabled(false)` to disable. Matches examples that simulate LMB drag without calling `setRotationEnabled(true)`.

### Removed

* **`CameraRig::makeOrthoPanZoom()`** — use **`CameraRig::makeOrtho2D()`** instead.
* **`CameraRig::makeGameCamera()`** — compose **`OrbitalCameraManipulator`** + **`FreeLookManipulator`** on **`CameraRig`** (see `camera_rig.h` usage example), or use **`Inspect3DController`** / **`Navigation3DController`** as appropriate.

### Changed

* **Breaking**: Renamed `InspectController` to `Inspect3DController`; header `inspect_controller.h` → `inspect_3d_controller.h`.
* **Breaking**: Renamed `OrbitArcballBehavior` to `OrbitalCameraBehavior`; header `orbit_arcball_behavior.h` → `orbital_camera_behavior.h`; accessors `orbitArcballBehavior()` → `orbitalCameraBehavior()` on `Inspect3DController` and `Navigation3DController`.
* **Breaking**: `OrbitalRotationMode::eArcball` renamed to `eTrackball`; `CameraRig::makeArcball()` → `makeTrackball()`.
* **Breaking**: New behavior-based architecture. Removed legacy manipulators (`OrbitManipulator`, `ArcballManipulator`, `FpsManipulator`, `FlyManipulator`, `OrthoPanZoomManipulator`, `FollowManipulator`), `CameraManipulatorFactory`, and `CameraSystemController`.
* **Added**: Behaviors — `OrbitalCameraBehavior`, `FreeLookBehavior`, `OrthoPanZoomBehavior`, `FollowBehavior` (implement `ICameraBehavior`).
* **Added**: Controllers — `Inspect3DController`, `Navigation3DController`, `Ortho2DController`, `FollowController`.
* **Added**: `InputMapper` with presets (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`).
* **Added**: `CameraRig` — multi-behavior container with factory methods.
* **Changed**: Event-based API — `onEvent(event, delta_time)` and `onUpdate(delta_time)` replace `handleMouse*`/`handleKeyboard`/`update`.
* **Changed**: `PlanarController` renamed to `Ortho2DController`, `NavigateController` to `Navigation3DController`.

## [1.5.3](https://github.com/vertexnova/vneinteraction/compare/v1.5.2...v1.5.3) (2026-03-30)

### Breaking Changes

* **Navigation**: Remove `NavigateMode::eGame`. `Navigation3DController` supports only **FPS** and **Fly** (`FreeLookBehavior` + `fpsPreset`). For orbit + WASD hybrid rigs, compose **`OrbitalCameraBehavior`** + **`FreeLookBehavior`** on **`CameraRig`** manually; use **`Inspect3DController`** for orbit-style inspection.
* **`Navigation3DController::orbitalCameraBehavior()`** always returns **`nullptr`**.
* **Inspect3DController**: When LMB orbit is enabled, default algorithm is **Euler orbit** (`OrbitalRotationMode::eOrbit`). Previously the default was **trackball** when rotation was enabled.

### Bug Fixes

* Fixing the rotational direction issue. The rotation is in opposite to intended direction. ([#28](https://github.com/vertexnova/vneinteraction/issues/28)) ([0eae905](https://github.com/vertexnova/vneinteraction/commit/0eae90591e26bc7f63575afd92410b53b7f763ad))

## [1.5.2](https://github.com/vertexnova/vneinteraction/compare/v1.5.1...v1.5.2) (2026-03-30)


### Bug Fixes

* Update maximum field of view to enhance camera behavior ([#26](https://github.com/vertexnova/vneinteraction/issues/26)) ([fb607b9](https://github.com/vertexnova/vneinteraction/commit/fb607b92a3dd0e5d08e2024a61aaa17f0f65a502))

## [1.5.1](https://github.com/vertexnova/vneinteraction/compare/v1.5.0...v1.5.1) (2026-03-29)


### Bug Fixes

* Fixing and refactoring the zoom implementation ([#24](https://github.com/vertexnova/vneinteraction/issues/24)) ([8c6848a](https://github.com/vertexnova/vneinteraction/commit/8c6848a14070de36d279f6974d4e47568c2e41fd))

## [1.5.0](https://github.com/vertexnova/vneinteraction/compare/v1.4.1...v1.5.0) (2026-03-28)


### Features

* Adding rotation for ortho 2d and fixing pan direction issue.  ([#22](https://github.com/vertexnova/vneinteraction/issues/22)) ([f776506](https://github.com/vertexnova/vneinteraction/commit/f776506048a328abdd50d8d99396ab5154be805a))

## [1.4.1](https://github.com/vertexnova/vneinteraction/compare/v1.4.0...v1.4.1) (2026-03-28)


### Bug Fixes

* Fixed architecture for orbital camera and panning. ([#20](https://github.com/vertexnova/vneinteraction/issues/20)) ([32a662b](https://github.com/vertexnova/vneinteraction/commit/32a662b406f026615fa1056a5cbe42f2eb336828))
* Orthographic pan: vertical drag direction now matches horizontal “drag the scene” behavior; orbital pan uses the same fix for perspective and orthographic modes.

### Features

* `Ortho2DBehavior` (renamed from `OrthoPanZoomBehavior`): in-plane slice rotation via `eBeginRotate` / `eRotateDelta` / `eEndRotate`, with `setRotationSensitivityDegreesPerPixel` (default 0.2°/px, aligned with orbit feel).
* `CameraRig::makeOrtho2D()` replaces the primary factory name; `makeOrthoPanZoom()` was a deprecated alias (removed; see [Unreleased]).
* `Ortho2DController::ortho2DBehavior()` replaces `orthoPanZoomBehavior()`.

### Breaking Changes

* Rename: `OrthoPanZoomBehavior` → `Ortho2DBehavior`, headers `ortho_pan_zoom_behavior.*` → `ortho_2d_behavior.*`, `Ortho2DController::orthoPanZoomBehavior()` → `ortho2DBehavior()`.

## [1.4.0](https://github.com/vertexnova/vneinteraction/compare/v1.3.3...v1.4.0) (2026-03-28)


### Features

* Introduce EulerOrbit class for yaw/pitch camera control ([#18](https://github.com/vertexnova/vneinteraction/issues/18)) ([ddf0746](https://github.com/vertexnova/vneinteraction/commit/ddf0746f72f3ad18530d111465c3c79d23c8fa8d))

## [1.3.3](https://github.com/vertexnova/vneinteraction/compare/v1.3.2...v1.3.3) (2026-03-28)


### Bug Fixes

* Fix the trackball camera rotation issue that was opposite direction; Also fix rotation speed factor and some refactoring ([#16](https://github.com/vertexnova/vneinteraction/issues/16)) ([e906f9e](https://github.com/vertexnova/vneinteraction/commit/e906f9ed897039cdd416b060b37ab6cbd249b680))

## [1.3.2](https://github.com/vertexnova/vneinteraction/compare/v1.3.1...v1.3.2) (2026-03-28)


### Bug Fixes

* fixing the math for trackball rotation. ([#14](https://github.com/vertexnova/vneinteraction/issues/14)) ([113dfa9](https://github.com/vertexnova/vneinteraction/commit/113dfa95f2a0331d4e572c08e0b00e5f01e0ae2d))

## [1.3.1](https://github.com/vertexnova/vneinteraction/compare/v1.3.0...v1.3.1) (2026-03-24)


### Bug Fixes

* Fixing the trackball and zoom bugs ([#9](https://github.com/vertexnova/vneinteraction/issues/9)) ([dae4009](https://github.com/vertexnova/vneinteraction/commit/dae4009ec47a7d9eba0aebc7e8df22bf19693b61))

## [1.3.0](https://github.com/vertexnova/vneinteraction/compare/v1.2.0...v1.3.0) (2026-03-24)


### Features

* Enhance camera behaviors with zoom functionality and refactor methods ([#10](https://github.com/vertexnova/vneinteraction/issues/10)) ([41d0001](https://github.com/vertexnova/vneinteraction/commit/41d0001eaf710730ab8253dca8e288f4732cfe61))

## [1.2.0](https://github.com/vertexnova/vneinteraction/compare/v1.1.1...v1.2.0) (2026-03-13)


### Features

* major refactoring camera intraction changes ([#6](https://github.com/vertexnova/vneinteraction/issues/6)) ([8548b74](https://github.com/vertexnova/vneinteraction/commit/8548b74e317d59691577479d3040601e4c7a3dda))

## [1.1.1](https://github.com/vertexnova/vneinteraction/compare/v1.1.0...v1.1.1) (2026-03-11)


### Bug Fixes

* Fixing pan issues ([#4](https://github.com/vertexnova/vneinteraction/issues/4)) ([2237b5e](https://github.com/vertexnova/vneinteraction/commit/2237b5e1a5c6a6418a566725f6b11b42e64deb0a))

## [1.1.0](https://github.com/vertexnova/vneinteraction/compare/v1.0.0...v1.1.0) (2026-03-10)


### Features

* vne intreaction implementation ([#1](https://github.com/vertexnova/vneinteraction/issues/1)) ([38c118a](https://github.com/vertexnova/vneinteraction/commit/38c118a6b603e55e03eab101acbd93d7b717d439))

## [1.0.0] - 2026-03-08

### Added

- Initial release of VneInteraction: camera manipulators (Orbit, Trackball, FPS, Fly, OrthoPanZoom, Follow), minimal camera interfaces, factory, and system controller.
- Integration with vnemath for vectors, quaternions, and angle utilities.
- Shared library export/import (VNE_INTERACTION_API) for Windows DLL builds.
