# 08 — Camera State Save / Restore

Shows all state structs from `interaction_types.h` and demonstrates bookmark/undo patterns.

## What it covers

- **`OrbitCameraState`** — `coi_world`, `distance`, `world_up`, `yaw_deg`, `pitch_deg`; bookmark before interaction, restore to jump back
- **`TrackballCameraState`** — `coi_world`, `distance`, `rotation` (quaternion), `world_up`; capture after trackball drag
- **`FreeCameraState`** — `position`, `yaw_deg`, `pitch_deg`, `up_hint`; save/restore FPS camera position with `markAnglesDirty()` to re-sync manipulator yaw/pitch from camera pose
- **`FreeLookInputState`** — `move_forward/backward/left/right/up/down`, `sprint`, `slow`, `looking`; documents the per-frame held-key model of `FreeLookManipulator`
- **`OrbitalInteractionState`** — `rotating`, `panning`, `modifier_shift`, `last_x/y_px`; documents the per-frame drag-tracking model

## State struct summary

| Struct | Manipulator | Fields |
|--------|-------------|--------|
| `OrbitCameraState` | `OrbitalCameraManipulator` (eOrbit) | coi_world, distance, world_up, yaw_deg, pitch_deg |
| `TrackballCameraState` | `OrbitalCameraManipulator` (eTrackball) | coi_world, distance, rotation (Quatf), world_up |
| `FreeCameraState` | `FreeLookManipulator` | position, yaw_deg, pitch_deg, up_hint |
| `FreeLookInputState` | `FreeLookManipulator` | per-frame held-key flags |
| `OrbitalInteractionState` | `OrbitalCameraManipulator` | per-frame drag-tracking flags |

## Restore pattern

```cpp
// Orbit restore
manip.setPivot(saved.coi_world);
manip.setOrbitDistance(saved.distance);
ctrl.onUpdate(dt);  // manipulator re-derives camera pose

// FPS restore
camera->setPosition(saved.position);
camera->updateMatrices();
manip.markAnglesDirty();   // re-sync yaw/pitch from camera on next onUpdate
ctrl.reset();
```
