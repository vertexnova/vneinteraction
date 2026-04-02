#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file interaction.h
 * @brief Umbrella include for the VertexNova interaction library.
 *
 * Pulls in manipulators, @ref CameraRig, high-level controllers, @ref InputMapper,
 * @ref interaction_types.h, and orbit/trackball math helpers (@ref OrbitBehavior,
 * @ref TrackballBehavior).
 *
 * @par Usage
 * Prefer `#include <vertexnova/interaction/interaction.h>` once per translation unit
 * that needs the full surface; include specific headers instead if you want a minimal
 * dependency set (faster builds).
 */

// Sub-headers (can be included individually for minimal dependencies)
#include "vertexnova/interaction/camera_action.h"
#include "vertexnova/interaction/camera_state.h"
#include "vertexnova/interaction/input_binding.h"

// Full umbrella types (behavioral enums + re-exports all sub-headers above)
#include "vertexnova/interaction/interaction_types.h"

// Manipulators
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"
#include "vertexnova/interaction/follow_manipulator.h"

// Rig, mapper, controller interface
#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/camera_controller.h"

// High-level controllers
#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/interaction/ortho_2d_controller.h"
#include "vertexnova/interaction/follow_controller.h"

// Rotation / trackball math (public while still in include/)
#include "vertexnova/interaction/orbit_behavior.h"
#include "vertexnova/interaction/trackball_behavior.h"
