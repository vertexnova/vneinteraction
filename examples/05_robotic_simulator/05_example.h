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
 * @brief Example 05 — robotic simulator: Inspect3D, Navigation3D, and Follow on one camera.
 *
 * Demonstrates runtime controller switching, FollowController with dynamic and static targets,
 * damping comparison, shared camera state, and reset() when switching modes.
 */

namespace vne::interaction::examples {

[[nodiscard]] int runRoboticSimulatorExample();

}  // namespace vne::interaction::examples
