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
#include <bit>
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

InputMapper::InputMapper() {
    resetState();
}

void InputMapper::setRules(std::span<const InputRule> rules) {
    rules_.assign(rules.begin(), rules.end());
    resetState();
}

void InputMapper::addRule(InputRule rule) {
    rules_.push_back(rule);
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

/** Bits allowed in @ref InputRule::modifier_mask (Shift/Ctrl/Alt). */
constexpr unsigned kKnownModifierBits = static_cast<unsigned>(kModShift | kModCtrl | kModAlt);

/**
 * @brief Specificity score for choosing among rules that all satisfy @ref InputMapper::modifiersMatch.
 *
 * Higher score = stricter chord (more modifiers required). Tie-break uses lower rule index.
 * @note Masks must be non-negative and use only @c kKnownModifierBits; asserted in debug builds.
 */
[[nodiscard]] int modifierMaskSpecificity(int mask) noexcept {
    assert(mask >= 0);
    const auto raw = static_cast<unsigned>(mask);
    assert((raw & ~kKnownModifierBits) == 0U);
    return std::popcount(raw & kKnownModifierBits);
}

/**
 * @brief Index of the best matching rule, or @c -1 if @p pred matches none.
 *
 * Among matches, prefers the highest @ref modifierMaskSpecificity; on a tie, the smallest index wins.
 */
template<typename Pred>
[[nodiscard]] int pickBestRuleIndexByModifierSpecificity(const std::vector<InputRule>& rules, Pred&& pred) noexcept {
    int best_i = -1;
    int best_score = -1;
    const int n = static_cast<int>(rules.size());
    for (int i = 0; i < n; ++i) {
        const auto& r = rules[static_cast<std::size_t>(i)];
        if (!pred(r, i)) {
            continue;
        }
        const int score = modifierMaskSpecificity(r.modifier_mask);
        if (best_i < 0 || score > best_score || (score == best_score && i < best_i)) {
            best_i = i;
            best_score = score;
        }
    }
    return best_i;
}

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
    std::fill(std::begin(active_key_rule_), std::end(active_key_rule_), -1);
    modifiers_ = 0;
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
    return (modifiers_ & mask) == mask;
}

void InputMapper::onMouseButton(int button, bool pressed, float x, float y, double dt) noexcept {
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;
    payload.pressed = pressed;

    if (pressed) {
        // Among all matching button rules, pick the most specific modifier chord (e.g. Shift+LMB over LMB).
        const int i = pickBestRuleIndexByModifierSpecificity(rules_, [this, button](const InputRule& r, int) {
            return r.trigger == InputRule::Trigger::eMouseButton && r.code == button && modifiersMatch(r.modifier_mask);
        });
        if (i >= 0) {
            const auto& r = rules_[static_cast<std::size_t>(i)];
            if (button >= 0 && button < kMaxButtons) {
                active_button_rule_[button] = i;
            }
            emit(r.on_press, payload, dt);
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

    const int i = pickBestRuleIndexByModifierSpecificity(rules_, [this, button](const InputRule& r, int) {
        return r.trigger == InputRule::Trigger::eMouseDblClick && r.code == button && modifiersMatch(r.modifier_mask);
    });
    if (i >= 0) {
        emit(rules_[static_cast<std::size_t>(i)].on_press, payload, dt);
    }
}

void InputMapper::onMouseMove(float x, float y, float dx, float dy, double dt) noexcept {
    // Emit on_delta for all currently active button rules
    CameraCommandPayload payload;
    payload.x_px = x;
    payload.y_px = y;
    payload.delta_x_px = dx;
    payload.delta_y_px = dy;

    for (const int idx : active_button_rule_) {
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

    const int i = pickBestRuleIndexByModifierSpecificity(rules_, [this](const InputRule& r, int) {
        return r.trigger == InputRule::Trigger::eScroll && modifiersMatch(r.modifier_mask);
    });
    if (i >= 0) {
        emit(rules_[static_cast<std::size_t>(i)].on_delta, payload, dt);
    }
}

void InputMapper::onKey(int key, bool pressed, double dt) noexcept {
    if (key < 0 || key >= kMaxKeys) {
        VNE_LOG_WARN << "InputMapper: key code " << key << " out of range [0, " << kMaxKeys << ")";
        return;
    }

    // Update modifier bitmask
    auto set_mod_bit = [&](int bit) {
        if (pressed)
            modifiers_ |= bit;
        else
            modifiers_ &= ~bit;
    };
    if (key == static_cast<int>(events::KeyCode::eLeftShift) || key == static_cast<int>(events::KeyCode::eRightShift)) {
        set_mod_bit(kModShift);
    }
    if (key == static_cast<int>(events::KeyCode::eLeftControl)
        || key == static_cast<int>(events::KeyCode::eRightControl)) {
        set_mod_bit(kModCtrl);
    }
    if (key == static_cast<int>(events::KeyCode::eLeftAlt) || key == static_cast<int>(events::KeyCode::eRightAlt)) {
        set_mod_bit(kModAlt);
    }

    // Track active key state
    active_key_[key] = pressed;

    CameraCommandPayload payload;
    payload.pressed = pressed;

    if (pressed) {
        active_key_rule_[key] = -1;
        const int i = pickBestRuleIndexByModifierSpecificity(rules_, [this, key](const InputRule& r, int) {
            return r.trigger == InputRule::Trigger::eKey && r.code == key && modifiersMatch(r.modifier_mask);
        });
        if (i >= 0) {
            const auto& r = rules_[static_cast<std::size_t>(i)];
            active_key_rule_[key] = i;
            emit(r.on_press, payload, dt);
        }
    } else {
        const int idx = active_key_rule_[key];
        active_key_rule_[key] = -1;
        if (idx >= 0 && idx < static_cast<int>(rules_.size())) {
            const auto& r = rules_[static_cast<std::size_t>(idx)];
            if (r.trigger == InputRule::Trigger::eKey && r.code == key) {
                emit(r.on_release, payload, dt);
            }
        }
    }
}

void InputMapper::onTouchPan(const TouchPan& pan, double dt) noexcept {
    CameraCommandPayload payload;
    payload.delta_x_px = pan.delta_x_px;
    payload.delta_y_px = pan.delta_y_px;

    const int i = pickBestRuleIndexByModifierSpecificity(rules_, [this](const InputRule& r, int) {
        return r.trigger == InputRule::Trigger::eTouchPan && modifiersMatch(r.modifier_mask);
    });
    if (i >= 0) {
        emit(rules_[static_cast<std::size_t>(i)].on_delta, payload, dt);
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

    const int i = pickBestRuleIndexByModifierSpecificity(rules_, [this](const InputRule& r, int) {
        return r.trigger == InputRule::Trigger::eTouchPinch && modifiersMatch(r.modifier_mask);
    });
    if (i >= 0) {
        emit(rules_[static_cast<std::size_t>(i)].on_delta, payload, dt);
    }
}

// ---------------------------------------------------------------------------
// Preset factories
// ---------------------------------------------------------------------------

std::vector<InputRule> InputMapper::orbitPreset() {
    const int left_button = static_cast<int>(MouseButton::eLeft);
    const int right_button = static_cast<int>(MouseButton::eRight);
    const int middle_button = static_cast<int>(MouseButton::eMiddle);

    return {
        // Shift+LMB: pan (stricter chord than plain LMB)
        makeButtonRule(left_button,
                       kModShift,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // LMB: rotate
        makeButtonRule(left_button,
                       kModNone,
                       CameraActionType::eBeginRotate,
                       CameraActionType::eEndRotate,
                       CameraActionType::eRotateDelta),
        // RMB: pan
        makeButtonRule(right_button,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // MMB: pan
        makeButtonRule(middle_button,
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
        makeDblClickRule(left_button, CameraActionType::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::fpsPreset() {
    const int right_button = static_cast<int>(MouseButton::eRight);

    return {
        // RMB: look (mouse look while held)
        makeButtonRule(right_button,
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
    const int left_button = static_cast<int>(MouseButton::eLeft);
    const int right_button = static_cast<int>(MouseButton::eRight);

    return {
        // LMB: orbit rotate
        makeButtonRule(left_button,
                       kModNone,
                       CameraActionType::eBeginRotate,
                       CameraActionType::eEndRotate,
                       CameraActionType::eRotateDelta),
        // RMB: free look
        makeButtonRule(right_button,
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
        makeDblClickRule(left_button, CameraActionType::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::cadPreset() {
    const int middle_button = static_cast<int>(MouseButton::eMiddle);

    return {
        // Shift+MMB: rotate (wins over plain MMB when Shift is held)
        makeButtonRule(middle_button,
                       kModShift,
                       CameraActionType::eBeginRotate,
                       CameraActionType::eEndRotate,
                       CameraActionType::eRotateDelta),
        // MMB: pan
        makeButtonRule(middle_button,
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
        // Double-click MMB: eSetPivotAtCursor (COI along view direction)
        makeDblClickRule(middle_button, CameraActionType::eSetPivotAtCursor),
    };
}

std::vector<InputRule> InputMapper::orthoPreset() {
    const int right_button = static_cast<int>(MouseButton::eRight);
    const int middle_button = static_cast<int>(MouseButton::eMiddle);

    return {
        // RMB: pan
        makeButtonRule(right_button,
                       kModNone,
                       CameraActionType::eBeginPan,
                       CameraActionType::eEndPan,
                       CameraActionType::ePanDelta),
        // MMB: pan
        makeButtonRule(middle_button,
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
