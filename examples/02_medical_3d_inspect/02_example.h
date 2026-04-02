/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 02: Medical 3D inspect — Inspect3DController
 *
 * Demonstrates:
 *   - Euler orbit (default) and trackball rotation modes
 *   - Rotation / pan / zoom DOF toggles
 *   - Landmark (fixed) pivot for anatomy-anchored rotation
 *   - Inertia enable/disable and damping tuning
 *   - Three ZoomMethod variants (scene scale, FOV, dolly)
 *   - fitToAABB with smooth animation frames
 *   - View direction presets (top, front, iso)
 *   - Interaction speed step keys (optional)
 *   - orbitalCameraManipulator() escape hatch for fine-grained tuning
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runMedical3dInspectExample();

}  // namespace vne::interaction::examples
