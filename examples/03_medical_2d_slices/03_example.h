/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 03: Medical 2D slices — Ortho2DController
 *
 * Demonstrates:
 *   - Orthographic camera setup
 *   - Default pan (LMB drag) and zoom-at-cursor (scroll)
 *   - Enable in-plane rotation for slice reorientation
 *   - Pan inertia enable/disable and damping
 *   - Zoom sensitivity tuning
 *   - Button rebinding (MMB pan, RMB rotate)
 *   - ZoomMethod variants via ortho2DManipulator() escape hatch
 *   - fitToAABB to frame a DICOM slice region
 *   - reset()
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runMedical2dSlicesExample();

}  // namespace vne::interaction::examples
