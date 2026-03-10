#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

#include <cstdint>

namespace vne::interaction {

enum class CameraManipulatorType : std::uint8_t {
    eOrbit = 0,
    eArcball = 1,
    eFps = 2,
    eFly = 3,
    eOrthoPanZoom = 4,
    eFollow = 5,
};

enum class CenterOfInterestSpace : std::uint8_t {
    eWorldSpace = 0,
    eCameraSpace = 1,
};

enum class ViewDirection : std::uint8_t {
    eFront = 0,
    eBack = 1,
    eLeft = 2,
    eRight = 3,
    eTop = 4,
    eBottom = 5,
    eIso = 6,
};

enum class ZoomMethod : std::uint8_t {
    eDollyToCoi = 0,
    eSceneScale = 1,
    eChangeFov = 2,
};

enum class UpAxis : std::uint8_t {
    eY = 0,
    eZ = 1,
};

enum class MouseButton : int {
    eLeft = 0,
    eRight = 1,
    eMiddle = 2,
};

/// Button indices for rotate/pan (e.g. left/right mouse). Used by orbit-style manipulators.
struct VNE_INTERACTION_API ButtonMap {
    int rotate = static_cast<int>(MouseButton::eLeft);
    int pan = static_cast<int>(MouseButton::eRight);
};

struct VNE_INTERACTION_API TouchPan final {
    float delta_x_px = 0.0f;
    float delta_y_px = 0.0f;
};

struct VNE_INTERACTION_API TouchPinch final {
    float scale = 1.0f;
    float center_x_px = 0.0f;
    float center_y_px = 0.0f;
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
    eFitBounds = 19,
    eResetView = 20,
};

/// Payload for actions that carry pointer/cursor or delta data.
struct VNE_INTERACTION_API CameraCommandPayload {
    float x_px = 0.0f;
    float y_px = 0.0f;
    float delta_x_px = 0.0f;
    float delta_y_px = 0.0f;
    float zoom_factor = 1.0f;
    bool pressed = false;
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

}  // namespace vne::interaction
