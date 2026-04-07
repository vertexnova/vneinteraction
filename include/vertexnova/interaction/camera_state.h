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
 * @brief Serializable camera state blobs: @ref TrackballCameraState, @ref FreeCameraState,
 *        and interaction state structs.
 *
 * These POD-like structs hold the logical camera pose for each manipulator type and can
 * be saved/restored independently of the ICamera object. Also includes grouped input-state
 * structs (@ref FreeLookInputState, @ref OrbitalInteractionState) used across manipulators.
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

/**
 * @brief Per-frame input state for FreeLookManipulator (held keys).
 */
struct VNE_INTERACTION_API FreeLookInputState {
    bool move_forward = false;   //!< Move forward by like W key or mouse wheel
    bool move_backward = false;  //!< Move backward by like S key or mouse wheel
    bool move_left = false;      //!< Move left by like A key or mouse wheel
    bool move_right = false;     //!< Move right by like D key or mouse wheel
    bool move_up = false;        //!< Move up by like E key or mouse wheel
    bool move_down = false;      //!< Move down by like Q key or mouse wheel
    bool sprint = false;         //!< Sprint the movement speed by like Shift key or mouse wheel
    bool slow = false;           //!< Slow the movement speed by like Ctrl key or mouse wheel
    bool looking = false;        //!< Looking with the mouse by like Right mouse button or touch pan
};

/**
 * @brief Drag/pan interaction state for OrbitalCameraManipulator.
 */
struct VNE_INTERACTION_API OrbitalInteractionState {
    bool rotating = false;        //!< Rotating the camera by like Left mouse button or touch rotate
    bool panning = false;         //!< Panning the camera by like Middle mouse button or touch pan
    bool modifier_shift = false;  //!< Modifier shift key by like Shift key or touch modifier shift
    float last_x_px = 0.0f;       //!< Last x position in pixels
    float last_y_px = 0.0f;       //!< Last y position in pixels
};

// -----------------------------------------------------------------------------
// Camera pose state blobs (manipulators operate on these, then apply to ICamera)
// -----------------------------------------------------------------------------

/**
 * @brief Serializable state for trackball camera (OrbitalCameraManipulator in eTrackball mode).
 */
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

/**
 * @brief Serializable state for free-look camera (FreeLookManipulator).
 */
struct VNE_INTERACTION_API FreeCameraState {
    vne::math::Vec3f position;     //!< Camera position in world space
    vne::math::Quatf orientation;  //!< Camera-to-world rotation

    FreeCameraState() noexcept
        : position(0.0f, 0.0f, 0.0f)
        , orientation(0.0f, 0.0f, 0.0f, 1.0f) {}
};

}  // namespace vne::interaction

#ifdef _MSC_VER
#pragma warning(pop)
#endif
