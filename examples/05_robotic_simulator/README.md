# 05 — Robotic Simulator

Multi-controller runtime switching with `Inspect3DController` and `Navigation3DController` on a shared camera.

## Use case

Robotic simulation, game engines, editors — any application that needs multiple camera modes (inspect, navigate) selectable at runtime without recreating the camera or reloading the scene.

## What it covers

### Setup — two controllers on one camera
- `Inspect3DController` and `Navigation3DController` share the same `ICamera`
- Each calls `setCamera(camera)` and `onResize(w, h)` — no camera duplication

### A — Inspect mode (orbit robot arm)
- Fixed pivot at robot base: `setPivot` + `setPivotMode(eFixed)`
- `setRotationInertiaEnabled(true)` + `setRotationDamping`
- LMB orbit drag, `fitToAABB` for robot arm bounding box

### B — Switch to Navigate (Tab-key pattern)
- `inspect.reset()` — clears orbit inertia velocity before handing off
- `navigate.reset()` — syncs FreeLookManipulator yaw/pitch from current camera pose
- FPS walk: RMB look + W forward

### C — Inspect with moving pivot (effector path)
- `navigate.reset()` + `inspect.reset()` before resuming inspect
- Each frame: `inspect.setPivot(simulatedEndEffector(t))` then `onUpdate` — orbit around a moving point without a dedicated follow controller

### D — Orbit rotation damping
- High vs low `orbitalCameraManipulator().setRotationDamping` after LMB drags — snappy vs floaty inertia decay

### E — Static pivot (robot base)
- `setPivot` at a fixed world point and run updates — stable orbit centre

### F — More inspect orbit
- `inspect.reset()` then another LMB drag — confirms orbit after mode switches

## Reset-on-switch pattern

```cpp
// Always reset() the outgoing controller before activating the next one.
// This clears stale drag velocities, held-key flags, and intermediate state
// that would otherwise carry over and cause phantom motion.
inspect.reset();
navigate.reset();
// Now safe to route events to navigate
```
