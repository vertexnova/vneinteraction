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
 * @file 02_example.h
 * @brief Example 02 — medical 3D inspect using Inspect3DController (headless simulated input).
 *
 * Demonstrates Euler orbit vs trackball, DOF toggles, fixed landmark pivot, inertia and damping,
 * all ZoomMethod variants, fitToAABB, view presets, interaction speed-step keys, and
 * orbitalCameraManipulator() for fine-grained tuning.
 */

namespace vne::interaction::examples {

[[nodiscard]] int runMedical3dInspectExample();

}  // namespace vne::interaction::examples
