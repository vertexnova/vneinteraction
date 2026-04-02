#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_state.h
 * @brief Serializable camera state blobs: @ref OrbitCameraState, @ref TrackballCameraState,
 *        @ref FreeCameraState, and interaction state structs.
 *
 * These POD-like structs hold the logical camera pose for each manipulator type and can
 * be saved/restored independently of the ICamera object. Also includes grouped input-state
 * structs (@ref FreeLookInputState, @ref OrbitInteractionState) used across manipulators.
 *
 * Include this header when you only need camera state types without input-binding or
 * action-command types. The umbrella @ref interaction_types.h includes everything.
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)   // dll-interface for member type from another lib
#pragma warning(disable : 26495)  // uninitialized member (ctor initializes all)
#endif

namespace vne::interaction {

// -----------------------------------------------------------------------------
// Grouped input / interaction state
// -----------------------------------------------------------------------------

/** Per-frame input state for FreeLookManipulator (held keys). */
struct VNE_INTERACTION_API FreeLookInputState {
    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;
    bool sprint = false;
    bool slow = false;
    bool looking = false;
};

/** Drag/pan interaction state for OrbitalCameraManipulator. */
struct VNE_INTERACTION_API OrbitInteractionState {
    bool rotating = false;
    bool panning = false;
    bool modifier_shift = false;
    float last_x_px = 0.0f;
    float last_y_px = 0.0f;
};

// -----------------------------------------------------------------------------
// Camera pose state blobs (manipulators operate on these, then apply to ICamera)
// -----------------------------------------------------------------------------

/** Serializable state for Euler-orbit camera (OrbitalCameraManipulator in eOrbit mode). */
struct VNE_INTERACTION_API OrbitCameraState {
    vne::math::Vec3f coi_world;  //!< Center of interest in world space
    float distance = 5.0f;       //!< Camera-to-pivot distance
    vne::math::Vec3f world_up;   //!< World up vector
    float yaw_deg = 0.0f;        //!< Yaw angle in degrees
    float pitch_deg = 0.0f;      //!< Pitch angle in degrees

    OrbitCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

/** Serializable state for trackball camera (OrbitalCameraManipulator in eTrackball mode). */
struct VNE_INTERACTION_API TrackballCameraState {
    vne::math::Vec3f coi_world;  //!< Center of interest in world space
    float distance = 5.0f;       //!< Camera-to-pivot distance
    vne::math::Quatf rotation;   //!< Orientation quaternion
    vne::math::Vec3f world_up;   //!< World up vector

    TrackballCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f, 1.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

/** Serializable state for free-look camera (FreeLookManipulator). */
struct VNE_INTERACTION_API FreeCameraState {
    vne::math::Vec3f position;  //!< Camera position in world space
    float yaw_deg = 0.0f;       //!< Yaw angle in degrees
    float pitch_deg = 0.0f;     //!< Pitch angle in degrees
    vne::math::Vec3f up_hint;   //!< World up hint vector

    FreeCameraState() noexcept
        : position(0.0f, 0.0f, 0.0f)
        , up_hint(0.0f, 1.0f, 0.0f) {}
};

}  // namespace vne::interaction

#ifdef _MSC_VER
#pragma warning(pop)
#endif
