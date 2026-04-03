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
 * @file 07_example.h
 * @brief Example 07 — CameraRig composition: factories, hybrid rigs, and runtime manipulator edits.
 *
 * Demonstrates factory methods, addManipulator for hybrid orbit + fly, removeManipulator hot-swap,
 * clearManipulators and rebuild, per-manipulator setEnabled, setHandleZoom(false) to avoid
 * double-zoom, direct onAction(), and resetState().
 */

namespace vne::interaction::examples {

[[nodiscard]] int runCameraRigCompositionExample();

}  // namespace vne::interaction::examples
