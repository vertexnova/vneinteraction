#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file interaction_types.h
 * @brief Umbrella header for all shared interaction types.
 *
 * Includes:
 * - Behavioral enums: @ref FreeLookMode, @ref ZoomMethod, @ref OrbitRotationMode,
 *   @ref OrbitPivotMode, @ref UpAxis, @ref ViewDirection, @ref CenterOfInterestSpace
 * - @ref camera_action.h — @ref CameraActionType, @ref CameraCommandPayload, @ref GestureAction
 * - @ref input_binding.h — @ref InputRule, @ref MouseBinding, @ref KeyBinding, touch structs,
 *   modifier bitmask constants
 * - @ref camera_state.h  — @ref OrbitCameraState, @ref TrackballCameraState, @ref FreeCameraState,
 *   @ref FreeLookInputState, @ref OrbitalInteractionState
 *
 * You may include only the focused sub-headers when the full set is not needed.
 *
 * @par Math helpers
 * Orbit/trackball **geometry** lives in @ref OrbitBehavior and @ref TrackballBehavior; this header
 * holds interaction enums, payloads, and grouped state used across manipulators.
 */

#include "vertexnova/interaction/camera_action.h"
#include "vertexnova/interaction/camera_state.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/input_binding.h"

#include <vertexnova/events/types.h>
#include <vertexnova/math/core/core.h>

#include <cstdint>

namespace vne::interaction {

/** Space for center-of-interest specification. */
enum class CenterOfInterestSpace : std::uint8_t {
    eWorldSpace = 0,   //!< Pivot point in absolute world coordinates
    eCameraSpace = 1,  //!< Pivot point in camera-relative coordinates
};

/** Preset view direction for camera framing. */
enum class ViewDirection : std::uint8_t {
    eFront = 0,   //!< Looking along +Z axis
    eBack = 1,    //!< Looking along -Z axis
    eLeft = 2,    //!< Looking along -X axis
    eRight = 3,   //!< Looking along +X axis
    eTop = 4,     //!< Looking down along -Y axis
    eBottom = 5,  //!< Looking up along +Y axis
    eIso = 6,     //!< Isometric view at 45-45 degrees
};

/** Zoom behavior method selection (declared in preferred UI / iteration order). */
enum class ZoomMethod : std::uint8_t {
    eSceneScale = 0,  //!< XY scene scale in the view matrix (virtual zoom)
    eChangeFov = 1,   //!< Adjust field of view (perspective) or ortho half-extents
    eDollyToCoi = 2,  //!< Move camera along view direction toward center of interest
};

/** Movement mode for FreeLookManipulator (FPS or Fly). */
enum class FreeLookMode : std::uint8_t {
    eFps = 0,  //!< FPS: world-up fixed, pitch clamped [-89°, 89°] (default)
    eFly = 1,  //!< Fly: unconstrained, up follows camera
};

/** Rotation algorithm for orbit-style camera (OrbitalCameraManipulator, Inspect3DController). */
enum class OrbitRotationMode : std::uint8_t {
    eOrbit = 0,      //!< Classic orbit (Euler yaw/pitch), pitch clamped [-89°, 89°]
    eTrackball = 1,  //!< Quaternion / virtual-trackball rotation (smooth, no gimbal lock)
};

/**
 * @brief Pivot control mode for orbit-style camera behaviors.
 *
 * Determines which point in world space the camera orbits around.
 * Used by OrbitalCameraManipulator and Inspect3DController.
 */
enum class OrbitPivotMode : std::uint8_t {
    eCoi = 0,         //!< Pan moves COI in the view plane; camera looks at COI (default).
    eViewCenter = 1,  //!< Same as eCoi during pan; on pan end, sync COI from camera target.
    eFixed = 2,       //!< Fixed world pivot; pan translates eye+target together; COI unchanged.
};

/** World up axis selection. */
enum class UpAxis : std::uint8_t {
    eY = 0,  //!< Y axis is world up (standard in graphics)
    eZ = 1,  //!< Z axis is world up (common in scientific/CAD)
};

/**
 * @brief Convert an UpAxis enum to a normalized world-space up vector.
 * @param axis The up axis (eY or eZ)
 * @return Normalized vector (0,1,0) for eY or (0,0,1) for eZ
 */
[[nodiscard]] inline vne::math::Vec3f toWorldUp(UpAxis axis) noexcept {
    return (axis == UpAxis::eZ) ? vne::math::Vec3f(0.0f, 0.0f, 1.0f) : vne::math::Vec3f(0.0f, 1.0f, 0.0f);
}

}  // namespace vne::interaction
