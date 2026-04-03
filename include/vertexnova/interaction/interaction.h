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
 * @par Type surface (backward compatible with pre-split TUs)
 * This header includes, in order:
 * - @c vertexnova/interaction/camera_action.h — @ref CameraActionType, @ref CameraCommandPayload, @ref GestureAction
 * - @c vertexnova/interaction/camera_state.h — @ref OrbitCameraState, @ref TrackballCameraState, @ref FreeCameraState,
 * …
 * - @c vertexnova/interaction/input_binding.h — @ref InputRule, @ref MouseBinding, @ref KeyBinding, …
 * - @c vertexnova/interaction/interaction_types.h — behavioral enums (@ref OrbitalRotationMode, …) and the same three
 *   sub-headers again (harmless duplicate; include guards dedupe).
 *
 * TUs that previously depended only on @c interaction.h for those symbols should keep working without extra includes.
 *
 * @par Renamed manipulator API (shim)
 * Legacy type aliases (e.g. behavior → manipulator names) live in @c camera_manipulator.h, which this file includes
 * via the manipulator headers.
 *
 * @par Maintainer migration (optional diagnostic)
 * Define @c VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0 before including this file to emit a compile-time
 * reminder that the redundant direct includes above may be collapsed to @c interaction_types.h-only in a future
 * **2.0.0** major release; track via CHANGELOG. Not defined by default (no warning for normal apps).
 *
 * @par Usage
 * Prefer `#include <vertexnova/interaction/interaction.h>` once per translation unit that needs the full surface;
 * include @c interaction_types.h or individual headers when you want a smaller dependency set (faster builds).
 */

#if defined(VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0)
#if defined(_MSC_VER)
#pragma message(                                                                                                     \
    "vertexnova/interaction/interaction.h: optional migration note — redundant includes before interaction_types.h " \
    "may be tightened in v2.0.0; see camera_action.h, camera_state.h, input_binding.h, interaction_types.h, interaction.h")
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC warning \
    "interaction.h: VNE_INTERACTION_INTERACTION_H_DEPRECATION_BEFORE_2_0 — see file docs; v2.0.0 include policy"
#endif
#endif

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
