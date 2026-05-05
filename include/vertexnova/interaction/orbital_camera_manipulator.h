#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Deprecated shim — include @ref trackball_manipulator.h instead.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/trackball_manipulator.h"

namespace vne::interaction {

using OrbitalCameraManipulator [[deprecated("Use TrackballManipulator and trackball_manipulator.h")]] =
    TrackballManipulator;

}  // namespace vne::interaction
