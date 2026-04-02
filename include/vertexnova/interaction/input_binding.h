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
 * @file input_binding.h
 * @brief Input configuration types: @ref InputRule, @ref MouseBinding, @ref KeyBinding,
 *        @ref TouchPan, @ref TouchPinch, and modifier bitmask constants.
 *
 * These types are used to configure @ref InputMapper rules and high-level controller bindings.
 * Include this header when configuring input without needing the full interaction_types.h umbrella.
 */

#include "vertexnova/interaction/camera_action.h"
#include "vertexnova/interaction/export.h"

#include <vertexnova/events/types.h>

#include <cstdint>

namespace vne::interaction {

//! Mouse button: use vne::events::MouseButton (eLeft, eRight, eMiddle) for compatibility.
using MouseButton = vne::events::MouseButton;

//! Modifier key bitmask constants for InputRule::modifier_mask.
static constexpr int kModNone = 0;        //!< No modifier key held
static constexpr int kModShift = 1 << 0;  //!< Shift key held
static constexpr int kModCtrl = 1 << 1;   //!< Ctrl key held
static constexpr int kModAlt = 1 << 2;    //!< Alt key held

/**
 * @brief Mouse button + modifier binding for gesture remapping.
 *
 * Uses vne::events::ModifierKey for modifier_mask (eModNone, eModShift, eModCtrl, eModAlt).
 */
struct VNE_INTERACTION_API MouseBinding {
    MouseButton button = MouseButton::eLeft;  //!< Mouse button for this binding.
    vne::events::ModifierKey modifier_mask =
        vne::events::ModifierKey::eModNone;  //!< Required modifiers; @c eModNone = none.
};

/**
 * @brief Keyboard key + optional modifier for rebindable actions.
 *
 * Used by controllers and @ref InputMapper::bindKey for data-driven key rules.
 */
struct VNE_INTERACTION_API KeyBinding {
    vne::events::KeyCode key = vne::events::KeyCode::eUnknown;  //!< Key for this binding.
    vne::events::ModifierKey modifier_mask =
        vne::events::ModifierKey::eModNone;  //!< Required modifiers; @c eModNone = none.
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

/**
 * @brief A single data-driven input binding rule.
 *
 * Maps one input trigger (button press, key, scroll, touch, double-click) to
 * CameraActionType values that are emitted to the active CameraRig.
 *
 * - For mouse/key triggers: on_press emitted on press, on_release on release,
 *   on_delta emitted each frame that a move delta arrives while the button is held.
 * - For scroll/touch triggers: on_delta emitted per event.
 * - For double-click: on_press emitted once.
 * - Use eNone as a sentinel to mean "no action for this event phase".
 */
struct VNE_INTERACTION_API InputRule {
    /** What kind of input triggers this rule. */
    enum class Trigger : std::uint8_t {
        eMouseButton = 0,    //!< Mouse button; code = MouseButton int (0=left, 1=right, 2=middle)
        eKey = 1,            //!< Keyboard key; code = vne::events::KeyCode:: value (e.g. KeyCode::eW)
        eScroll = 2,         //!< Mouse scroll wheel; code = 0
        eTouchPan = 3,       //!< Touch pan gesture; code = 0
        eTouchPinch = 4,     //!< Touch pinch gesture; code = 0
        eMouseDblClick = 5,  //!< Mouse button double-click; code = MouseButton int
    };

    Trigger trigger = Trigger::eMouseButton;
    int code = 0;  //!< Button index or key code; 0 for scroll/touch
    int modifier_mask =
        kModNone;  //!< Required modifier bitmask (non-negative; only @c kModShift|kModCtrl|kModAlt); 0 = none required

    CameraActionType on_press = CameraActionType::eNone;    //!< Emitted on press / double-click
    CameraActionType on_release = CameraActionType::eNone;  //!< Emitted on release
    CameraActionType on_delta = CameraActionType::eNone;    //!< Emitted per delta/scroll/touch event
};

}  // namespace vne::interaction
