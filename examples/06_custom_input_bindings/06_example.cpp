#include "06_example.h"

#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include "common/input_simulation.h"
#include "common/logging_guard.h"

static constexpr double kDt = 1.0 / 60.0;
static constexpr float kVpW = 1280.0f;
static constexpr float kVpH = 720.0f;
static constexpr float kCx = kVpW / 2.0f;
static constexpr float kCy = kVpH / 2.0f;

namespace vne::interaction::examples {

int runCustomInputBindingsExample() {
    vne::interaction::examples::LoggingGuard logging_guard;

    auto camera = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, kVpW / kVpH, 0.1f, 1000.0f));
    camera->setPosition(vne::math::Vec3f(0.0f, 2.0f, 8.0f));
    camera->lookAt(vne::math::Vec3f(0.0f, 0.0f, 0.0f), vne::math::Vec3f(0.0f, 1.0f, 0.0f));

    // ─────────────────────────────────────────────────────────────────────────
    // Section A: Preset survey — inspect rule counts
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- A: Preset rule counts ---";
    VNE_LOG_INFO << "  orbit=" << vne::interaction::InputMapper::orbitPreset().size()
                 << "  fps=" << vne::interaction::InputMapper::fpsPreset().size()
                 << "  game=" << vne::interaction::InputMapper::gamePreset().size()
                 << "  cad=" << vne::interaction::InputMapper::cadPreset().size()
                 << "  ortho=" << vne::interaction::InputMapper::orthoPreset().size();

    // ─────────────────────────────────────────────────────────────────────────
    // Section B: High-level controller binding API
    //   Inspect3DController exposes convenience setters that rebuild rules
    //   without requiring direct InputMapper access.
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- B: Controller binding API (Inspect3DController) ---";
    {
        vne::interaction::Inspect3DController ctrl;
        ctrl.setCamera(camera);
        ctrl.onResize(kVpW, kVpH);

        // Swap to CAD preset: MMB=pan, Shift+MMB=rotate, scroll=zoom
        ctrl.inputMapper().setRules(vne::interaction::InputMapper::cadPreset());
        VNE_LOG_INFO << "  Applied cadPreset; rules=" << ctrl.inputMapper().rules().size();

        // Rebind rotate to Shift+MMB explicitly (already in CAD, shown for clarity)
        ctrl.setRotateButton(vne::events::MouseButton::eMiddle, vne::events::ModifierKey::eModShift);

        // Require Ctrl to zoom (only Ctrl+scroll zooms, plain scroll does nothing)
        ctrl.setZoomScrollModifier(vne::events::ModifierKey::eModCtrl);

        // Disable double-click pivot
        ctrl.setPivotOnDoubleClickEnabled(false);

        auto on_event = [&](const vne::events::Event& e, double dt) { ctrl.onEvent(e, dt); };

        // Simulate Shift+MMB drag → rotate
        vne::events::KeyPressedEvent shift(vne::events::KeyCode::eLeftShift);
        on_event(shift, kDt);
        vne::interaction::examples::simulateMouseDrag(on_event,
                                                      vne::events::MouseButton::eMiddle,
                                                      kCx,
                                                      kCy,
                                                      60.0f,
                                                      20.0f,
                                                      15,
                                                      kDt);
        vne::events::KeyReleasedEvent shift_rel(vne::events::KeyCode::eLeftShift);
        on_event(shift_rel, kDt);
        for (int i = 0; i < 10; ++i)
            ctrl.onUpdate(kDt);
        VNE_LOG_INFO << "  CAD preset: Shift+MMB orbit done";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section C: bindGesture / bindScroll / bindDoubleClick / unbindGesture
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- C: bindGesture, bindScroll, bindDoubleClick ---";
    {
        vne::interaction::InputMapper mapper;
        mapper.setRules(vne::interaction::InputMapper::orbitPreset());
        VNE_LOG_INFO << "  After orbitPreset: rules=" << mapper.rules().size();

        // Move rotate from LMB to MMB
        mapper.bindGesture(vne::interaction::GestureAction::eRotate,
                           {vne::events::MouseButton::eMiddle, vne::events::ModifierKey::eModNone});
        VNE_LOG_INFO << "  After bindGesture(rotate→MMB): rules=" << mapper.rules().size();

        // Move pan from RMB to LMB
        mapper.bindGesture(vne::interaction::GestureAction::ePan,
                           {vne::events::MouseButton::eLeft, vne::events::ModifierKey::eModNone});

        // Zoom only when Alt is held
        mapper.bindScroll(vne::interaction::GestureAction::eZoom, vne::events::ModifierKey::eModAlt);
        VNE_LOG_INFO << "  After Alt-scroll zoom: rules=" << mapper.rules().size();

        // Double-click MMB to set pivot
        mapper.bindDoubleClick(vne::interaction::GestureAction::eSetPivot, vne::events::MouseButton::eMiddle);

        // Remove zoom entirely (pen-tablet workflow with no scroll)
        mapper.unbindGesture(vne::interaction::GestureAction::eZoom);
        VNE_LOG_INFO << "  After unbindGesture(zoom): rules=" << mapper.rules().size();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section D: bindKey / unbindKey
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- D: bindKey / unbindKey ---";
    {
        vne::interaction::InputMapper mapper;
        mapper.setRules(vne::interaction::InputMapper::fpsPreset());

        // Add Space = jump (move up)
        mapper.bindKey(vne::interaction::CameraActionType::eMoveUp,
                       vne::interaction::CameraActionType::eNone,
                       vne::events::KeyCode::eSpace);
        VNE_LOG_INFO << "  Bound Space→eMoveUp; rules=" << mapper.rules().size();

        // Remove the forward key rule
        mapper.unbindKey(vne::interaction::CameraActionType::eMoveForward);
        VNE_LOG_INFO << "  After unbindKey(eMoveForward): rules=" << mapper.rules().size();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section E: Direct InputRule construction — full low-level control
    //
    //   InputRule fields:
    //     trigger      — eMouseButton | eKey | eScroll | eTouchPan | eTouchPinch | eMouseDblClick
    //     code         — button index or KeyCode int; 0 for scroll/touch
    //     modifier_mask — kModNone | kModShift | kModCtrl | kModAlt (bitmask)
    //     on_press     — action emitted on button/key press
    //     on_release   — action emitted on button/key release
    //     on_delta     — action emitted each move/scroll frame
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- E: Direct InputRule construction ---";
    {
        // Build a minimal orbit rule set from scratch:
        //   LMB drag         → rotate
        //   MMB drag         → pan
        //   Scroll           → zoom
        //   Double-click LMB → set pivot
        using R = vne::interaction::InputRule;
        using T = vne::interaction::InputRule::Trigger;
        using A = vne::interaction::CameraActionType;

        const int kLeft = static_cast<int>(vne::events::MouseButton::eLeft);
        const int kMiddle = static_cast<int>(vne::events::MouseButton::eMiddle);

        vne::interaction::InputMapper mapper;
        mapper.clearRules();

        // LMB → orbit
        R orbit_rule;
        orbit_rule.trigger = T::eMouseButton;
        orbit_rule.code = kLeft;
        orbit_rule.modifier_mask = vne::interaction::kModNone;
        orbit_rule.on_press = A::eBeginRotate;
        orbit_rule.on_release = A::eEndRotate;
        orbit_rule.on_delta = A::eRotateDelta;
        mapper.addRule(orbit_rule);

        // MMB → pan
        R pan_rule;
        pan_rule.trigger = T::eMouseButton;
        pan_rule.code = kMiddle;
        pan_rule.modifier_mask = vne::interaction::kModNone;
        pan_rule.on_press = A::eBeginPan;
        pan_rule.on_release = A::eEndPan;
        pan_rule.on_delta = A::ePanDelta;
        mapper.addRule(pan_rule);

        // Scroll → zoom (no modifier required)
        R scroll_rule;
        scroll_rule.trigger = T::eScroll;
        scroll_rule.code = 0;
        scroll_rule.modifier_mask = vne::interaction::kModNone;
        scroll_rule.on_delta = A::eZoomAtCursor;
        mapper.addRule(scroll_rule);

        // Double-click LMB → set pivot along view ray
        R dblclick_rule;
        dblclick_rule.trigger = T::eMouseDblClick;
        dblclick_rule.code = kLeft;
        dblclick_rule.modifier_mask = vne::interaction::kModNone;
        dblclick_rule.on_press = A::eSetPivotAtCursor;
        mapper.addRule(dblclick_rule);

        VNE_LOG_INFO << "  Custom orbit rules from scratch: count=" << mapper.rules().size();

        // Wire to a CameraRig and drive with simulated events
        auto rig = vne::interaction::CameraRig::makeTrackball();
        rig.setCamera(camera);
        rig.onResize(kVpW, kVpH);

        mapper.setActionCallback([&rig](vne::interaction::CameraActionType action,
                                        const vne::interaction::CameraCommandPayload& payload,
                                        double dt) { rig.onAction(action, payload, dt); });

        // Feed raw mapper input directly
        mapper.onMouseButton(kLeft, true, kCx, kCy, kDt);
        mapper.onMouseMove(kCx + 10.0f, kCy, 10.0f, 0.0f, kDt);
        mapper.onMouseMove(kCx + 20.0f, kCy + 5.0f, 10.0f, 5.0f, kDt);
        mapper.onMouseButton(kLeft, false, kCx + 20.0f, kCy + 5.0f, kDt);
        rig.onUpdate(kDt);

        mapper.onMouseScroll(0.0f, -1.0f, kCx, kCy, kDt);
        rig.onUpdate(kDt);

        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  After direct mapper drive — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";

        // resetState: call on focus loss to clear held-button tracking
        mapper.resetState();
        VNE_LOG_INFO << "  mapper.resetState() called (focus lost)";
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Section F: Touch gestures
    // ─────────────────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "--- F: Touch gestures (onTouchPan / onTouchPinch) ---";
    {
        vne::interaction::InputMapper mapper;
        mapper.setRules(vne::interaction::InputMapper::orbitPreset());

        auto rig = vne::interaction::CameraRig::makeTrackball();
        rig.setCamera(camera);
        rig.onResize(kVpW, kVpH);
        mapper.setActionCallback([&rig](vne::interaction::CameraActionType action,
                                        const vne::interaction::CameraCommandPayload& payload,
                                        double dt) { rig.onAction(action, payload, dt); });

        // Touch pan (two-finger drag)
        vne::interaction::TouchPan pan_gesture;
        pan_gesture.delta_x_px = 15.0f;
        pan_gesture.delta_y_px = -8.0f;
        for (int i = 0; i < 10; ++i) {
            mapper.onTouchPan(pan_gesture, kDt);
            rig.onUpdate(kDt);
        }
        VNE_LOG_INFO << "  Touch pan applied";

        // Touch pinch (two-finger pinch-to-zoom)
        vne::interaction::TouchPinch pinch_gesture;
        pinch_gesture.scale = 0.9f;  // < 1 = zoom in
        pinch_gesture.center_x_px = kCx;
        pinch_gesture.center_y_px = kCy;
        for (int i = 0; i < 5; ++i) {
            mapper.onTouchPinch(pinch_gesture, kDt);
            rig.onUpdate(kDt);
        }
        VNE_LOG_INFO << "  Touch pinch applied";

        const auto pos = camera->getPosition();
        VNE_LOG_INFO << "  After touch — eye=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }

    VNE_LOG_INFO << "06_custom_input_bindings: done.";
    return 0;
}

}  // namespace vne::interaction::examples
