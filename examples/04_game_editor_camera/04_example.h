/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 04: Game / editor camera — Navigation3DController
 *
 * Demonstrates:
 *   - FPS mode (world-up constrained, pitch clamped ±89°)
 *   - Fly mode (unconstrained 6-DoF)
 *   - WASD + look, sprint, slow modifiers
 *   - Full 6-DoF key bindings: up/down (E/Q by default)
 *   - Key rebinding (arrow keys, custom look button)
 *   - DOF gating: setLookEnabled / setMoveEnabled / setZoomEnabled
 *   - Discrete speed-step keys (]/[ to adjust move speed in-flight)
 *   - freeLookManipulator() escape hatch for direct tuning
 *   - ZoomMethod variants
 *   - fitToAABB and reset()
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runGameEditorCameraExample();

}  // namespace vne::interaction::examples
