#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 *
 * Autodoc:   no
 * ----------------------------------------------------------------------
 */

/**
 * @file 04_example.h
 * @brief Example 04 — game / editor camera using Navigation3DController (headless simulated input).
 *
 * Demonstrates FPS vs Fly modes, WASD + look with sprint/slow, full 6-DoF bindings and rebinding,
 * DOF gating, discrete speed-step keys, freeLookManipulator() tuning, ZoomMethod variants,
 * fitToAABB, and reset().
 */

namespace vne::interaction::examples {

[[nodiscard]] int runGameEditorCameraExample();

}  // namespace vne::interaction::examples
