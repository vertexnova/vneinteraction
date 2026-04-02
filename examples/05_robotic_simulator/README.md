# 05 — Robotic Simulator

Multi-controller runtime switching with all three primary controller types on a shared camera.

## Use case

Robotic simulation, game engines, editors — any application that needs multiple camera modes (inspect, navigate, follow) selectable at runtime without recreating the camera or reloading the scene.

## What it covers

### Setup — three controllers on one camera
- `Inspect3DController`, `Navigation3DController`, `FollowController` all share the same `ICamera`
- Each calls `setCamera(camera)` and `onResize(w, h)` — no camera duplication

### A — Inspect mode (orbit robot arm)
- Fixed pivot at robot base: `setPivot` + `setPivotMode(eFixed)`
- `setRotationInertiaEnabled(true)` + `setRotationDamping`
- LMB orbit drag, `fitToAABB` for robot arm bounding box

### B — Switch to Navigate (Tab-key pattern)
- `inspect.reset()` — clears orbit inertia velocity before handing off
- `navigate.reset()` — syncs FreeLookManipulator yaw/pitch from current camera pose
- FPS walk: RMB look + W forward

### C — Switch to Follow (end-effector chase cam)
- `navigate.reset()` before activating follow
- `followManipulator().setTargetProvider(callback)` — dynamic `Vec3f` callback updated each frame
- Simulates 120 frames (~2 s) of circular end-effector motion
- Logs effector position vs camera eye to show the lag

### D — Damping comparison
- `followManipulator().setDamping(20.0f)` — responsive, fast catch-up
- `followManipulator().setDamping(1.5f)` — cinematic, slow floaty feel

### E — Static world-space target
- `followManipulator().setTargetWorld(pos)` — clears provider, uses fixed point
- Useful when the target doesn't move every frame (e.g. a building overview)

### F — Switch back to inspect
- `follow.reset()` + `inspect.reset()` before resuming orbit
- Confirms orbit still works cleanly after all the mode switches

## Reset-on-switch pattern

```cpp
// Always reset() the outgoing controller before activating the next one.
// This clears stale drag velocities, held-key flags, and intermediate state
// that would otherwise carry over and cause phantom motion.
inspect.reset();
navigate.reset();
// Now safe to route events to navigate
```
