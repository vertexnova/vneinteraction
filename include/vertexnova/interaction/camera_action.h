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
    eNone = 0,               //!< Sentinel: no action for this event phase (used in InputRule)
    eBeginRotate = 1,        //!< Begin rotate the camera by like Left mouse button or touch rotate
    eRotateDelta = 2,        //!< Rotate the camera by like Left mouse button or touch rotate
    eEndRotate = 3,          //!< End rotate the camera by like Left mouse button or touch rotate
    eBeginPan = 4,           //!< Begin pan the camera by like Middle mouse button or touch pan
    ePanDelta = 5,           //!< Pan the camera by like Middle mouse button or touch pan
    eEndPan = 6,             //!< End pan the camera by like Middle mouse button or touch pan
    eZoomAtCursor = 7,       //!< Zoom the camera at the cursor by like scroll wheel or touch pinch
    eLookDelta = 8,          //!< Look the camera by like Right mouse button or touch pan
    eBeginLook = 9,          //!< Begin look the camera by like Right mouse button or touch pan
    eEndLook = 10,           //!< End look the camera by like Right mouse button or touch pan
    eMoveForward = 11,       //!< Move forward the camera by like W key or mouse wheel
    eMoveBackward = 12,      //!< Move backward the camera by like S key or mouse wheel
    eMoveLeft = 13,          //!< Move left the camera by like A key or mouse wheel
    eMoveRight = 14,         //!< Move right the camera by like D key or mouse wheel
    eMoveUp = 15,            //!< Move up the camera by like E key or mouse wheel
    eMoveDown = 16,          //!< Move down the camera by like Q key or mouse wheel
    eSprintModifier = 17,    //!< Sprint the movement speed by like Shift key or mouse wheel
    eSlowModifier = 18,      //!< Slow the movement speed by like Ctrl key or mouse wheel
    eOrbitPanModifier = 19,  //!< Orbit: Shift+LMB pan alias; navigation: sprint-like where mapped.
    eResetView = 20,         //!< Reset the view of the camera
    eSetPivotAtCursor =
        21,  //!< Double-click: set COI along view direction (camera + front * orbit distance); payload x/y ignored
    eIncreaseMoveSpeed = 22,         //!< Increase the movement speed by like Shift key or mouse wheel
    eDecreaseMoveSpeed = 23,         //!< Decrease the movement speed by like Ctrl key or mouse wheel
    eIncreaseInteractionSpeed = 24,  //!< Increase the interaction speed by like Shift key or mouse wheel
    eDecreaseInteractionSpeed = 25,  //!< Decrease the interaction speed by like Ctrl key or mouse wheel
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
