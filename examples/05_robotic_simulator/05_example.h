/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 05: Robotic simulator — multi-controller switching
 *
 * Demonstrates:
 *   - Three view modes on one camera:
 *       Inspect3DController  — inspect robot arm / tool in orbit
 *       Navigation3DController — walk the environment (FPS)
 *       FollowController      — end-effector chase cam
 *   - Runtime controller switching (Tab-key pattern)
 *   - FollowController with dynamic callback target (Vec3f provider)
 *   - FollowController with static world-space target
 *   - FollowManipulator damping: responsive vs cinematic
 *   - Shared camera state across all three controllers
 *   - reset() on controller switch to clear stale drag state
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runRoboticSimulatorExample();

}  // namespace vne::interaction::examples
