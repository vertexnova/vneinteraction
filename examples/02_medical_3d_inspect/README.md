# 02 — Medical 3D Inspect

Full coverage of `Inspect3DController` and `OrbitalCameraManipulator`.

## Use case

3D medical imaging, CAD review, scientific visualization — any scenario where a user orbits a fixed object with mouse input. The "medical" framing emphasises the landmark pivot pattern common in anatomy viewers.

## What it covers

### A — Virtual trackball orbit, inertia
- `setPivotMode(eCoi)`
- `setRotateSensitivity`, `setPanSensitivity`, `setZoomSensitivity`
- `setRotationInertiaEnabled(true)`, `setPanInertiaEnabled(true)`
- Fine-tune damping via escape hatch: `orbitalCameraManipulator().setRotationDamping / setPanDamping`
- LMB drag → orbit, RMB drag → pan, scroll → zoom

### B — Trackball projection modes
- `setTrackballProjectionMode(eHyperbolic)` — smooth centre-to-rim (default)
- `setTrackballProjectionMode(eRim)` — aggressive edge spin
- `setTrackballRotationScale` — overall trackball response

### C — Anatomy landmark, fixed pivot
- `setPivot(world_pos)` — lock orbit to a landmark
- `setPivotMode(eFixed)` — pan trucks eye + target without moving COI
- `setPivotOnDoubleClickEnabled(false)` — prevent accidental pivot change
- Read back `getCenterOfInterestWorld()` to verify COI stays near landmark

### D — DOF toggles
- `setRotationEnabled(false/true)` + `isRotationEnabled()`
- `setPanEnabled(false/true)`
- `setZoomEnabled(false/true)`

### E — ZoomMethod variants (via escape hatch)
- `ZoomMethod::eSceneScale` — virtual zoom, no geometry movement; read `getZoomScale()`
- `ZoomMethod::eChangeFov` — shrinks/widens FOV
- `ZoomMethod::eDollyToCoi` — moves camera along view ray

### F — fitToAABB + view direction presets
- `fitToAABB` with 60-frame smooth animation
- `setViewDirection(eTop / eFront / eIso)`

### G — Interaction speed step keys
- `setIncreaseInteractionSpeedKey` / `setDecreaseInteractionSpeedKey`
- `setInteractionSpeedStep(1.2f)` — multiplicative factor per key press
- Read back `getRotationSpeed()` / `getPanSpeed()` to verify scaling

### H — reset()
