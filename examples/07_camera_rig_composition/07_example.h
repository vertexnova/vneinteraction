/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 07: CameraRig composition
 *
 * Demonstrates:
 *   - CameraRig factory methods (makeOrbit, makeTrackball, makeFps, makeFly,
 *     makeOrtho2D, makeFollow)
 *   - addManipulator() for a hybrid orbit + fly rig
 *   - removeManipulator() to hot-swap a manipulator at runtime
 *   - clearManipulators() and manual rebuild
 *   - setEnabled() on individual manipulators (mute one, keep the other)
 *   - setHandleZoom(false) on FreeLookManipulator to avoid double-zoom
 *   - Direct onAction() dispatch (bypass controller / mapper)
 *   - resetState() on the rig
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runCameraRigCompositionExample();

}  // namespace vne::interaction::examples
