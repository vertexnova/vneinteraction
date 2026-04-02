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
 * @file 03_example.h
 * @brief Example 03 — medical 2D slices using Ortho2DController (headless simulated input).
 *
 * Demonstrates orthographic setup, default pan and scroll zoom, optional in-plane rotation,
 * pan inertia and zoom sensitivity, button rebinding, ZoomMethod via ortho2DManipulator(),
 * fitToAABB for a slice region, and reset().
 */

namespace vne::interaction::examples {

[[nodiscard]] int runMedical2dSlicesExample();

}  // namespace vne::interaction::examples
