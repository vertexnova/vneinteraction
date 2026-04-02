#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file input_mapper.h
 * @brief InputMapper — viewport input interpreter: raw events to @ref CameraActionType commands.
 *
 * @par Command pipeline
 * Window or UI code forwards pointer, key, scroll, and touch data into @ref onMouseButton,
 * @ref onMouseMove, @ref onKey, etc. The mapper matches @ref InputRule rows (trigger + optional
 * modifiers) and emits semantic actions through @ref setActionCallback. High-level controllers
 * typically wire that callback to @ref CameraRig::onAction; see @ref Inspect3DController.
 *
 * @par Rules and state
 * Rules are data-driven: each @ref InputRule maps a trigger to @c on_press, @c on_release, and/or
 * @c on_delta. The mapper tracks which button/key rules are active so drag deltas only apply
 * while the correct chord is held. @ref resetState clears that tracking (focus loss, mode change).
 * If multiple rules match the same trigger (e.g. same button), the strictest modifier chord wins
 * (most required Shift/Ctrl/Alt bits); equal specificity uses the earlier rule in the list.
 *
 * @par Presets and rebinding
 * Static factories (@ref orbitPreset, @ref fpsPreset, …) return full rule vectors for common modes.
 * @ref bindGesture, @ref bindScroll, @ref bindDoubleClick, and @ref bindKey adjust bindings without
 * editing @ref InputRule directly — the intended path for app-level customization from controllers.
 *
 * @par Symmetry with behaviors
 * @ref OrbitBehavior and @ref TrackballBehavior own orbit math; @ref InputMapper owns **which**
 * gestures produce @c eBeginRotate, @c ePanDelta, @c eZoomAtCursor, etc.
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <functional>
#include <span>
#include <vector>

namespace vne::interaction {

/**
 * @brief Callback when an @ref InputRule matches and emits an action.
 * @param action     Semantic action (see @ref CameraActionType).
 * @param payload    Pointer position, deltas, zoom factor, and press state as applicable.
 * @param delta_time Time delta in seconds from the UI (may be @c 0 for discrete events).
 */
using ActionCallback = std::function<void(CameraActionType, const CameraCommandPayload&, double)>;

/**
 * @brief Data-driven mapper from low-level input to @ref CameraActionType values.
 *
 * Typical usage (inside a controller):
 * - Build rules: @ref setRules with a preset or custom @ref InputRule list.
 * - Register @ref setActionCallback before any input (forwards to @ref CameraRig::onAction or custom logic).
 * - Each frame or event: call the @c on* entry points; optional @ref resetState on focus loss.
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread as the window/event source.
 */
class VNE_INTERACTION_API InputMapper {
   public:
    InputMapper();

    /** Replace the entire rule set. */
    void setRules(std::span<const InputRule> rules);

    /** Append a single rule to the current set. */
    void addRule(InputRule rule);

    /** Remove all rules. */
    void clearRules();

    /** Get the current rule set (read-only). */
    [[nodiscard]] const std::vector<InputRule>& rules() const noexcept { return rules_; }

    /**
     * @brief Register the handler for emitted actions.
     * @param cb Callback invoked from @ref emit; replaces any previous callback.
     * @note Must be set before processing input; otherwise actions are logged and dropped.
     */
    void setActionCallback(ActionCallback cb) { callback_ = std::move(cb); }

    /**
     * @brief Mouse button press or release.
     * @param button  Button index (same encoding as @c vne::events::MouseButton).
     * @param pressed @c true on press, @c false on release.
     * @param x       Cursor X in pixels.
     * @param y       Cursor Y in pixels.
     * @param dt      Time delta in seconds.
     */
    void onMouseButton(int button, bool pressed, float x, float y, double dt) noexcept;

    /**
     * @brief Mouse double-click (e.g. pivot / COI rules).
     * @param button Button index.
     * @param x      Cursor X in pixels.
     * @param y      Cursor Y in pixels.
     * @param dt     Time delta in seconds.
     */
    void onMouseDoubleClick(int button, float x, float y, double dt) noexcept;

    /**
     * @brief Pointer motion; may emit drag deltas for active button/key rules.
     * @param x  Cursor X in pixels.
     * @param y  Cursor Y in pixels.
     * @param dx Delta X in pixels since last move.
     * @param dy Delta Y in pixels since last move.
     * @param dt Time delta in seconds.
     */
    void onMouseMove(float x, float y, float dx, float dy, double dt) noexcept;

    /**
     * @brief Scroll wheel or trackpad scroll.
     * @param scroll_x Horizontal scroll (device units).
     * @param scroll_y Vertical scroll (device units).
     * @param mouse_x  Cursor X in pixels (zoom-at-cursor).
     * @param mouse_y  Cursor Y in pixels.
     * @param dt       Time delta in seconds.
     */
    void onMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double dt) noexcept;

    /**
     * @brief Key press or release; updates modifier state and key-held rules.
     * @param key     Key code (@c vne::events::KeyCode).
     * @param pressed @c true on press, @c false on release.
     * @param dt      Time delta in seconds.
     */
    void onKey(int key, bool pressed, double dt) noexcept;

    /**
     * @brief Touch pan gesture.
     * @param pan Pan deltas in pixels.
     * @param dt  Time delta in seconds.
     */
    void onTouchPan(const TouchPan& pan, double dt) noexcept;

    /**
     * @brief Touch pinch (zoom) gesture.
     * @param pinch Scale and center in pixels.
     * @param dt    Time delta in seconds.
     */
    void onTouchPinch(const TouchPinch& pinch, double dt) noexcept;

    /**
     * @brief Clear active button/key tracking and modifier chord state.
     * @details Use on focus loss, mode change, or when the owning controller @c reset() runs.
     */
    void resetState() noexcept;

    // -------------------------------------------------------------------------
    // Built-in presets — return a complete rule set for each use case
    // -------------------------------------------------------------------------

    /** Orbit: LMB=rotate, RMB=pan, MMB=pan, Shift+LMB=pan, scroll=zoom, dblclick LMB=set pivot. */
    static std::vector<InputRule> orbitPreset();

    /** FPS: RMB=look, WASD=move, QE=up/down, Shift=sprint, Ctrl=slow, scroll=zoom. */
    static std::vector<InputRule> fpsPreset();

    /**
     * Game camera: LMB=orbit rotate, RMB=look, WASD=move, QE=up/down,
     * scroll=zoom, dblclick LMB=set pivot.
     */
    static std::vector<InputRule> gamePreset();

    /** CAD: MMB=pan, Shift+MMB=rotate, scroll=zoom, dblclick MMB=set pivot. */
    static std::vector<InputRule> cadPreset();

    /** Ortho 2D: RMB/MMB=pan, scroll=zoom, no rotation rules. */
    static std::vector<InputRule> orthoPreset();

    // -------------------------------------------------------------------------
    // Gesture binding API — remap controls without exposing InputRule
    // -------------------------------------------------------------------------

    /**
     * @brief Bind a button+drag gesture (rotate, pan, look) to a new mouse button.
     * Replaces any existing rule for this gesture. Call after setRules(preset).
     */
    void bindGesture(GestureAction action, MouseBinding binding);

    /**
     * @brief Bind scroll wheel to zoom (or other scroll action). Replaces existing scroll rule.
     * @param modifier Modifier required (eModNone for default).
     */
    void bindScroll(GestureAction action, vne::events::ModifierKey modifier = vne::events::ModifierKey::eModNone);

    /**
     * @brief Bind double-click to `eSetPivotAtCursor` (orbit COI along view direction; see OrbitalCameraManipulator).
     * Replaces existing double-click rule for this action.
     */
    void bindDoubleClick(GestureAction action,
                         MouseButton button,
                         vne::events::ModifierKey modifier = vne::events::ModifierKey::eModNone);

    /**
     * @brief Remove all rules that implement the given gesture.
     */
    void unbindGesture(GestureAction action);

    /**
     * @brief Bind a keyboard key to press/release actions (replaces prior rules with the same @a press_action).
     */
    void bindKey(CameraActionType press_action,
                 CameraActionType release_action,
                 vne::events::KeyCode key,
                 vne::events::ModifierKey modifier = vne::events::ModifierKey::eModNone);

    /** Remove all key rules whose @c on_press equals @a press_action. */
    void unbindKey(CameraActionType press_action);

   private:
    void emit(CameraActionType action, const CameraCommandPayload& payload, double dt) noexcept;
    [[nodiscard]] bool modifiersMatch(int mask) const noexcept;

    std::vector<InputRule> rules_;  //!< Active rule table (@ref setRules / @ref addRule).
    ActionCallback callback_;       //!< Sink for matched actions; empty until @ref setActionCallback.

    static constexpr int kMaxButtons = 8;  //!< Mouse button slots for @a active_button_rule_.
    static constexpr int kMaxKeys = 512;   //!< Key code range tracked (0..511).
    int active_button_rule_[kMaxButtons] = {
        -1, -1, -1, -1, -1, -1, -1, -1};  //!< Per-button index into @a rules_, or @c -1 if none.
    bool active_key_[kMaxKeys] = {};  //!< Per-key held flags for @c InputRule::Trigger::eKey.
    int active_key_rule_[kMaxKeys] = {};  //!< Filled with @c -1 in @ref resetState; rule index for key press/release pairing.

    bool mod_shift_ = false;  //!< Shift held (updated in @ref onKey).
    bool mod_ctrl_ = false;   //!< Ctrl held.
    bool mod_alt_ = false;    //!< Alt held.
};

}  // namespace vne::interaction
