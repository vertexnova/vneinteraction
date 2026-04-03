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
 * @file 08_example.h
 * @brief Example 08 — save and restore camera state (orbit, trackball, free-look) and related snapshots.
 *
 * Covers OrbitCameraState, TrackballCameraState, FreeCameraState bookmark/undo patterns, reading
 * live fields from OrbitalCameraManipulator and FreeLookManipulator, FreeLookInputState diagnostics,
 * and OrbitalInteractionState via the manipulator API. Library structs are plain data; persistence
 * is application-defined.
 */

namespace vne::interaction::examples {

[[nodiscard]] int runCameraStateSaveRestoreExample();

}  // namespace vne::interaction::examples
