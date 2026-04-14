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
 * @file 05_example.h
 * @brief Example 05 — robotic simulator: Inspect3D and Navigation3D on one camera.
 *
 * Demonstrates runtime controller switching, moving pivot via Inspect3D (effector path),
 * orbit rotation damping, shared camera state, and reset() when switching modes.
 */

namespace vne::interaction::examples {

[[nodiscard]] int runRoboticSimulatorExample();

}  // namespace vne::interaction::examples
