#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @file interaction.h
 * @brief Umbrella header for the VertexNova interaction library.
 *
 * Includes all public headers for camera manipulators, controllers, behaviors,
 * and input handling.
 */

// --- New composable behavior system ---
#include "vertexnova/interaction/camera_behavior.h"
#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/interaction/interaction_types.h"
// --- High-level controllers ---
#include "vertexnova/interaction/inspect_controller.h"
#include "vertexnova/interaction/navigate_controller.h"
#include "vertexnova/interaction/planar_controller.h"
#include "vertexnova/interaction/follow_controller.h"
// --- Legacy manipulators (kept for backward compatibility) ---
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"
#include "vertexnova/interaction/follow_manipulator.h"
