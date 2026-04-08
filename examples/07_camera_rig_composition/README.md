# 07 — CameraRig Composition

Demonstrates using `CameraRig` directly to build single and hybrid manipulator stacks without a high-level controller.

## What it covers

- **Factory methods** — `makeTrackball`, `makeFps`, `makeFly`, `makeOrtho2D`
- **`addManipulator`** — compose trackball orbit + fly in one rig (both receive every action)
- **`setHandleZoom(false)`** on `FreeLookManipulator` — prevents double-zoom when trackball owns scroll
- **`InputMapper` wiring** — `setActionCallback` connects mapper output to `rig.onAction`
- **`removeManipulator`** — hot-swap a manipulator at runtime
- **`setEnabled`** — mute one manipulator while keeping the other active
- **`clearManipulators` + rebuild** — e.g. switch trackball projection mode (`eRim`) on a fresh manipulator
- **Direct `onAction` dispatch** — drive the rig without a controller or mapper (useful in tests)
- **`resetState`** — clear stale drag state after focus loss

## Hybrid trackball + fly rig

```text
CameraRig
  ├── OrbitalCameraManipulator   handles: eBeginRotate, eRotateDelta, eZoomAtCursor
  └── FreeLookManipulator        handles: eMoveForward, eLookDelta
      setHandleZoom(false)       ← trackball owns eZoomAtCursor; fly ignores it
```

Both manipulators receive every `CameraActionType`. Each silently ignores actions it doesn't implement.
