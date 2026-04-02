# 03 — Medical 2D Slices

Full coverage of `Ortho2DController` and `Ortho2DManipulator`.

## Use case

DICOM slice viewers, 2D maps, sprite editors, any orthographic viewport where the primary interactions are pan and zoom-at-cursor.

## What it covers

### A — Default pan + scroll zoom
- Orthographic camera creation (`CameraFactory::createOrthographic`)
- LMB drag → pan (default binding)
- Scroll → zoom-at-cursor
- `isRotationEnabled()` confirms rotation is off by default

### B — Pan inertia and damping
- `setPanInertiaEnabled(true)` — pan coasts after release
- `setPanSensitivity(damping)` — delegates to `Ortho2DManipulator::setPanDamping`; higher = faster stop
- `setPanInertiaEnabled(false)` — immediate stop comparison

### C — Zoom sensitivity
- `setZoomSensitivity(1.5f)` — aggressive zoom per scroll tick
- `setZoomSensitivity(1.05f)` — subtle zoom

### D — In-plane rotation
- `setRotationEnabled(true)` — enables RMB drag to rotate view in-plane
- `setRotateSensitivity(degrees_per_pixel)`
- Use case: slice reorientation in DICOM viewers

### E — Button rebinding
- `setPanButton(eMiddle)` — move pan to MMB
- `setRotateButton(eRight)` — assign rotation to RMB
- `setZoomScrollModifier` — require a modifier key before scroll zooms

### F — ZoomMethod variants (via escape hatch)
- `ortho2DManipulator().setZoomMethod(eSceneScale)` — read `getZoomScale()`
- `setZoomMethod(eChangeFov)` — adjusts orthographic half-extents
- `setZoomMethod(eDollyToCoi)` — cursor-anchored ortho zoom

### G — fitToAABB
- Frame a 256×256 mm DICOM slice region

### H — DOF gating
- `setZoomEnabled(false)` — scroll ignored
- `setPanEnabled(false)` — LMB drag ignored

### I — reset()
