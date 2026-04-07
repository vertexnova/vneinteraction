/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/inspect_3d_controller.h"

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/orbital_camera_manipulator.h"

#include "camera_controller_impl.h"

#include <vertexnova/events/key_event.h>
#include <vertexnova/logging/logging.h>

#include <algorithm>
#include <cmath>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.interaction.inspect_3d");
}  // namespace

namespace vne::interaction {

using namespace vne;

namespace {
constexpr float kInspectInteractionScaleMin = 0.01f;
constexpr float kInspectInteractionScaleMax = 100.0f;
constexpr float kInspectZoomSensitivityMin = 0.01f;
constexpr float kInspectInteractionSpeedStepMin = 1.0001f;
}  // namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

class Inspect3DController::Impl {
    friend class Inspect3DController;

   private:
    CameraControllerContext core_;
    std::shared_ptr<OrbitalCameraManipulator> orbit_;

    OrbitalRotationMode rotation_mode_ = OrbitalRotationMode::eTrackball;
    bool rotation_enabled_ = true;
    bool pivot_on_double_click_enabled_ = true;
    bool pan_enabled_ = true;
    bool zoom_enabled_ = true;

    MouseBinding rotate_bind_{MouseButton::eLeft, events::ModifierKey::eModNone};
    MouseBinding pan_primary_{MouseButton::eRight, events::ModifierKey::eModNone};
    MouseBinding pan_secondary_{MouseButton::eMiddle, events::ModifierKey::eModNone};
    events::ModifierKey pan_alt_modifier_ = events::ModifierKey::eModShift;
    events::ModifierKey zoom_scroll_modifier_ = events::ModifierKey::eModNone;
    MouseBinding pivot_double_click_{MouseButton::eLeft, events::ModifierKey::eModNone};

    float user_rotation_speed_ = 0.2f;
    float user_pan_speed_ = 1.0f;
    float user_zoom_speed_ = 1.1f;
    float interaction_scale_ = 1.0f;
    float interaction_speed_step_ = 1.1f;

    events::KeyCode increase_interaction_key_ = events::KeyCode::eUnknown;
    events::KeyCode decrease_interaction_key_ = events::KeyCode::eUnknown;

    void applyOrbitSpeeds() noexcept {
        if (!orbit_) {
            return;
        }
        orbit_->setRotationSpeed(user_rotation_speed_ * interaction_scale_);
        orbit_->setPanSpeed(user_pan_speed_ * interaction_scale_);
        orbit_->setZoomSpeed(user_zoom_speed_);
    }

    void bumpInteractionScale(float mult) noexcept {
        interaction_scale_ =
            std::clamp(interaction_scale_ * mult, kInspectInteractionScaleMin, kInspectInteractionScaleMax);
        applyOrbitSpeeds();
    }
};

// ---------------------------------------------------------------------------
// Rule builder
// ---------------------------------------------------------------------------

namespace {

static InputRule makeMouseButtonRule(
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

static InputRule makeScrollRule(int modifier_mask) {
    InputRule r;
    r.trigger = InputRule::Trigger::eScroll;
    r.modifier_mask = modifier_mask;
    r.on_delta = CameraActionType::eZoomAtCursor;
    return r;
}

static InputRule makeDblClickRule(int button, int mod) {
    InputRule r;
    r.trigger = InputRule::Trigger::eMouseDblClick;
    r.code = button;
    r.modifier_mask = mod;
    r.on_press = CameraActionType::eSetPivotAtCursor;
    return r;
}

static InputRule makeKeyPressRule(int key, int mod, CameraActionType on_press) {
    InputRule r;
    r.trigger = InputRule::Trigger::eKey;
    r.code = key;
    r.modifier_mask = mod;
    r.on_press = on_press;
    r.on_release = CameraActionType::eNone;
    return r;
}

[[nodiscard]] static bool sameBinding(const MouseBinding& a, const MouseBinding& b) noexcept {
    return a.button == b.button && a.modifier_mask == b.modifier_mask;
}

struct InspectRuleConfig {
    bool rotation_enabled = true;
    bool pivot_on_double_click_enabled = true;
    bool pan_enabled = true;
    bool zoom_enabled = true;
    MouseBinding rotate_bind{MouseButton::eLeft, events::ModifierKey::eModNone};
    MouseBinding pan_primary{MouseButton::eRight, events::ModifierKey::eModNone};
    MouseBinding pan_secondary{MouseButton::eMiddle, events::ModifierKey::eModNone};
    events::ModifierKey pan_alt_modifier = events::ModifierKey::eModShift;
    events::ModifierKey zoom_scroll_modifier = events::ModifierKey::eModNone;
    MouseBinding pivot_double_click{MouseButton::eLeft, events::ModifierKey::eModNone};
    events::KeyCode increase_interaction_key = events::KeyCode::eUnknown;
    events::KeyCode decrease_interaction_key = events::KeyCode::eUnknown;
};

static std::vector<InputRule> buildInspectRules(const InspectRuleConfig& cfg) {
    std::vector<InputRule> rules;

    const int rotate_button = static_cast<int>(cfg.rotate_bind.button);

    if (cfg.rotation_enabled) {
        const int rmod = static_cast<int>(cfg.rotate_bind.modifier_mask);
        rules.push_back(makeMouseButtonRule(rotate_button,
                                            rmod,
                                            CameraActionType::eBeginRotate,
                                            CameraActionType::eEndRotate,
                                            CameraActionType::eRotateDelta));
    }

    if (cfg.pan_enabled && cfg.pan_alt_modifier != events::ModifierKey::eModNone) {
        rules.push_back(makeMouseButtonRule(rotate_button,
                                            static_cast<int>(cfg.pan_alt_modifier),
                                            CameraActionType::eBeginPan,
                                            CameraActionType::eEndPan,
                                            CameraActionType::ePanDelta));
    }

    if (cfg.pivot_on_double_click_enabled) {
        rules.push_back(makeDblClickRule(static_cast<int>(cfg.pivot_double_click.button),
                                         static_cast<int>(cfg.pivot_double_click.modifier_mask)));
    }

    if (cfg.pan_enabled) {
        const int p1 = static_cast<int>(cfg.pan_primary.button);
        const int m1 = static_cast<int>(cfg.pan_primary.modifier_mask);
        rules.push_back(makeMouseButtonRule(p1,
                                            m1,
                                            CameraActionType::eBeginPan,
                                            CameraActionType::eEndPan,
                                            CameraActionType::ePanDelta));
        if (!sameBinding(cfg.pan_primary, cfg.pan_secondary)) {
            const int p2 = static_cast<int>(cfg.pan_secondary.button);
            const int m2 = static_cast<int>(cfg.pan_secondary.modifier_mask);
            rules.push_back(makeMouseButtonRule(p2,
                                                m2,
                                                CameraActionType::eBeginPan,
                                                CameraActionType::eEndPan,
                                                CameraActionType::ePanDelta));
        }
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPan,
            .on_delta = CameraActionType::ePanDelta,
        });
    }

    if (cfg.zoom_enabled) {
        rules.push_back(makeScrollRule(static_cast<int>(cfg.zoom_scroll_modifier)));
        rules.push_back({
            .trigger = InputRule::Trigger::eTouchPinch,
            .on_delta = CameraActionType::eZoomAtCursor,
        });
    }

    if (cfg.increase_interaction_key != events::KeyCode::eUnknown) {
        rules.push_back(makeKeyPressRule(static_cast<int>(cfg.increase_interaction_key),
                                         kModNone,
                                         CameraActionType::eIncreaseInteractionSpeed));
    }
    if (cfg.decrease_interaction_key != events::KeyCode::eUnknown) {
        rules.push_back(makeKeyPressRule(static_cast<int>(cfg.decrease_interaction_key),
                                         kModNone,
                                         CameraActionType::eDecreaseInteractionSpeed));
    }

    return rules;
}

}  // namespace

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

Inspect3DController::Inspect3DController()
    : impl_(std::make_unique<Impl>()) {
    impl_->orbit_ = std::make_shared<OrbitalCameraManipulator>();
    impl_->orbit_->setRotationMode(OrbitalRotationMode::eTrackball);
    impl_->core_.rig.addManipulator(impl_->orbit_);

    impl_->user_rotation_speed_ = impl_->orbit_->getRotationSpeed();
    impl_->user_pan_speed_ = impl_->orbit_->getPanSpeed();
    impl_->user_zoom_speed_ = impl_->orbit_->getZoomSpeed();
    impl_->applyOrbitSpeeds();

    impl_->core_.mapper.setActionCallback(
        [impl = impl_.get()](CameraActionType a, const CameraCommandPayload& p, double dt) {
            if (a == CameraActionType::eIncreaseInteractionSpeed) {
                impl->bumpInteractionScale(impl->interaction_speed_step_);
                return;
            }
            if (a == CameraActionType::eDecreaseInteractionSpeed) {
                impl->bumpInteractionScale(1.0f / impl->interaction_speed_step_);
                return;
            }
            impl->core_.rig.onAction(a, p, dt);
        });

    rebuildRules();
}

Inspect3DController::~Inspect3DController() = default;
Inspect3DController::Inspect3DController(Inspect3DController&&) noexcept = default;
Inspect3DController& Inspect3DController::operator=(Inspect3DController&&) noexcept = default;

// ---------------------------------------------------------------------------
// Core setup
// ---------------------------------------------------------------------------

void Inspect3DController::setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept {
    if (!camera) {
        VNE_LOG_DEBUG << "Inspect3DController: camera detached (null camera)";
    }
    impl_->core_.setCamera(std::move(camera));
}

void Inspect3DController::onResize(float w, float h) noexcept {
    impl_->core_.onResize(w, h);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void Inspect3DController::onEvent(const events::Event& event, double delta_time) noexcept {
    switch (event.type()) {
        case events::EventType::eKeyPressed:
        case events::EventType::eKeyRepeat: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->core_.mapper.onKey(static_cast<int>(e.keyCode()), true, delta_time);
            return;
        }
        case events::EventType::eKeyReleased: {
            const auto& e = static_cast<const events::KeyEvent&>(event);
            impl_->core_.mapper.onKey(static_cast<int>(e.keyCode()), false, delta_time);
            return;
        }
        default:
            break;
    }
    dispatchMouseEvents(impl_->core_.mapper, impl_->core_.cursor, event, delta_time);
}

void Inspect3DController::onUpdate(double dt) noexcept {
    impl_->core_.onUpdate(dt);
}

// ---------------------------------------------------------------------------
// Rotation mode
// ---------------------------------------------------------------------------

void Inspect3DController::setRotationMode(OrbitalRotationMode mode) noexcept {
    impl_->rotation_mode_ = mode;
    if (impl_->orbit_) {
        impl_->orbit_->setRotationMode(mode);
    }
}

OrbitalRotationMode Inspect3DController::getRotationMode() const noexcept {
    return impl_->rotation_mode_;
}

// ---------------------------------------------------------------------------
// Pivot
// ---------------------------------------------------------------------------

void Inspect3DController::setPivot(const vne::math::Vec3f& world_pos) noexcept {
    if (impl_->orbit_) {
        impl_->orbit_->setPivot(world_pos, CenterOfInterestSpace::eWorldSpace);
        impl_->orbit_->setPivotMode(OrbitPivotMode::eFixed);
    }
}

void Inspect3DController::setPivotMode(OrbitPivotMode mode) noexcept {
    if (impl_->orbit_) {
        impl_->orbit_->setPivotMode(mode);
    }
}

OrbitPivotMode Inspect3DController::getPivotMode() const noexcept {
    if (impl_->orbit_) {
        return impl_->orbit_->getPivotMode();
    }
    return OrbitPivotMode::eCoi;
}

// ---------------------------------------------------------------------------
// DOF enable/disable
// ---------------------------------------------------------------------------

void Inspect3DController::setRotationEnabled(bool enabled) noexcept {
    if (!enabled && impl_->rotation_enabled_ && impl_->orbit_) {
        const CameraCommandPayload p{};
        impl_->core_.rig.onAction(CameraActionType::eEndRotate, p, 0.0);
    }
    impl_->rotation_enabled_ = enabled;
    if (impl_->orbit_) {
        impl_->orbit_->setRotateEnabled(enabled);
    }
    rebuildRules();
}

bool Inspect3DController::isRotationEnabled() const noexcept {
    return impl_->rotation_enabled_;
}

void Inspect3DController::setPivotOnDoubleClickEnabled(bool enabled) noexcept {
    impl_->pivot_on_double_click_enabled_ = enabled;
    rebuildRules();
}

bool Inspect3DController::isPivotOnDoubleClickEnabled() const noexcept {
    return impl_->pivot_on_double_click_enabled_;
}

void Inspect3DController::setPanEnabled(bool enabled) noexcept {
    if (!enabled && impl_->pan_enabled_ && impl_->orbit_) {
        const CameraCommandPayload p{};
        impl_->core_.rig.onAction(CameraActionType::eEndPan, p, 0.0);
    }
    impl_->pan_enabled_ = enabled;
    if (impl_->orbit_) {
        impl_->orbit_->setPanEnabled(enabled);
    }
    rebuildRules();
}

void Inspect3DController::setZoomEnabled(bool enabled) noexcept {
    impl_->zoom_enabled_ = enabled;
    rebuildRules();
}

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------

void Inspect3DController::setRotateButton(MouseButton btn, events::ModifierKey modifier) noexcept {
    impl_->rotate_bind_ = MouseBinding{btn, modifier};
    rebuildRules();
}

void Inspect3DController::setPanButton(MouseButton btn, events::ModifierKey modifier) noexcept {
    impl_->pan_primary_ = MouseBinding{btn, modifier};
    rebuildRules();
}

void Inspect3DController::setPanSecondaryButton(MouseButton btn, events::ModifierKey modifier) noexcept {
    impl_->pan_secondary_ = MouseBinding{btn, modifier};
    rebuildRules();
}

void Inspect3DController::setPanAlternateModifier(events::ModifierKey modifier) noexcept {
    impl_->pan_alt_modifier_ = modifier;
    rebuildRules();
}

void Inspect3DController::setZoomScrollModifier(events::ModifierKey modifier) noexcept {
    impl_->zoom_scroll_modifier_ = modifier;
    rebuildRules();
}

void Inspect3DController::setPivotDoubleClickButton(MouseButton btn, events::ModifierKey modifier) noexcept {
    impl_->pivot_double_click_ = MouseBinding{btn, modifier};
    rebuildRules();
}

// ---------------------------------------------------------------------------
// Sensitivity
// ---------------------------------------------------------------------------

void Inspect3DController::setRotateSensitivity(float degrees_per_pixel) noexcept {
    impl_->user_rotation_speed_ = std::max(0.0f, degrees_per_pixel);
    impl_->applyOrbitSpeeds();
}

void Inspect3DController::setPanSensitivity(float multiplier) noexcept {
    impl_->user_pan_speed_ = std::max(0.0f, multiplier);
    impl_->applyOrbitSpeeds();
}

void Inspect3DController::setZoomSensitivity(float multiplier) noexcept {
    impl_->user_zoom_speed_ = std::max(kInspectZoomSensitivityMin, multiplier);
    impl_->applyOrbitSpeeds();
}

// ---------------------------------------------------------------------------
// Inertia
// ---------------------------------------------------------------------------

void Inspect3DController::setRotationInertiaEnabled(bool enabled) noexcept {
    if (impl_->orbit_) {
        impl_->orbit_->setRotationInertiaEnabled(enabled);
    }
}

void Inspect3DController::setPanInertiaEnabled(bool enabled) noexcept {
    if (impl_->orbit_) {
        impl_->orbit_->setPanInertiaEnabled(enabled);
    }
}

// -------------------------------------------------------------------------
// Interaction speed keys
// -------------------------------------------------------------------------

void Inspect3DController::setIncreaseInteractionSpeedKey(events::KeyCode key) noexcept {
    impl_->increase_interaction_key_ = key;
    rebuildRules();
}

void Inspect3DController::setDecreaseInteractionSpeedKey(events::KeyCode key) noexcept {
    impl_->decrease_interaction_key_ = key;
    rebuildRules();
}

void Inspect3DController::setInteractionSpeedStep(float factor) noexcept {
    impl_->interaction_speed_step_ = std::max(kInspectInteractionSpeedStepMin, factor);
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

void Inspect3DController::fitToAABB(const vne::math::Vec3f& mn, const vne::math::Vec3f& mx) noexcept {
    if (impl_->orbit_) {
        impl_->orbit_->fitToAABB(mn, mx);
    }
}

void Inspect3DController::reset() noexcept {
    impl_->core_.resetRigAndInteraction();
    impl_->interaction_scale_ = 1.0f;
    impl_->applyOrbitSpeeds();
}

// ---------------------------------------------------------------------------
// Escape hatches
// ---------------------------------------------------------------------------

InputMapper& Inspect3DController::inputMapper() noexcept {
    return impl_->core_.mapper;
}

OrbitalCameraManipulator& Inspect3DController::orbitalCameraManipulator() noexcept {
    return *impl_->orbit_;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void Inspect3DController::rebuildRules() noexcept {
    InspectRuleConfig cfg;
    cfg.rotation_enabled = impl_->rotation_enabled_;
    cfg.pivot_on_double_click_enabled = impl_->pivot_on_double_click_enabled_;
    cfg.pan_enabled = impl_->pan_enabled_;
    cfg.zoom_enabled = impl_->zoom_enabled_;
    cfg.rotate_bind = impl_->rotate_bind_;
    cfg.pan_primary = impl_->pan_primary_;
    cfg.pan_secondary = impl_->pan_secondary_;
    cfg.pan_alt_modifier = impl_->pan_alt_modifier_;
    cfg.zoom_scroll_modifier = impl_->zoom_scroll_modifier_;
    cfg.pivot_double_click = impl_->pivot_double_click_;
    cfg.increase_interaction_key = impl_->increase_interaction_key_;
    cfg.decrease_interaction_key = impl_->decrease_interaction_key_;

    // setRules clears mapper active-button/key tracking; a drag in progress would never get a matching
    // release rule. Unwind orbit gestures so inertia/onUpdate is not stuck behind latched rotate/pan.
    const CameraCommandPayload empty{};
    impl_->core_.rig.onAction(CameraActionType::eEndRotate, empty, 0.0);
    impl_->core_.rig.onAction(CameraActionType::eEndPan, empty, 0.0);

    impl_->core_.mapper.setRules(buildInspectRules(cfg));
}

}  // namespace vne::interaction
