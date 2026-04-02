/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 08: Camera state save / restore
 *
 * Demonstrates:
 *   - OrbitCameraState  — save/restore Euler orbit (COI, distance, yaw, pitch, up)
 *   - TrackballCameraState — save/restore quaternion trackball (COI, distance, quat, up)
 *   - FreeCameraState   — save/restore free-look (position, yaw, pitch, up_hint)
 *   - Typical "undo/bookmark" pattern: save before interaction, restore to jump back
 *   - Reading live state from OrbitalCameraManipulator fields
 *   - Reading live state from FreeLookManipulator fields
 *   - FreeLookInputState — querying held-key snapshot (diagnostic use)
 *   - OrbitalInteractionState — not exported as snapshot; shown via manipulator API
 *
 * Note: camera_state.h structs are plain data — no serialize/deserialize helpers
 * are provided by the library. Persistence to disk is the application's job.
 * ----------------------------------------------------------------------
 */

#pragma once

namespace vne::interaction::examples {

[[nodiscard]] int runCameraStateSaveRestoreExample();

}  // namespace vne::interaction::examples
