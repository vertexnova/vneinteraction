# VneInteraction Examples

This directory contains examples demonstrating the VneInteraction API.

## Building Examples

From the project root:

```bash
# Shared library build
./scripts/build_macos.sh   # macOS
# or
cmake -B build -DVNE_INTERACTION_EXAMPLES=ON -DVNE_INTERACTION_LIB_TYPE=shared
cmake --build build
```

Alternatively, `-DVNE_INTERACTION_DEV=ON` enables both tests and examples.

Executables are placed in `build/shared/Debug/bin/examples/` (or `build/static/bin/examples/` for static builds).

## Available Examples

| Example | Description |
|---------|-------------|
| **01_library_info** | Version info; list all behavior types and presets |
| **02_medical_3d_inspect** | Inspect3DController — trackball rotation, landmark pivot, fitToAABB |
| **03_medical_2d_slices** | Ortho2DController — pan, scroll-zoom, orthographic camera |
| **04_game_editor_camera** | Navigation3DController — FPS + orbit modes, WASD |
| **05_robotic_simulator** | Inspect3DController + Navigation3DController + FollowController |

## Quick Reference

| Example | Controller | Focus |
|---------|------------|-------|
| 01_library_info | — | Version, behavior listing |
| 02_medical_3d_inspect | Inspect3DController | 3D inspection, trackball |
| 03_medical_2d_slices | Ortho2DController | 2D slices, ortho pan+zoom |
| 04_game_editor_camera | Navigation3DController | Game/editor camera |
| 05_robotic_simulator | Inspect + Navigate + Follow | Robotic simulator |

## Common

The `common/` folder contains shared helpers:

- `input_simulation.h` — Simulate mouse drag, scroll, key hold for headless testing
- `key_codes.h` — Key code constants
- `logging_guard.h` — Optional logging setup
