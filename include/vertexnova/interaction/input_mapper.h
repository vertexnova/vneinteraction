#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @brief Translates raw input events into CameraActionType commands using
 * data-driven InputRule mappings.
 *
 * Replaces CameraInputAdapter's hardcoded logic. Callers configure rules via
 * setRules() or addRule(), or use one of the static preset factories.
 * The controller calls the typed on*() methods which evaluate rules and emit
 * the appropriate actions to the registered callback.
 */

#include "vertexnova/interaction/interaction_types.h"

#include <functional>
#include <span>
#include <vector>

namespace vne::interaction {

/**
 * @brief Callback type invoked when an InputRule matches and emits an action.
 * Parameters: (action, payload, delta_time)
 */
using ActionCallback = std::function<void(CameraActionType, const CameraCommandPayload&, double)>;

/**
 * @brief Data-driven input → CameraAction mapper.
 *
 * Each InputRule describes a trigger (button, key, scroll, touch, double-click),
 * optional modifier mask, and up to three action types (on_press, on_release, on_delta).
 * eNone is used as a sentinel meaning "no action for this phase".
 *
 * The mapper tracks which button/key rules are currently "active" (pressed) so that
 * on_delta actions are only emitted for rules whose trigger button/key is held down.
 */
class InputMapper {
   public:
    InputMapper() = default;

    /** Replace the entire rule set. */
    void setRules(std::span<const InputRule> rules);

    /** Append a single rule to the current set. */
    void addRule(InputRule rule);

    /** Remove all rules. */
    void clearRules();

    /** Get the current rule set (read-only). */
    [[nodiscard]] const std::vector<InputRule>& rules() const noexcept { return rules_; }

    /**
     * @brief Set the callback invoked when a rule fires an action.
     * Must be set before processing any input.
     */
    void setActionCallback(ActionCallback cb) { callback_ = std::move(cb); }

    // --- Input event entry points (called by controller::onEvent) ---

    void onMouseButton(int button, bool pressed, float x, float y, double dt) noexcept;
    void onMouseDoubleClick(int button, float x, float y, double dt) noexcept;
    void onMouseMove(float x, float y, float dx, float dy, double dt) noexcept;
    void onMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double dt) noexcept;
    void onKey(int key, bool pressed, double dt) noexcept;
    void onTouchPan(const TouchPan& pan, double dt) noexcept;
    void onTouchPinch(const TouchPinch& pinch, double dt) noexcept;

    /** Reset all active-button/key tracking state (e.g. on focus loss or reset). */
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

   private:
    void emit(CameraActionType action, const CameraCommandPayload& payload, double dt) noexcept;

    /** Returns true if the current modifier state satisfies the rule's modifier_mask. */
    [[nodiscard]] bool modifiersMatch(int mask) const noexcept;

    std::vector<InputRule> rules_;
    ActionCallback callback_;

    // Active-press tracking: which button/key rules are currently held
    // We store the rule index of each active mouse button slot (indexed by button code)
    static constexpr int kMaxButtons = 8;
    static constexpr int kMaxKeys = 512;                                      // tracks keys 0..511
    int active_button_rule_[kMaxButtons] = {-1, -1, -1, -1, -1, -1, -1, -1};  // -1 = not active
    bool active_key_[kMaxKeys] = {};

    // Current modifier state (updated by onKey)
    bool mod_shift_ = false;
    bool mod_ctrl_ = false;
    bool mod_alt_ = false;
};

}  // namespace vne::interaction
