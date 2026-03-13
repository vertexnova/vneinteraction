# vneinteraction Library Review & Roadmap

## Overview

This document reviews the vneinteraction camera manipulation library against modern game engines (Unity/Cinemachine, Unreal Engine, Godot, O3DE) and medical visualization toolkits (3D Slicer, VTK, MITK). It identifies missing features, potential code issues, and provides a prioritized roadmap for both game and medical visualization use cases.

> **Note (2026)**: The library has been refactored to a behavior-based architecture. Legacy manipulators (`ICameraManipulator`, `OrbitManipulator`, etc.) have been removed. The current design uses `ICameraBehavior` (OrbitArcballBehavior, FreeLookBehavior, OrthoPanZoomBehavior, FollowBehavior) and high-level controllers (InspectController, Navigation3DController, Ortho2DController, FollowController). Many items below remain relevant for future enhancements; some references to removed classes are historical.

---

## Current Strengths

- Clean interface hierarchy: `ICameraBehavior` -> `OrbitArcballBehavior`, `FreeLookBehavior`, `OrthoPanZoomBehavior`, `FollowBehavior`
- Command/action layer (`CameraInputAdapter` + `CameraActionType`) decouples input bindings from manipulation logic
- Both perspective and orthographic camera support across orbit-style behaviors
- Inertia with exponential decay on pan and rotation
- Cursor-tracking zoom (Blender-style dolly-to-cursor)
- Touch input support (pan + pinch)
- Multiple zoom methods (dolly, FOV, scene scale)
- Configurable rotation pivot modes (COI, ViewCenter, FixedWorld)
- Smooth animated `fitToAABB`
- Good test coverage (9 test files)
- `noexcept` throughout input paths

---

## Missing Features

### Camera Animation & Transition System

**Priority: HIGH (game + medical)**

Modern engines provide smooth animated transitions when switching between manipulators, view directions, or camera presets. Currently:
- `setViewDirection()` snaps instantly with no lerp/slerp
- Switching manipulators (`setManipulator()`) causes camera jumps
- No generic `animateTo(position, target, duration, easing)` API
- No keyframe or path animation support

**References:** Unity Cinemachine blends between virtual cameras. Unreal has camera rigs with blend times. VTK has `vtkCameraInterpolator`. Medical viewers need smooth fly-throughs for surgical planning.

### Camera Constraints & Limits

**Priority: HIGH (medical)**

- No configurable pitch limits on orbit manipulators (hardcoded to +/-89 degrees)
- No configurable distance limits on orbit (hardcoded min=0.01, max=1e6)
- No bounding box/volume constraints to keep camera within a region
- No collision avoidance (camera-to-geometry intersection)
- No configurable axis lock for medical anatomical planes (axial, sagittal, coronal)

**References:** Unity/Unreal expose min/max distance and pitch clamps plus keep-in-bounds volumes. 3D Slicer has strict planar camera constraints for MPR views. VTK has `ClippingRange` auto-adjustment.

### Multi-View / Linked Camera Support

**Priority: HIGH (medical)**

Medical visualization apps commonly show 4 linked views (axial/sagittal/coronal + 3D). The library lacks:
- Camera groups or linked cameras
- Synchronized zoom/pan across views
- Observer/event pattern for camera state changes (no callbacks or signals)
- Split-screen or multi-viewport coordination

**References:** VTK has `vtkRenderWindowInteractor` with event callbacks. 3D Slicer links slice views. Unreal has multi-viewport with shared camera state.

### Ray Picking / Screen-to-World Projection

**Priority: HIGH (game + medical)**

The library computes `getWorldUnitsPerPixel()` but has no:
- `screenToWorldRay(x_px, y_px)` returning a `Ray3f`
- `worldToScreen(Vec3f)` returning a `Vec2f`
- Cursor-under-point or pick-at-cursor utility
- Integration with the math library's existing `Ray3f` type

**References:** Every engine provides screen-to-world as a core camera utility. Medical viewers use it for annotation placement, measurement tools, and anatomy picking.

### Camera Undo/Redo

**Priority: MEDIUM (medical)**

- No snapshot/restore mechanism for camera state
- No history stack for undo/redo
- Medical apps need "return to previous view" after exploration

### Gamepad / XR Controller Input

**Priority: MEDIUM (game), HIGH (medical VR)**

- `CameraInputBindings` only covers mouse + keyboard
- No gamepad axis/button mapping (thumbstick orbit/pan, triggers for zoom)
- No 6DOF controller support (VR/AR headset tracking)
- No SpaceMouse / 3DConnexion device support (common in medical/CAD)

### Camera Shake / Effects

**Priority: MEDIUM (game)**

- No camera shake (explosion/impact feedback)
- No screen-space effects hooks (DOF focus distance from orbit distance)
- No motion blur velocity output

### Serialization / Presets

**Priority: MEDIUM (both)**

- No save/load camera state (position, orientation, manipulator settings)
- No camera presets ("anterior", "posterior", "lateral" are standard in medical)
- No bookmark system

### Clipping Plane Management

**Priority: HIGH (medical)**

- No near/far auto-adjustment based on scene bounds
- Medical volume rendering requires precise clipping plane control
- No integration with slab or thick-slice clipping

---

## Existing Code Issues

> The following issues referred to legacy manipulator code that has been removed. Equivalent behavior classes (e.g. `FollowBehavior`, `FreeLookBehavior`) have been audited; `getWorldUnitsPerPixel()` is implemented where applicable. Retained for historical reference.

### ~~`FollowManipulator::getWorldUnitsPerPixel()` returns 0~~ (resolved in FollowBehavior)

`follow_manipulator.cpp:131` returns `0.0f` unconditionally. Any downstream division by this value produces infinity or NaN.

**Fix:** Compute from offset distance and camera FOV/viewport, similar to `OrbitStyleBase::getWorldUnitsPerPixel()`.

### ~~`FpsManipulator::getWorldUnitsPerPixel()` returns 0~~ (resolved in FreeLookBehavior)

`fps_manipulator.cpp:62` returns `0.0f` unconditionally. Same issue as above.

**Fix:** Compute from FOV and a reference distance (1.0 unit ahead of camera).

### ~~`FlyManipulator::getWorldUnitsPerPixel()` returns 0~~ (resolved in FreeLookBehavior)

`fly_manipulator.cpp:62` returns `0.0f` unconditionally. Same issue as above.

**Fix:** Same approach as FPS manipulator.

### `OrbitArcballBehavior` Y-up handling

OrbitArcballBehavior and FreeLookBehavior use world-up for spherical coordinates. Verify Z-up (`setWorldUp`) works correctly across all modes.

**Fix:** Generalize yaw/pitch extraction by projecting onto the world-up axis and its perpendicular plane.

### `UpAxis` enum defined but unused

`interaction_types.h:58-61` defines `UpAxis::eY` and `UpAxis::eZ` but nothing references them.

**Fix:** Add a `toWorldUp(UpAxis)` helper function and wire it into manipulator construction or factory.

### Thread Safety

All classes are marked not thread-safe. In a modern engine where rendering and input happen on different threads, `shared_ptr<ICamera>` can be read in `update()` (render thread) and written in `handleMouse*()` (input thread) with no synchronization.

**Recommendation:** Add a frame-synced camera snapshot or command queue pattern for multi-threaded use.

### Input path

The new design uses a single path: events -> `InputMapper` -> `CameraActionType` -> behaviors. No dual-path concern.

### Behavior configuration

Behaviors are constructed with defaults; configuration is via setters (`setRotationMode`, `setPivotMode`, etc.). Consider builder or config-struct for complex setups.

### Unhandled actions

`ICameraBehavior::onAction` returns `bool`; behaviors silently ignore actions they don't handle. Consider logging or assertion for unknown `CameraActionType` in debug builds.

---

## Comparison Table

| Feature | vneinteraction | Unity/Cinemachine | Unreal | VTK/3D Slicer |
|---|---|---|---|---|
| Camera transitions | None | Blend between vcams | Camera rigs | vtkCameraInterpolator |
| Constraint system | Hardcoded limits | Configurable volumes | Spring arm + collision | ClippingRange auto |
| Event system | None | CinemachineEvents | Delegates | vtkCommand/Observer |
| Multi-view link | None | CinemachineBrain | PlayerController | Linked slice views |
| Undo/Redo | None | Via Unity Undo | Via Transaction | Via vtkUndoStack |
| Ray picking | None | Camera.ScreenPointToRay | DeprojectScreenToWorld | vtkPicker |
| Gamepad/XR | None | InputSystem integration | Enhanced Input | vtkRenderWindowInteractor |
| Serialization | None | Scriptable profiles | SaveGame | XML state |
| Up-axis config | Enum exists, unused | Project settings | Project settings | Global coordinate system |
| Slab/clip planes | None | N/A | N/A | vtkImageReslice |
| Auto near/far | None | CinemachineConfiner | Auto adjust | ResetCameraClippingRange |

---

## Recommended Roadmap

### For Game Engine Use

| Priority | Feature | Effort |
|----------|---------|--------|
| 1 | Camera transition/blend system | Medium |
| 2 | Screen-to-world ray | Low |
| 3 | Event/callback system (camera changed notifications) | Low |
| 4 | Gamepad input (extend CameraInputBindings with axis mappings) | Medium |
| 5 | Thread-safe camera snapshot for render/input thread separation | Medium |
| 6 | Camera shake/effects | Low |
| 7 | Serialization/presets | Low |

### For Medical Visualization

| Priority | Feature | Effort |
|----------|---------|--------|
| 1 | Multi-view linked cameras (synchronized axial/sagittal/coronal + 3D) | High |
| 2 | Axis-constrained rotation (lock to anatomical planes) | Medium |
| 3 | Screen-to-world ray + picking (measurement and annotation tools) | Low |
| 4 | Camera presets (standard anatomical views with smooth transitions) | Medium |
| 5 | Undo/redo camera state (snapshot and restore) | Low |
| 6 | Near/far auto-adjustment (prevent clipping through anatomy) | Low |
| 7 | Clipping plane / slab management | Medium |

### Quick Wins (Low Effort, High Value)

1. Fix `getWorldUnitsPerPixel()` in FollowManipulator, FpsManipulator, FlyManipulator
2. Wire up the `UpAxis` enum to manipulator construction
3. Fix Y-up assumption in OrbitManipulator and FreeCameraBase
4. Add `screenToWorldRay()` utility (math lib already has `Ray3f`)
5. Add camera state snapshot struct with save/restore methods
6. Add `onCameraChanged` callback (`std::function` on `CameraSystemController`)
