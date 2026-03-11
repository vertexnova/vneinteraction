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
 * @brief Types, enums, and structs for camera interaction (manipulators, gestures, bindings).
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

#include <cstdint>

namespace vne::interaction {

/** Camera manipulator type enum for selecting interaction model. */
enum class CameraManipulatorType : std::uint8_t {
    eOrbit = 0,         //!< Orbit-style rotation around center of interest
    eArcball = 1,       //!< Arcball-style quaternion-based smooth rotation
    eFps = 2,           //!< FPS-style movement with world-up constraint
    eFly = 3,           //!< Fly-style 6-DOF unconstrained movement
    eOrthoPanZoom = 4,  //!< Orthographic pan and zoom (no rotation)
    eFollow = 5,        //!< Follow-style autonomous behavior tracking
};

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

/** Zoom behavior method selection. */
enum class ZoomMethod : std::uint8_t {
    eDollyToCoi = 0,  //!< Move camera along view direction toward center of interest
    eSceneScale = 1,  //!< Scale entire scene relative to camera
    eChangeFov = 2,   //!< Adjust field of view (perspective cameras only)
};

/** Controls which point in world space the camera rotates around (the center of interest / pivot point). */
enum class RotationPivotMode : std::uint8_t {
    eCoi = 0,  //!< Rotate around the current center of interest in world space (default; panning moves this pivot)
    eViewCenter = 1,  //!< On pan end, the center of interest updates to the camera's view center (current camera
                      //!< target); subsequent rotations pivot around the point you panned to
    eFixedWorld = 2,  //!< The world-space pivot is fixed; panning translates eye and target together without changing
                      //!< the center of interest; rotation always orbits around this fixed pivot
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

/** Mouse button enumeration. */
enum class MouseButton : int {
    eLeft = 0,    //!< Left mouse button
    eRight = 1,   //!< Right mouse button
    eMiddle = 2,  //!< Middle/wheel mouse button
};

/** Button indices for rotate/pan (e.g. left/right mouse). Used by orbit-style manipulators. */
struct VNE_INTERACTION_API ButtonMap {
    int rotate = static_cast<int>(MouseButton::eLeft);  //!< Button for rotation (default: left mouse)
    int pan = static_cast<int>(MouseButton::eRight);    //!< Button for panning (default: right mouse)
};

/** Touch pan gesture data with screen pixel deltas. */
struct VNE_INTERACTION_API TouchPan final {
    float delta_x_px = 0.0f;  //!< Horizontal pan delta in screen pixels
    float delta_y_px = 0.0f;  //!< Vertical pan delta in screen pixels
};

/** Touch pinch (zoom) gesture data with scale and center position. */
struct VNE_INTERACTION_API TouchPinch final {
    float scale = 1.0f;        //!< Zoom scale factor (>1 zooms in, <1 zooms out)
    float center_x_px = 0.0f;  //!< Pinch center X position in screen pixels
    float center_y_px = 0.0f;  //!< Pinch center Y position in screen pixels
};

// -----------------------------------------------------------------------------
// Input bindings (customizable mouse buttons, keys, modifiers)
// -----------------------------------------------------------------------------
/// Configurable bindings for camera input when using CameraSystemController.
/// Defaults match GLFW-like values (left=rotate, right=pan/look, WASD/QE, shift/ctrl).
/// Set via CameraSystemController::setInputBindings() to support different setups.
struct VNE_INTERACTION_API CameraInputBindings {
    int rotate_button = static_cast<int>(MouseButton::eLeft);
    int pan_button = static_cast<int>(MouseButton::eRight);
    int pan_button_middle = static_cast<int>(MouseButton::eMiddle);  // additional pan alias
    int look_button = static_cast<int>(MouseButton::eRight);

    int key_move_forward = 87;   // W
    int key_move_backward = 83;  // S
    int key_move_left = 65;      // A
    int key_move_right = 68;     // D
    int key_move_up = 69;        // E
    int key_move_down = 81;      // Q

    int key_shift_left = 340;
    int key_shift_right = 344;
    int key_ctrl_left = 341;
    int key_ctrl_right = 345;
};

// -----------------------------------------------------------------------------
// Camera action / command layer (intent between input and manipulator)
// -----------------------------------------------------------------------------
enum class CameraActionType : std::uint8_t {
    eBeginRotate = 0,
    eRotateDelta = 1,
    eEndRotate = 2,
    eBeginPan = 3,
    ePanDelta = 4,
    eEndPan = 5,
    eZoomAtCursor = 6,
    eLookDelta = 7,
    eBeginLook = 8,
    eEndLook = 9,
    eMoveForward = 10,
    eMoveBackward = 11,
    eMoveLeft = 12,
    eMoveRight = 13,
    eMoveUp = 14,
    eMoveDown = 15,
    eSprintModifier = 16,
    eSlowModifier = 17,
    eOrbitPanModifier = 18,  // shift: orbit uses for pan-alias, free uses SprintModifier
    eResetView = 19,
};

/** Payload for actions that carry pointer/cursor or delta data. */
struct VNE_INTERACTION_API CameraCommandPayload {
    float x_px = 0.0f;         //!< Absolute X position in screen pixels
    float y_px = 0.0f;         //!< Absolute Y position in screen pixels
    float delta_x_px = 0.0f;   //!< Relative X delta in screen pixels
    float delta_y_px = 0.0f;   //!< Relative Y delta in screen pixels
    float zoom_factor = 1.0f;  //!< Zoom scaling factor
    bool pressed = false;      //!< Button pressed state
};

// -----------------------------------------------------------------------------
// Grouped input / interaction state (replaces scattered booleans)
// -----------------------------------------------------------------------------
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

struct VNE_INTERACTION_API OrbitInteractionState {
    bool rotating = false;
    bool panning = false;
    bool modifier_shift = false;
    float last_x_px = 0.0f;
    float last_y_px = 0.0f;
};

// -----------------------------------------------------------------------------
// Internal camera state (manipulators operate on these, then apply to ICamera)
// -----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)   // dll-interface for member type from another lib
#pragma warning(disable : 26495)  // uninitialized member (ctor initializes all)
#endif

struct VNE_INTERACTION_API OrbitCameraState {
    vne::math::Vec3f coi_world;
    float distance = 5.0f;
    vne::math::Vec3f world_up;
    float yaw_deg = 0.0f;
    float pitch_deg = 0.0f;

    OrbitCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

struct VNE_INTERACTION_API ArcballCameraState {
    vne::math::Vec3f coi_world;
    float distance = 5.0f;
    vne::math::Quatf rotation;
    vne::math::Vec3f world_up;

    ArcballCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f, 1.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

struct VNE_INTERACTION_API FreeCameraState {
    vne::math::Vec3f position;
    float yaw_deg = 0.0f;
    float pitch_deg = 0.0f;
    vne::math::Vec3f up_hint;

    FreeCameraState() noexcept
        : position(0.0f, 0.0f, 0.0f)
        , up_hint(0.0f, 1.0f, 0.0f) {}
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace vne::interaction
