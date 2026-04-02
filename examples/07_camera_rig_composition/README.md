# 07 — CameraRig Composition

Demonstrates using `CameraRig` directly to build single and hybrid manipulator stacks without a high-level controller.

## What it covers

- **Factory methods** — `makeOrbit`, `makeTrackball`, `makeFps`, `makeFly`, `makeOrtho2D`, `makeFollow`
- **`addManipulator`** — compose orbit + fly in one rig (both receive every action)
- **`setHandleZoom(false)`** on `FreeLookManipulator` — prevents double-zoom when orbit owns scroll
- **`InputMapper` wiring** — `setActionCallback` connects mapper output to `rig.onAction`
- **`removeManipulator`** — hot-swap a manipulator at runtime
- **`setEnabled`** — mute one manipulator while keeping the other active
- **`clearManipulators` + rebuild** — switch from orbit to trackball at runtime
- **Direct `onAction` dispatch** — drive the rig without a controller or mapper (useful in tests)
- **`resetState`** — clear stale drag state after focus loss

## Hybrid orbit + fly rig

```
CameraRig
  ├── OrbitalCameraManipulator   handles: eBeginRotate, eRotateDelta, eZoomAtCursor
  └── FreeLookManipulator        handles: eMoveForward, eLookDelta
      setHandleZoom(false)       ← orbit owns eZoomAtCursor; fly ignores it
```

Both manipulators receive every `CameraActionType`. Each silently ignores actions it doesn't implement.
