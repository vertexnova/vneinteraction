# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
