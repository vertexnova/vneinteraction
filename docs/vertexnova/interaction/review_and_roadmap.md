# vneinteraction Library Review & Roadmap

## Overview

This document reviews the vneinteraction camera manipulation library against modern game engines (Unity/Cinemachine, Unreal Engine, Godot, O3DE) and medical visualization toolkits (3D Slicer, VTK, MITK). It identifies missing features, potential code issues, and provides a prioritized roadmap for both game and medical visualization use cases.

---

## Current Strengths

- Clean interface hierarchy: `ICameraManipulator` -> `CameraManipulatorBase` -> `OrbitStyleBase`/`FreeCameraBase` -> concrete types
- Command/action layer (`CameraInputAdapter` + `CameraActionType`) decouples input bindings from manipulation logic
- Both perspective and orthographic camera support across orbit-style manipulators
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

### `FollowManipulator::getWorldUnitsPerPixel()` returns 0

`follow_manipulator.cpp:131` returns `0.0f` unconditionally. Any downstream division by this value produces infinity or NaN.

**Fix:** Compute from offset distance and camera FOV/viewport, similar to `OrbitStyleBase::getWorldUnitsPerPixel()`.

### `FpsManipulator::getWorldUnitsPerPixel()` returns 0

`fps_manipulator.cpp:62` returns `0.0f` unconditionally. Same issue as above.

**Fix:** Compute from FOV and a reference distance (1.0 unit ahead of camera).

### `FlyManipulator::getWorldUnitsPerPixel()` returns 0

`fly_manipulator.cpp:62` returns `0.0f` unconditionally. Same issue as above.

**Fix:** Same approach as FPS manipulator.

### `OrbitManipulator` assumes Y-up in `syncFromCamera()` and `computeFront()`

`orbit_manipulator.cpp:74` uses `front.y()` for pitch and `atan2(front.x(), -front.z())` for yaw, which only works when `world_up_ = (0,1,0)`. Setting Z-up via `setWorldUp()` produces incorrect angles.

**Fix:** Generalize yaw/pitch extraction by projecting onto the world-up axis and its perpendicular plane. Update both `syncFromCamera()` and `computeFront()` to work with any `world_up_` direction.

### `FreeCameraBase` assumes Y-up in `front()` and `syncAnglesFromCamera()`

`free_camera_base.cpp:48` hardcodes Y-up spherical coordinates. Same root cause as the orbit issue.

**Fix:** Use the virtual `upVector()` to generalize the spherical coordinate extraction.

### `UpAxis` enum defined but unused

`interaction_types.h:58-61` defines `UpAxis::eY` and `UpAxis::eZ` but nothing references them.

**Fix:** Add a `toWorldUp(UpAxis)` helper function and wire it into manipulator construction or factory.

### Thread Safety

All classes are marked not thread-safe. In a modern engine where rendering and input happen on different threads, `shared_ptr<ICamera>` can be read in `update()` (render thread) and written in `handleMouse*()` (input thread) with no synchronization.

**Recommendation:** Add a frame-synced camera snapshot or command queue pattern for multi-threaded use.

### Dual Input Paths

Both direct `handleMouse*()` methods and `applyCommand()` exist as public API. `CameraSystemController` routes through `CameraInputAdapter` -> `applyCommand()`, but direct methods are also callable. This creates two code paths that can diverge. Consider deprecating the direct methods or making them non-public.

### Factory lacks configuration

`CameraManipulatorFactory::create()` returns a default-configured manipulator with no way to pass initial parameters. Consider a builder or config-struct pattern.

### `CameraManipulatorBase::applyCommand()` is a silent no-op

The base implementation silently drops all commands. If a new `CameraActionType` is added and a subclass forgets to handle it, there is no warning.

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
