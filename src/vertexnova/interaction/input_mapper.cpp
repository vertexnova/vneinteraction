/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/input_mapper.h"

#include <algorithm>
#include <cstring>

namespace vne::interaction {

// ---------------------------------------------------------------------------
// Construction helpers
// ---------------------------------------------------------------------------

static InputRule makeButtonRule(int button, int mod,
                                CameraActionType press,
                                CameraActionType release,
                                CameraActionType delta) {
    InputRule r;
    r.trigger       = InputRule::Trigger::eMouseButton;
    r.code          = button;
    r.modifier_mask = mod;
    r.on_press      = press;
    r.on_release    = release;
    r.on_delta      = delta;
    return r;
}

static InputRule makeKeyRule(int key,
                             CameraActionType press,
                             CameraActionType release) {
    InputRule r;
    r.trigger    = InputRule::Trigger::eKey;
    r.code       = key;
    r.on_press   = press;
    r.on_release = release;
    r.on_delta   = CameraActionType::eResetView;
    return r;
}

static InputRule makeScrollRule(CameraActionType delta) {
    InputRule r;
    r.trigger  = InputRule::Trigger::eScroll;
    r.on_delta = delta;
    return r;
}

static InputRule makeTouchPanRule(CameraActionType delta) {
    InputRule r;
    r.trigger  = InputRule::Trigger::eTouchPan;
    r.on_delta = delta;
    return r;
}

static InputRule makeTouchPinchRule(CameraActionType delta) {
    InputRule r;
    r.trigger  = InputRule::Trigger::eTouchPinch;
    r.on_delta = delta;
    return r;
}

static InputRule makeDblClickRule(int button, CameraActionType action) {
    InputRule r;
    r.trigger  = InputRule::Trigger::eMouseDblClick;
    r.code     = button;
    r.on_press = action;
    return r;
}

// ---------------------------------------------------------------------------
// InputMapper implementation
// ---------------------------------------------------------------------------

void InputMapper::setRules(std::span<const InputRule> rules) {
    rules_.assign(rules.begin(), rules.end());
    resetState();
}

void InputMapper::addRule(InputRule rule) {
    rules_.push_back(std::move(rule));
}

void InputMapper::clearRules() {
    rules_.clear();
    resetState();
}

void InputMapper::resetState() noexcept {
    std::fill(std::begin(active_button_rule_), std::end(active_button_rule_), -1);
    std::fill(std::begin(active_key_), std::end(active_key_), false);
    mod_shift_ = false;
    mod_ctrl_  = false;
    mod_alt_   = false;
}

void InputMapper::emit(CameraActionType action, const CameraCommandPayload& payload, double dt) noexcept {
    if (action == CameraActionType::eResetView) {
        return;  // sentinel = no-op
    }
    if (callback_) {
        callback_(action, payload, dt);
    }
}

bool InputMapper::modifiersMatch(int mask) const noexcept {
    const int current = (mod_shift_ ? kModShift : 0)
                      | (mod_ctrl_  ? kModCtrl  : 0)
                      | (mod_alt_   ? kModAlt   : 0);
    return (current & mask) == mask;
}

void InputMapper::onMouseButton(int button, bool pressed, float x, float y, double dt) noexcept {
    CameraCommandPayload payload;
    payload.x_px    = x;
    payload.y_px    = y;
    payload.pressed = pressed;

    if (pressed) {
        // Find the first matching button rule with the right modifier
        for (int i = 0; i < static_cast<int>(rules_.size()); ++i) {
            const auto& r = rules_[static_cast<std::size_t>(i)];
            if (r.trigger != InputRule::Trigger::eMouseButton) continue;
            if (r.code != button) continue;
            if (!modifiersMatch(r.modifier_mask)) continue;
            // Activate this rule for this button slot
            if (button >= 0 && button < kMaxButtons) {
                active_button_rule_[button] = i;
            }
            emit(r.on_press, payload, dt);
            break;  // only first matching rule fires
        }
    } else {
        // Release: fire on_release for the previously active rule
        if (button >= 0 && button < kMaxButtons && active_button_rule_[button] >= 0) {
            const auto& r = rules_[static_cast<size_t>(active_button_rule_[button])];
            emit(r.on_release, payload, dt);
            active_button_rule_[button] = -1;
        }
    }
}

void InputMapper::onMouseDoubleClick(int button, float x, float y, double dt) noexcept {
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eMouseDblClick) continue;
        if (r.code != button) continue;
        if (!modifiersMatch(r.modifier_mask)) continue;
        emit(r.on_press, payload, dt);
        break;
    }
}

void InputMapper::onMouseMove(float x, float y, float dx, float dy, double dt) noexcept {
    // Emit on_delta for all currently active button rules
    CameraCommandPayload payload;
    payload.x_px       = x;
    payload.y_px       = y;
    payload.delta_x_px = dx;
    payload.delta_y_px = dy;

    for (int btn = 0; btn < kMaxButtons; ++btn) {
        const int idx = active_button_rule_[btn];
        if (idx < 0) continue;
        const auto& r = rules_[static_cast<size_t>(idx)];
        emit(r.on_delta, payload, dt);
    }
}

void InputMapper::onMouseScroll(float /*scroll_x*/, float scroll_y, float mouse_x, float mouse_y, double dt) noexcept {
    if (scroll_y == 0.0f) return;
    CameraCommandPayload payload;
    payload.x_px        = mouse_x;
    payload.y_px        = mouse_y;
    // zoom_factor: scroll up (<1) = zoom in, scroll down (>1) = zoom out
    // The behavior's applyZoom uses this convention directly
    payload.zoom_factor = (scroll_y > 0.0f) ? 0.9f : 1.1f;  // ~10% per notch; behaviors can scale

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eScroll) continue;
        if (!modifiersMatch(r.modifier_mask)) continue;
        emit(r.on_delta, payload, dt);
        break;
    }
}

void InputMapper::onKey(int key, bool pressed, double dt) noexcept {
    // Update modifier state
    constexpr int kShiftL = 340, kShiftR = 344;
    constexpr int kCtrlL  = 341, kCtrlR  = 345;
    constexpr int kAltL   = 342, kAltR   = 346;
    if (key == kShiftL || key == kShiftR) mod_shift_ = pressed;
    if (key == kCtrlL  || key == kCtrlR)  mod_ctrl_  = pressed;
    if (key == kAltL   || key == kAltR)   mod_alt_   = pressed;

    // Track active key state
    if (key >= 0 && key < kMaxKeys) {
        active_key_[key] = pressed;
    }

    CameraCommandPayload payload;
    payload.pressed = pressed;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eKey) continue;
        if (r.code != key) continue;
        if (!modifiersMatch(r.modifier_mask)) continue;
        emit(pressed ? r.on_press : r.on_release, payload, dt);
        break;
    }
}

void InputMapper::onTouchPan(const TouchPan& pan, double dt) noexcept {
    CameraCommandPayload payload;
    payload.delta_x_px = pan.delta_x_px;
    payload.delta_y_px = pan.delta_y_px;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eTouchPan) continue;
        if (!modifiersMatch(r.modifier_mask)) continue;
        emit(r.on_delta, payload, dt);
        break;
    }
}

void InputMapper::onTouchPinch(const TouchPinch& pinch, double dt) noexcept {
    if (pinch.scale <= 0.0f) return;
    CameraCommandPayload payload;
    payload.x_px        = pinch.center_x_px;
    payload.y_px        = pinch.center_y_px;
    payload.zoom_factor = 1.0f / pinch.scale;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eTouchPinch) continue;
        if (!modifiersMatch(r.modifier_mask)) continue;
        emit(r.on_delta, payload, dt);
        break;
    }
}

// ---------------------------------------------------------------------------
// Preset factories
// ---------------------------------------------------------------------------

std::vector<InputRule> InputMapper::orbitPreset() {
    const int kLeft   = static_cast<int>(MouseButton::eLeft);
    const int kRight  = static_cast<int>(MouseButton::eRight);
    const int kMiddle = static_cast<int>(MouseButton::eMiddle);
    using AT = CameraActionType;

    return {
        // LMB: rotate
        makeButtonRule(kLeft,   kModNone,  AT::eBeginRotate, AT::eEndRotate, AT::eRotateDelta),
        // Shift+LMB: pan alias
        makeButtonRule(kLeft,   kModShift, AT::eBeginPan,    AT::eEndPan,    AT::ePanDelta),
        // RMB: pan
        makeButtonRule(kRight,  kModNone,  AT::eBeginPan,    AT::eEndPan,    AT::ePanDelta),
        // MMB: pan
        makeButtonRule(kMiddle, kModNone,  AT::eBeginPan,    AT::eEndPan,    AT::ePanDelta),
        // Scroll: zoom
        makeScrollRule(AT::eZoomAtCursor),
        // Touch pan: rotate
        makeTouchPanRule(AT::eRotateDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(AT::eZoomAtCursor),
        // Double-click LMB: set pivot at cursor
        makeDblClickRule(kLeft, AT::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::fpsPreset() {
    const int kRight = static_cast<int>(MouseButton::eRight);
    using AT = CameraActionType;

    return {
        // RMB: look (mouse look while held)
        makeButtonRule(kRight, kModNone, AT::eBeginLook, AT::eEndLook, AT::eLookDelta),
        // WASD + QE
        makeKeyRule(87, AT::eMoveForward,  AT::eMoveForward),   // W
        makeKeyRule(83, AT::eMoveBackward, AT::eMoveBackward),  // S
        makeKeyRule(65, AT::eMoveLeft,     AT::eMoveLeft),      // A
        makeKeyRule(68, AT::eMoveRight,    AT::eMoveRight),     // D
        makeKeyRule(69, AT::eMoveUp,       AT::eMoveUp),        // E
        makeKeyRule(81, AT::eMoveDown,     AT::eMoveDown),      // Q
        // Shift: sprint, Ctrl: slow
        makeKeyRule(340, AT::eSprintModifier, AT::eSprintModifier),  // Left Shift
        makeKeyRule(344, AT::eSprintModifier, AT::eSprintModifier),  // Right Shift
        makeKeyRule(341, AT::eSlowModifier,   AT::eSlowModifier),    // Left Ctrl
        makeKeyRule(345, AT::eSlowModifier,   AT::eSlowModifier),    // Right Ctrl
        // Scroll: zoom
        makeScrollRule(AT::eZoomAtCursor),
        // Touch pan: look
        makeTouchPanRule(AT::eLookDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(AT::eZoomAtCursor),
    };
}

std::vector<InputRule> InputMapper::gamePreset() {
    const int kLeft  = static_cast<int>(MouseButton::eLeft);
    const int kRight = static_cast<int>(MouseButton::eRight);
    using AT = CameraActionType;

    return {
        // LMB: orbit rotate
        makeButtonRule(kLeft,  kModNone, AT::eBeginRotate, AT::eEndRotate, AT::eRotateDelta),
        // RMB: free look
        makeButtonRule(kRight, kModNone, AT::eBeginLook,   AT::eEndLook,   AT::eLookDelta),
        // WASD + QE
        makeKeyRule(87, AT::eMoveForward,  AT::eMoveForward),
        makeKeyRule(83, AT::eMoveBackward, AT::eMoveBackward),
        makeKeyRule(65, AT::eMoveLeft,     AT::eMoveLeft),
        makeKeyRule(68, AT::eMoveRight,    AT::eMoveRight),
        makeKeyRule(69, AT::eMoveUp,       AT::eMoveUp),
        makeKeyRule(81, AT::eMoveDown,     AT::eMoveDown),
        // Shift: sprint
        makeKeyRule(340, AT::eSprintModifier, AT::eSprintModifier),
        makeKeyRule(344, AT::eSprintModifier, AT::eSprintModifier),
        // Scroll: zoom
        makeScrollRule(AT::eZoomAtCursor),
        // Touch pan: rotate
        makeTouchPanRule(AT::eRotateDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(AT::eZoomAtCursor),
        // Double-click LMB: set pivot
        makeDblClickRule(kLeft, AT::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::cadPreset() {
    const int kMiddle = static_cast<int>(MouseButton::eMiddle);
    using AT = CameraActionType;

    return {
        // MMB: pan
        makeButtonRule(kMiddle, kModNone,  AT::eBeginPan,    AT::eEndPan,    AT::ePanDelta),
        // Shift+MMB: rotate
        makeButtonRule(kMiddle, kModShift, AT::eBeginRotate, AT::eEndRotate, AT::eRotateDelta),
        // Scroll: zoom
        makeScrollRule(AT::eZoomAtCursor),
        // Touch pan: pan
        makeTouchPanRule(AT::ePanDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(AT::eZoomAtCursor),
        // Double-click MMB: set pivot
        makeDblClickRule(kMiddle, AT::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::orthoPreset() {
    const int kRight  = static_cast<int>(MouseButton::eRight);
    const int kMiddle = static_cast<int>(MouseButton::eMiddle);
    using AT = CameraActionType;

    return {
        // RMB: pan
        makeButtonRule(kRight,  kModNone, AT::eBeginPan, AT::eEndPan, AT::ePanDelta),
        // MMB: pan
        makeButtonRule(kMiddle, kModNone, AT::eBeginPan, AT::eEndPan, AT::ePanDelta),
        // Scroll: zoom
        makeScrollRule(AT::eZoomAtCursor),
        // Touch pan: pan
        makeTouchPanRule(AT::ePanDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(AT::eZoomAtCursor),
        // No rotation rules — DOF gating via omission
    };
}

}  // namespace vne::interaction
