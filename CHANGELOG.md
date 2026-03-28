# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0](https://github.com/vertexnova/vneinteraction/compare/v1.3.3...v1.4.0) (2026-03-28)


### Features

* Introduce EulerOrbit class for yaw/pitch camera control ([#18](https://github.com/vertexnova/vneinteraction/issues/18)) ([ddf0746](https://github.com/vertexnova/vneinteraction/commit/ddf0746f72f3ad18530d111465c3c79d23c8fa8d))

## [1.3.3](https://github.com/vertexnova/vneinteraction/compare/v1.3.2...v1.3.3) (2026-03-28)


### Bug Fixes

* Fix the arcball camera rotation issue that was oposite direction; ALso fix rotation speed factor and some refactoring ([#16](https://github.com/vertexnova/vneinteraction/issues/16)) ([e906f9e](https://github.com/vertexnova/vneinteraction/commit/e906f9ed897039cdd416b060b37ab6cbd249b680))

## [1.3.2](https://github.com/vertexnova/vneinteraction/compare/v1.3.1...v1.3.2) (2026-03-28)


### Bug Fixes

* fixing the math for arcball rotation. ([#14](https://github.com/vertexnova/vneinteraction/issues/14)) ([113dfa9](https://github.com/vertexnova/vneinteraction/commit/113dfa95f2a0331d4e572c08e0b00e5f01e0ae2d))

## [1.3.1](https://github.com/vertexnova/vneinteraction/compare/v1.3.0...v1.3.1) (2026-03-24)


### Bug Fixes

* Fixing the arcball and zoom bugs ([#9](https://github.com/vertexnova/vneinteraction/issues/9)) ([dae4009](https://github.com/vertexnova/vneinteraction/commit/dae4009ec47a7d9eba0aebc7e8df22bf19693b61))

## [1.3.0](https://github.com/vertexnova/vneinteraction/compare/v1.2.0...v1.3.0) (2026-03-24)


### Features

* Enhance camera behaviors with zoom functionality and refactor methods ([#10](https://github.com/vertexnova/vneinteraction/issues/10)) ([41d0001](https://github.com/vertexnova/vneinteraction/commit/41d0001eaf710730ab8253dca8e288f4732cfe61))

## [1.2.0](https://github.com/vertexnova/vneinteraction/compare/v1.1.1...v1.2.0) (2026-03-13)


### Features

* major refactoring camera intraction changes ([#6](https://github.com/vertexnova/vneinteraction/issues/6)) ([8548b74](https://github.com/vertexnova/vneinteraction/commit/8548b74e317d59691577479d3040601e4c7a3dda))

## [Unreleased]

### Changed

- **Breaking**: New behavior-based architecture. Removed legacy manipulators (`OrbitManipulator`, `ArcballManipulator`, `FpsManipulator`, `FlyManipulator`, `OrthoPanZoomManipulator`, `FollowManipulator`), `CameraManipulatorFactory`, and `CameraSystemController`.
- **Added**: Behaviors — `OrbitArcballBehavior`, `FreeLookBehavior`, `OrthoPanZoomBehavior`, `FollowBehavior` (implement `ICameraBehavior`).
- **Added**: Controllers — `InspectController`, `Navigation3DController`, `Ortho2DController`, `FollowController`.
- **Added**: `InputMapper` with presets (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`).
- **Added**: `CameraRig` — multi-behavior container with factory methods.
- **Changed**: Event-based API — `onEvent(event, delta_time)` and `onUpdate(delta_time)` replace `handleMouse*`/`handleKeyboard`/`update`.
- **Changed**: `PlanarController` renamed to `Ortho2DController`, `NavigateController` to `Navigation3DController`.

## [1.1.1](https://github.com/vertexnova/vneinteraction/compare/v1.1.0...v1.1.1) (2026-03-11)


### Bug Fixes

* Fixing pan issues ([#4](https://github.com/vertexnova/vneinteraction/issues/4)) ([2237b5e](https://github.com/vertexnova/vneinteraction/commit/2237b5e1a5c6a6418a566725f6b11b42e64deb0a))

## [1.1.0](https://github.com/vertexnova/vneinteraction/compare/v1.0.0...v1.1.0) (2026-03-10)


### Features

* vne intreaction implementation ([#1](https://github.com/vertexnova/vneinteraction/issues/1)) ([38c118a](https://github.com/vertexnova/vneinteraction/commit/38c118a6b603e55e03eab101acbd93d7b717d439))

## [1.0.0] - 2026-03-08

### Added

- Initial release of VneInteraction: camera manipulators (Orbit, Arcball, FPS, Fly, OrthoPanZoom, Follow), minimal camera interfaces, factory, and system controller.
- Integration with vnemath for vectors, quaternions, and angle utilities.
- Shared library export/import (VNE_INTERACTION_API) for Windows DLL builds.
