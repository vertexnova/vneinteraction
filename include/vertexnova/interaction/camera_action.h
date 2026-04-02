#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_action.h
 * @brief Camera command / intent layer: @ref CameraActionType, @ref CameraCommandPayload,
 *        @ref GestureAction.
 *
 * These types form the **intent bridge** between the input layer (@ref InputMapper, controllers)
 * and the manipulation layer (@ref CameraRig, @ref ICameraManipulator). Each manipulator consumes
 * the subset of actions it implements and silently ignores the rest.
 *
 * Include this header when you only need the action enum and payload, without pulling in
 * input-binding or camera-state types. The umbrella @ref interaction_types.h includes everything.
 */

#include "vertexnova/interaction/export.h"

#include <cstdint>

namespace vne::interaction {

/**
 * @brief High-level gesture actions for remappable bindings.
 *
 * Used with InputMapper::bindGesture, bindScroll, bindDoubleClick to customize
 * controls without exposing InputRule or CameraActionType.
 */
enum class GestureAction : std::uint8_t {
    eRotate = 0,    //!< Orbit / trackball rotate (button + drag)
    ePan = 1,       //!< Pan (button + drag)
    eZoom = 2,      //!< Zoom (scroll wheel)
    eLook = 3,      //!< FPS-style look (button + drag)
    eSetPivot = 4,  //!< Set orbit pivot via double-click (maps to orbit COI; see OrbitalCameraManipulator)
};

// -----------------------------------------------------------------------------
// Camera action / command layer (intent between input and manipulator)
// -----------------------------------------------------------------------------

/**
 * @brief Semantic camera commands emitted by @ref InputMapper to @ref CameraRig.
 *
 * Manipulators respond to the subset of actions they implement; unhandled actions are ignored.
 */
enum class CameraActionType : std::uint8_t {
    eNone = 0,  //!< Sentinel: no action for this event phase (used in InputRule)
    eBeginRotate = 1,
    eRotateDelta = 2,
    eEndRotate = 3,
    eBeginPan = 4,
    ePanDelta = 5,
    eEndPan = 6,
    eZoomAtCursor = 7,
    eLookDelta = 8,
    eBeginLook = 9,
    eEndLook = 10,
    eMoveForward = 11,
    eMoveBackward = 12,
    eMoveLeft = 13,
    eMoveRight = 14,
    eMoveUp = 15,
    eMoveDown = 16,
    eSprintModifier = 17,
    eSlowModifier = 18,
    eOrbitPanModifier = 19,  //!< Orbit: Shift+LMB pan alias; navigation: sprint-like where mapped.
    eResetView = 20,
    eSetPivotAtCursor =
        21,  //!< Double-click: set COI along view direction (camera + front * orbit distance); payload x/y ignored
    eIncreaseMoveSpeed = 22,         //!< Discrete: increase FPS move speed (Navigation3DController)
    eDecreaseMoveSpeed = 23,         //!< Discrete: decrease FPS move speed
    eIncreaseInteractionSpeed = 24,  //!< Discrete: scale orbit pan/rotate sensitivity (Inspect3DController)
    eDecreaseInteractionSpeed = 25,  //!< Discrete: scale orbit pan/rotate sensitivity down
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

}  // namespace vne::interaction
