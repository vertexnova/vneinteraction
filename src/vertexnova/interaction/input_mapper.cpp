/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/input_mapper.h"

#include <vertexnova/events/types.h>
#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

namespace vne::interaction {

using namespace vne;

namespace {
/**
 * Wheel / trackpad scroll → multiplicative `zoom_factor` in onMouseScroll.
 * factor = exp(-(-ln(kWheelZoomFactorPerLine)) * dy) = kWheelZoomFactorPerLine^dy (dy in "line" units).
 * Small |dy| → small steps (smooth trackpads); dy ≈ ±1 matches legacy discrete ±10% step.
 */
constexpr float kWheelZoomFactorPerLine = 0.9f;
constexpr float kWheelScrollYAbsMax = 25.0f;
constexpr float kWheelZoomFactorMin = 0.5f;
constexpr float kWheelZoomFactorMax = 2.0f;
}  // namespace

// ---------------------------------------------------------------------------
// Construction helpers
// ---------------------------------------------------------------------------

static InputRule makeButtonRule(
    int button, int mod, CameraActionType press, CameraActionType release, CameraActionType delta) {
    InputRule r;
    r.trigger = InputRule::Trigger::eMouseButton;
    r.code = button;
    r.modifier_mask = mod;
    r.on_press = press;
    r.on_release = release;
    r.on_delta = delta;
    return r;
}

static InputRule makeKeyRule(int key, CameraActionType press, CameraActionType release) {
    InputRule r;
    r.trigger = InputRule::Trigger::eKey;
    r.code = key;
    r.on_press = press;
    r.on_release = release;
    r.on_delta = CameraActionType::eNone;
    return r;
}

static InputRule makeScrollRule(CameraActionType delta, int modifier = kModNone) {
    InputRule r;
    r.trigger = InputRule::Trigger::eScroll;
    r.modifier_mask = modifier;
    r.on_delta = delta;
    return r;
}

static InputRule makeTouchPanRule(CameraActionType delta) {
    InputRule r;
    r.trigger = InputRule::Trigger::eTouchPan;
    r.on_delta = delta;
    return r;
}

static InputRule makeTouchPinchRule(CameraActionType delta) {
    InputRule r;
    r.trigger = InputRule::Trigger::eTouchPinch;
    r.on_delta = delta;
    return r;
}

static InputRule makeDblClickRule(int button, CameraActionType action) {
    InputRule r;
    r.trigger = InputRule::Trigger::eMouseDblClick;
    r.code = button;
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

// ---------------------------------------------------------------------------
// Gesture binding API
// ---------------------------------------------------------------------------

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.input_mapper");

bool isRotateRule(const InputRule& r) {
    return r.trigger == InputRule::Trigger::eMouseButton && r.on_press == CameraActionType::eBeginRotate
           && r.on_release == CameraActionType::eEndRotate && r.on_delta == CameraActionType::eRotateDelta;
}
bool isPanRule(const InputRule& r) {
    return r.trigger == InputRule::Trigger::eMouseButton && r.on_press == CameraActionType::eBeginPan
           && r.on_release == CameraActionType::eEndPan && r.on_delta == CameraActionType::ePanDelta;
}
bool isLookRule(const InputRule& r) {
    return r.trigger == InputRule::Trigger::eMouseButton && r.on_press == CameraActionType::eBeginLook
           && r.on_release == CameraActionType::eEndLook && r.on_delta == CameraActionType::eLookDelta;
}
bool isSetPivotRule(const InputRule& r) {
    return r.trigger == InputRule::Trigger::eMouseDblClick && r.on_press == CameraActionType::eSetPivotAtCursor;
}
bool isZoomScrollRule(const InputRule& r) {
    return r.trigger == InputRule::Trigger::eScroll && r.on_delta == CameraActionType::eZoomAtCursor;
}

template<typename Pred>
void eraseRules(std::vector<InputRule>& rules, Pred pred) {
    rules.erase(std::remove_if(rules.begin(), rules.end(), pred), rules.end());
}

void eraseButtonRuleAt(std::vector<InputRule>& rules, int button, int modifier) {
    eraseRules(rules, [button, modifier](const InputRule& r) {
        return r.trigger == InputRule::Trigger::eMouseButton && r.code == button && r.modifier_mask == modifier;
    });
}

}  // namespace

void InputMapper::bindGesture(GestureAction action, MouseBinding binding) {
    const int btn = static_cast<int>(binding.button);
    const int mod = static_cast<int>(binding.modifier_mask);

    switch (action) {
        case GestureAction::eRotate:
            eraseRules(rules_, isRotateRule);
            eraseButtonRuleAt(rules_, btn, mod);  // clear slot for new binding
            rules_.push_back(makeButtonRule(btn,
                                            mod,
                                            CameraActionType::eBeginRotate,
                                            CameraActionType::eEndRotate,
                                            CameraActionType::eRotateDelta));
            break;
        case GestureAction::ePan:
            eraseRules(rules_, isPanRule);
            eraseButtonRuleAt(rules_, btn, mod);
            rules_.push_back(makeButtonRule(btn,
                                            mod,
                                            CameraActionType::eBeginPan,
                                            CameraActionType::eEndPan,
                                            CameraActionType::ePanDelta));
            break;
        case GestureAction::eLook:
            eraseRules(rules_, isLookRule);
            eraseButtonRuleAt(rules_, btn, mod);
            rules_.push_back(makeButtonRule(btn,
                                            mod,
                                            CameraActionType::eBeginLook,
                                            CameraActionType::eEndLook,
                                            CameraActionType::eLookDelta));
            break;
        case GestureAction::eZoom:
        case GestureAction::eSetPivot:
            // bindGesture only handles button+drag; use bindScroll/bindDoubleClick for those
            break;
    }
    resetState();
}

void InputMapper::bindScroll(GestureAction action, vne::events::ModifierKey modifier) {
    if (action != GestureAction::eZoom) {
        return;
    }
    eraseRules(rules_, isZoomScrollRule);
    rules_.push_back(makeScrollRule(CameraActionType::eZoomAtCursor, static_cast<int>(modifier)));
    resetState();
}

void InputMapper::bindDoubleClick(GestureAction action, MouseButton button, vne::events::ModifierKey modifier) {
    if (action != GestureAction::eSetPivot) {
        return;
    }
    eraseRules(rules_, isSetPivotRule);
    rules_.push_back(makeDblClickRule(static_cast<int>(button), CameraActionType::eSetPivotAtCursor));
    if (!rules_.empty()) {
        rules_.back().modifier_mask = static_cast<int>(modifier);
    }
    resetState();
}

void InputMapper::bindKey(CameraActionType press_action,
                          CameraActionType release_action,
                          vne::events::KeyCode key,
                          vne::events::ModifierKey modifier) {
    unbindKey(press_action);
    InputRule r;
    r.trigger = InputRule::Trigger::eKey;
    r.code = static_cast<int>(key);
    r.modifier_mask = static_cast<int>(modifier);
    r.on_press = press_action;
    r.on_release = release_action;
    r.on_delta = CameraActionType::eNone;
    rules_.push_back(r);
    resetState();
}

void InputMapper::unbindKey(CameraActionType press_action) {
    eraseRules(rules_, [press_action](const InputRule& r) {
        return r.trigger == InputRule::Trigger::eKey && r.on_press == press_action;
    });
    resetState();
}

void InputMapper::unbindGesture(GestureAction action) {
    switch (action) {
        case GestureAction::eRotate:
            eraseRules(rules_, isRotateRule);
            break;
        case GestureAction::ePan:
            eraseRules(rules_, isPanRule);
            break;
        case GestureAction::eLook:
            eraseRules(rules_, isLookRule);
            break;
        case GestureAction::eZoom:
            eraseRules(rules_, isZoomScrollRule);
            break;
        case GestureAction::eSetPivot:
            eraseRules(rules_, isSetPivotRule);
            break;
    }
    resetState();
}

void InputMapper::resetState() noexcept {
    std::fill(std::begin(active_button_rule_), std::end(active_button_rule_), -1);
    std::fill(std::begin(active_key_), std::end(active_key_), false);
    mod_shift_ = false;
    mod_ctrl_ = false;
    mod_alt_ = false;
}

void InputMapper::emit(CameraActionType action, const CameraCommandPayload& payload, double dt) noexcept {
    if (action == CameraActionType::eNone) {
        return;  // sentinel = no-op
    }
    if (callback_) {
        callback_(action, payload, dt);
    } else {
        VNE_LOG_DEBUG << "InputMapper: action emitted but no callback registered";
    }
}

bool InputMapper::modifiersMatch(int mask) const noexcept {
    const int current = (mod_shift_ ? kModShift : 0) | (mod_ctrl_ ? kModCtrl : 0) | (mod_alt_ ? kModAlt : 0);
    return (current & mask) == mask;
}

void InputMapper::onMouseButton(int button, bool pressed, float x, float y, double dt) noexcept {
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;
    payload.pressed = pressed;

    if (pressed) {
        // Find the first matching button rule with the right modifier
        for (int i = 0; i < static_cast<int>(rules_.size()); ++i) {
            const auto& r = rules_[static_cast<std::size_t>(i)];
            if (r.trigger != InputRule::Trigger::eMouseButton) {
                continue;
            }
            if (r.code != button) {
                continue;
            }
            if (!modifiersMatch(r.modifier_mask)) {
                continue;
            }
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
        if (r.trigger != InputRule::Trigger::eMouseDblClick) {
            continue;
        }
        if (r.code != button) {
            continue;
        }
        if (!modifiersMatch(r.modifier_mask)) {
            continue;
        }
        emit(r.on_press, payload, dt);
        break;
    }
}

void InputMapper::onMouseMove(float x, float y, float dx, float dy, double dt) noexcept {
    // Emit on_delta for all currently active button rules
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;
    payload.delta_x_px = dx;
    payload.delta_y_px = dy;

    for (int btn = 0; btn < kMaxButtons; ++btn) {
        const int idx = active_button_rule_[btn];
        if (idx < 0) {
            continue;
        }
        const auto& r = rules_[static_cast<size_t>(idx)];
        emit(r.on_delta, payload, dt);
    }
}

void InputMapper::onMouseScroll(float /*scroll_x*/, float scroll_y, float mouse_x, float mouse_y, double dt) noexcept {
    if (scroll_y == 0.0f) {
        return;
    }
    CameraCommandPayload payload;
    payload.x_px = mouse_x;
    payload.y_px = mouse_y;
    const float clamped_dy = std::clamp(scroll_y, -kWheelScrollYAbsMax, kWheelScrollYAbsMax);
    float factor = std::pow(kWheelZoomFactorPerLine, clamped_dy);
    factor = std::clamp(factor, kWheelZoomFactorMin, kWheelZoomFactorMax);
    payload.zoom_factor = factor;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eScroll) {
            continue;
        }
        if (!modifiersMatch(r.modifier_mask)) {
            continue;
        }
        emit(r.on_delta, payload, dt);
        break;
    }
}

void InputMapper::onKey(int key, bool pressed, double dt) noexcept {
    if (key < 0 || key >= kMaxKeys) {
        VNE_LOG_WARN << "InputMapper: key code " << key << " out of range [0, " << kMaxKeys << ")";
        return;
    }

    // Update modifier state
    if (key == static_cast<int>(events::KeyCode::eLeftShift) || key == static_cast<int>(events::KeyCode::eRightShift)) {
        mod_shift_ = pressed;
    }
    if (key == static_cast<int>(events::KeyCode::eLeftControl)
        || key == static_cast<int>(events::KeyCode::eRightControl)) {
        mod_ctrl_ = pressed;
    }
    if (key == static_cast<int>(events::KeyCode::eLeftAlt) || key == static_cast<int>(events::KeyCode::eRightAlt)) {
        mod_alt_ = pressed;
    }

    // Track active key state
    active_key_[key] = pressed;

    CameraCommandPayload payload;
    payload.pressed = pressed;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eKey) {
            continue;
        }
        if (r.code != key) {
            continue;
        }
        if (!modifiersMatch(r.modifier_mask)) {
            continue;
        }
        emit(pressed ? r.on_press : r.on_release, payload, dt);
        break;
    }
}

void InputMapper::onTouchPan(const TouchPan& pan, double dt) noexcept {
    CameraCommandPayload payload;
    payload.delta_x_px = pan.delta_x_px;
    payload.delta_y_px = pan.delta_y_px;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eTouchPan) {
            continue;
        }
        if (!modifiersMatch(r.modifier_mask)) {
            continue;
        }
        emit(r.on_delta, payload, dt);
        break;
    }
}

void InputMapper::onTouchPinch(const TouchPinch& pinch, double dt) noexcept {
    if (pinch.scale <= 0.0f) {
        return;
    }
    CameraCommandPayload payload;
    payload.x_px = pinch.center_x_px;
    payload.y_px = pinch.center_y_px;
    payload.zoom_factor = 1.0f / pinch.scale;

    for (const auto& r : rules_) {
        if (r.trigger != InputRule::Trigger::eTouchPinch) {
            continue;
        }
        if (!modifiersMatch(r.modifier_mask)) {
            continue;
        }
        emit(r.on_delta, payload, dt);
        break;
    }
}

// ---------------------------------------------------------------------------
// Preset factories
// ---------------------------------------------------------------------------

std::vector<InputRule> InputMapper::orbitPreset() {
    const int kLeft = static_cast<int>(MouseButton::eLeft);
    const int kRight = static_cast<int>(MouseButton::eRight);
    const int kMiddle = static_cast<int>(MouseButton::eMiddle);

    return {
        // LMB: rotate
        makeButtonRule(kLeft,
                       kModNone,
                       CameraActionType::eBeginRotate,
                       CameraActionType::eEndRotate,
                       CameraActionType::eRotateDelta),
        // Shift+LMB: pan alias
        makeButtonRule(kLeft,
                       kModShift,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // RMB: pan
        makeButtonRule(kRight,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // MMB: pan
        makeButtonRule(kMiddle,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // Scroll: zoom
        makeScrollRule(CameraActionType::eZoomAtCursor),
        // Touch pan: rotate
        makeTouchPanRule(CameraActionType::eRotateDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(CameraActionType::eZoomAtCursor),
        // Double-click LMB: eSetPivotAtCursor (COI along view direction in OrbitalCameraManipulator)
        makeDblClickRule(kLeft, CameraActionType::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::fpsPreset() {
    const int kRight = static_cast<int>(MouseButton::eRight);

    return {
        // RMB: look (mouse look while held)
        makeButtonRule(kRight,
                       kModNone,
                       CameraActionType::eBeginLook,
                       CameraActionType::eEndLook,
                       CameraActionType::eLookDelta),
        // WASD + QE
        makeKeyRule(static_cast<int>(events::KeyCode::eW),
                    CameraActionType::eMoveForward,
                    CameraActionType::eMoveForward),
        makeKeyRule(static_cast<int>(events::KeyCode::eS),
                    CameraActionType::eMoveBackward,
                    CameraActionType::eMoveBackward),
        makeKeyRule(static_cast<int>(events::KeyCode::eA), CameraActionType::eMoveLeft, CameraActionType::eMoveLeft),
        makeKeyRule(static_cast<int>(events::KeyCode::eD), CameraActionType::eMoveRight, CameraActionType::eMoveRight),
        makeKeyRule(static_cast<int>(events::KeyCode::eE), CameraActionType::eMoveUp, CameraActionType::eMoveUp),
        makeKeyRule(static_cast<int>(events::KeyCode::eQ), CameraActionType::eMoveDown, CameraActionType::eMoveDown),
        // Shift: sprint, Ctrl: slow
        makeKeyRule(static_cast<int>(events::KeyCode::eLeftShift),
                    CameraActionType::eSprintModifier,
                    CameraActionType::eSprintModifier),
        makeKeyRule(static_cast<int>(events::KeyCode::eRightShift),
                    CameraActionType::eSprintModifier,
                    CameraActionType::eSprintModifier),
        makeKeyRule(static_cast<int>(events::KeyCode::eLeftControl),
                    CameraActionType::eSlowModifier,
                    CameraActionType::eSlowModifier),
        makeKeyRule(static_cast<int>(events::KeyCode::eRightControl),
                    CameraActionType::eSlowModifier,
                    CameraActionType::eSlowModifier),
        // Scroll: zoom
        makeScrollRule(CameraActionType::eZoomAtCursor),
        // Touch pan: look
        makeTouchPanRule(CameraActionType::eLookDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(CameraActionType::eZoomAtCursor),
    };
}

std::vector<InputRule> InputMapper::gamePreset() {
    const int kLeft = static_cast<int>(MouseButton::eLeft);
    const int kRight = static_cast<int>(MouseButton::eRight);

    return {
        // LMB: orbit rotate
        makeButtonRule(kLeft,
                       kModNone,
                       CameraActionType::eBeginRotate,
                       CameraActionType::eEndRotate,
                       CameraActionType::eRotateDelta),
        // RMB: free look
        makeButtonRule(kRight,
                       kModNone,
                       CameraActionType::eBeginLook,
                       CameraActionType::eEndLook,
                       CameraActionType::eLookDelta),
        // WASD + QE
        makeKeyRule(static_cast<int>(events::KeyCode::eW),
                    CameraActionType::eMoveForward,
                    CameraActionType::eMoveForward),
        makeKeyRule(static_cast<int>(events::KeyCode::eS),
                    CameraActionType::eMoveBackward,
                    CameraActionType::eMoveBackward),
        makeKeyRule(static_cast<int>(events::KeyCode::eA), CameraActionType::eMoveLeft, CameraActionType::eMoveLeft),
        makeKeyRule(static_cast<int>(events::KeyCode::eD), CameraActionType::eMoveRight, CameraActionType::eMoveRight),
        makeKeyRule(static_cast<int>(events::KeyCode::eE), CameraActionType::eMoveUp, CameraActionType::eMoveUp),
        makeKeyRule(static_cast<int>(events::KeyCode::eQ), CameraActionType::eMoveDown, CameraActionType::eMoveDown),
        // Shift: sprint
        makeKeyRule(static_cast<int>(events::KeyCode::eLeftShift),
                    CameraActionType::eSprintModifier,
                    CameraActionType::eSprintModifier),
        makeKeyRule(static_cast<int>(events::KeyCode::eRightShift),
                    CameraActionType::eSprintModifier,
                    CameraActionType::eSprintModifier),
        // Scroll: zoom
        makeScrollRule(CameraActionType::eZoomAtCursor),
        // Touch pan: rotate
        makeTouchPanRule(CameraActionType::eRotateDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(CameraActionType::eZoomAtCursor),
        // Double-click LMB: eSetPivotAtCursor (COI along view direction)
        makeDblClickRule(kLeft, CameraActionType::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::cadPreset() {
    const int kMiddle = static_cast<int>(MouseButton::eMiddle);

    return {
        // MMB: pan
        makeButtonRule(kMiddle,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // Shift+MMB: rotate
        makeButtonRule(kMiddle,
                       kModShift,
                       CameraActionType::eBeginRotate,
                       CameraActionType::eEndRotate,
                       CameraActionType::eRotateDelta),
        // Scroll: zoom
        makeScrollRule(CameraActionType::eZoomAtCursor),
        // Touch pan: pan
        makeTouchPanRule(CameraActionType::ePanDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(CameraActionType::eZoomAtCursor),
        // Double-click MMB: eSetPivotAtCursor (COI along view direction)
        makeDblClickRule(kMiddle, CameraActionType::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::orthoPreset() {
    const int kRight = static_cast<int>(MouseButton::eRight);
    const int kMiddle = static_cast<int>(MouseButton::eMiddle);

    return {
        // RMB: pan
        makeButtonRule(kRight,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // MMB: pan
        makeButtonRule(kMiddle,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // Scroll: zoom
        makeScrollRule(CameraActionType::eZoomAtCursor),
        // Touch pan: pan
        makeTouchPanRule(CameraActionType::ePanDelta),
        // Touch pinch: zoom
        makeTouchPinchRule(CameraActionType::eZoomAtCursor),
        // No rotation rules — DOF gating via omission
    };
}

}  // namespace vne::interaction
