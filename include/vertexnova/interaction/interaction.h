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
 * and the full type surface (actions, state blobs, bindings, behavioral enums).
 *
 * @par Type surface
 * This header includes @c vertexnova/interaction/interaction_types.h, the single source for actions, state blobs,
 * input bindings, and behavioral enums.
 *
 * TUs that previously depended only on @c interaction.h for those symbols should keep working without extra includes.
 *
 * @par Renamed manipulator API (shim)
 * Legacy type aliases (e.g. behavior → manipulator names) live in @c camera_manipulator.h, which this file includes
 * via the manipulator headers.
 *
 * @par Maintainer migration (optional diagnostic)
 * Define @c VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0 before including this file to emit a compile-time
 * reminder that the umbrella surface may narrow further in a future **2.0.0** major release; track via CHANGELOG.
 * Not defined by default (no warning for normal apps).
 *
 * @par Usage
 * Prefer `#include <vertexnova/interaction/interaction.h>` once per translation unit that needs the full surface;
 * include @c interaction_types.h when you want types only.
 */

// clang-format off
#if defined(VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0)
#if defined(_MSC_VER)
#pragma message("interaction.h: VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0 — see @file Doxygen; v2.0.0")
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC warning "interaction.h: VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0 — see @file; v2.0.0"
#endif
#endif
// clang-format on

// Full type surface (actions, state blobs, bindings, behavioral enums)
#include "vertexnova/interaction/interaction_types.h"

// Manipulators
#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"
#include "vertexnova/interaction/free_look_manipulator.h"
#include "vertexnova/interaction/ortho_2d_manipulator.h"

// Rig, mapper, controller interface
#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/camera_controller.h"

// High-level controllers
#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/interaction/ortho_2d_controller.h"
