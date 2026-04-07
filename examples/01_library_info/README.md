# 01 — Library Info

Verifies linkage and enumerates the entire public API surface at startup.

## What it covers

- **Version** — `get_version()` string
- **All three controllers** — default-constructs each one and reads default property values:
  - `Inspect3DController`: trackball projection via `orbitalCameraManipulator().getTrackballProjectionMode`, `getPivotMode`, `isRotationEnabled`, `isPivotOnDoubleClickEnabled`
  - `Navigation3DController`: `getMode`, `getMoveSpeed`, `getMouseSensitivity`, `getSprintMultiplier`, `getSlowMultiplier`, `isLookEnabled`, `isMoveEnabled`, `isZoomEnabled`
  - `Ortho2DController`: `isRotationEnabled`
- **`ZoomMethod` enum** — all three values: `eSceneScale`, `eChangeFov`, `eDollyToCoi`
- **`InputMapper` presets** — calls all five (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`) and logs rule counts to confirm non-empty
- **`CameraRig` factories** — `makeTrackball`, `makeFps`, `makeFly`, `makeOrtho2D`; logs manipulator count per rig
- **Behavioral enums** — `TrackballProjectionMode`, `OrbitPivotMode`, `FreeLookMode`, `ViewDirection`

## Purpose

Use this as a smoke test after building — if it links and all counts are non-zero, the library is correctly wired. It also serves as a quick reference for what the defaults are for every controller.
