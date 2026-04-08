/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 05: Robotic simulator — multi-controller switching
 *
 * Demonstrates:
 *   - Two view modes on one camera:
 *       Inspect3DController — orbit / inspect robot arm or moving pivot
 *       Navigation3DController — walk the environment (FPS)
 *   - Runtime controller switching (Tab-key pattern)
 *   - Moving pivot each frame to track a simulated end-effector path
 *   - Orbit rotation damping (snappy vs floaty decay)
 *   - Shared camera state across controllers
 *   - reset() on controller switch to clear stale drag state
 * ----------------------------------------------------------------------
 */

#include "05_example.h"

int main() {
    return vne::interaction::examples::runRoboticSimulatorExample();
}
