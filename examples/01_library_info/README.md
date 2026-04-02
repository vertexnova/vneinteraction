# 01 — Library Info

Verifies linkage and enumerates the entire public API surface at startup.

## What it covers

- **Version** — `get_version()` string
- **All four controllers** — default-constructs each one and reads default property values:
  - `Inspect3DController`: `getRotationMode`, `getPivotMode`, `isRotationEnabled`, `isPivotOnDoubleClickEnabled`
  - `Navigation3DController`: `getMode`, `getMoveSpeed`, `getMouseSensitivity`, `getSprintMultiplier`, `getSlowMultiplier`, `isLookEnabled`, `isMoveEnabled`, `isZoomEnabled`
  - `Ortho2DController`: `isRotationEnabled`
  - `FollowController`: `getLag`, `getOffset`
- **`ZoomMethod` enum** — all three values: `eSceneScale`, `eChangeFov`, `eDollyToCoi`
- **`InputMapper` presets** — calls all five (`orbitPreset`, `fpsPreset`, `gamePreset`, `cadPreset`, `orthoPreset`) and logs rule counts to confirm non-empty
- **`CameraRig` factories** — `makeOrbit`, `makeTrackball`, `makeFps`, `makeFly`, `makeOrtho2D`, `makeFollow`; logs manipulator count per rig
- **All behavioral enums** — `OrbitalRotationMode`, `TrackballProjectionMode`, `OrbitPivotMode`, `FreeLookMode`, `ViewDirection`

## Purpose

Use this as a smoke test after building — if it links and all counts are non-zero, the library is correctly wired. It also serves as a quick reference for what the defaults are for every controller.
