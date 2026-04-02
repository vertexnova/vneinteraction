# VneInteraction — Examples

Headless (no window, no GPU) examples that drive the library through its full public API using simulated input events. Each example is a self-contained `main.cpp` that builds and runs standalone.

## Building

```bash
# Dev preset (tests + examples)
cmake -B build -DVNE_INTERACTION_DEV=ON
cmake --build build

# Examples only
cmake -B build -DVNE_INTERACTION_EXAMPLES=ON
cmake --build build
```

Executables land in `build/bin/examples/`.

## Running

```bash
./build/bin/examples/01_library_info
./build/bin/examples/02_medical_3d_inspect
# ... etc.
```

---

## Examples

| # | Directory | Controller(s) | What it covers |
|---|-----------|---------------|----------------|
| 01 | `01_library_info` | All | Version, all preset rule counts, all controller defaults, ZoomMethod and rotation-mode enums, CameraRig factory methods |
| 02 | `02_medical_3d_inspect` | `Inspect3DController` | Euler orbit + trackball modes, inertia damping, fixed landmark pivot, DOF toggles, all three ZoomMethod variants, fitToAABB, view presets, interaction speed step keys, `orbitalCameraManipulator()` escape hatch |
| 03 | `03_medical_2d_slices` | `Ortho2DController` | Pan + scroll zoom, in-plane rotation, pan inertia, zoom sensitivity, button rebinding, ZoomMethod variants via `ortho2DManipulator()`, fitToAABB, DOF gating |
| 04 | `04_game_editor_camera` | `Navigation3DController` | FPS and Fly modes, WASD + mouse look, sprint/slow, full 6-DoF key bindings (up/down), key rebinding to arrow keys, DOF gating, discrete speed-step keys, `freeLookManipulator()` escape hatch, ZoomMethod variants |
| 05 | `05_robotic_simulator` | `Inspect3DController` + `Navigation3DController` + `FollowController` | Runtime controller switching, callback-based dynamic follow target, static world-space follow target, damping comparison (responsive vs cinematic), reset-on-switch pattern |
| 06 | `06_custom_input_bindings` | `InputMapper` + `Inspect3DController` | All presets, `bindGesture` / `bindScroll` / `bindDoubleClick` / `bindKey` / `unbindKey` / `unbindGesture`, direct `InputRule` struct construction, full `addRule` / `clearRules` workflow, `onMouseButton` / `onMouseMove` / `onMouseScroll` / `onKey` direct drive, touch pan + pinch, `resetState` on focus loss |
| 07 | `07_camera_rig_composition` | `CameraRig` (direct) | All factory methods, hybrid orbit+fly rig via `addManipulator`, `removeManipulator` hot-swap, `clearManipulators` + rebuild, `setEnabled` per manipulator, `setHandleZoom(false)` to avoid double-zoom, `resetState` |
| 08 | `08_camera_state_save_restore` | `Inspect3DController` + `Navigation3DController` | `OrbitCameraState` and `TrackballCameraState` bookmark/undo, `FreeCameraState` save/restore with `markAnglesDirty`, `FreeLookInputState` held-key model, `OrbitalInteractionState` drag-tracking model |

---

## Common helpers (`common/`)

| File | Purpose |
|------|---------|
| `input_simulation.h` | `simulateMouseDrag`, `simulateMouseScroll`, `simulateKeyHold`, `runFrames` — drive event-based controllers headlessly |
| `key_codes.h` | GLFW-compatible key code constants (W=87, A=65, S=83, D=68, Q=81, E=69, Shift=340) |
| `logging_guard.h` | RAII console logger; wrap `main()` with `LoggingGuard` to see output |
