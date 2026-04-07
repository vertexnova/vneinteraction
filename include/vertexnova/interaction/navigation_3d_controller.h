#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file navigation_3d_controller.h
 * @brief Navigation3DController — high-level camera controller for 3D environment traversal.
 *
 * Covers FPS, Fly, and game-hybrid camera needs in three lines:
 *
 * @code
 * auto ctrl = vne::interaction::Navigation3DController{};
 * ctrl.setCamera(camera);
 * ctrl.onResize(1280, 720);
 * // In your loop:
 * ctrl.onEvent(event);
 * ctrl.onUpdate(dt);
 * @endcode
 *
 * ### Modes
 * - **eFps**  — WASD + mouse look, world-up fixed, pitch clamped ±89° (default)
 * - **eFly**  — WASD + mouse look, unconstrained, allows barrel roll
 *
 * ### Speed
 * @code
 * ctrl.setMoveSpeed(10.0f);
 * ctrl.setSprintMultiplier(5.0f);
 * @endcode
 *
 * Defaults match common viewport tools: **Shift** = sprint, **Ctrl** = slow while moving.
 *
 * For direct `FreeLookManipulator` access (sensitivity, mode, etc.), use @ref freeLookManipulator(); that reference
 * stays valid across @ref setMode and internal input-rule rebuilds.
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/camera_controller.h"
#include "vertexnova/interaction/camera_rig.h"

#include <vertexnova/events/types.h>
#include <memory>

namespace vne::events {
class Event;
}

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

class InputMapper;
class FreeLookManipulator;

/**
 * @brief High-level camera controller for 3D environment traversal.
 *
 * Wraps a CameraRig (FreeLookManipulator) and an InputMapper with a sensible preset per mode.
 *
 * Covers: FPS games, flight/space sims, architectural walkthroughs,
 * game editors, drone simulators.
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API Navigation3DController : public ICameraController {
   public:
    Navigation3DController();
    ~Navigation3DController();

    Navigation3DController(const Navigation3DController&) = delete;
    Navigation3DController& operator=(const Navigation3DController&) = delete;
    Navigation3DController(Navigation3DController&&) noexcept;
    Navigation3DController& operator=(Navigation3DController&&) noexcept;

    // -------------------------------------------------------------------------
    // Core setup
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void onResize(float width_px, float height_px) noexcept override;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept override;
    void onUpdate(double delta_time) noexcept override;

    // -------------------------------------------------------------------------
    // Mode
    // -------------------------------------------------------------------------

    /** Switch navigation mode. Rebuilds rig and input rules. */
    void setMode(FreeLookMode mode) noexcept;
    [[nodiscard]] FreeLookMode getMode() const noexcept;

    /** Mouse look: yaw/pitch or virtual trackball (see @ref FreeLookManipulator). */
    void setRotationMode(FreeLookRotationMode mode) noexcept;
    [[nodiscard]] FreeLookRotationMode getRotationMode() const noexcept;

    // -------------------------------------------------------------------------
    // Speed / sensitivity
    // -------------------------------------------------------------------------

    void setMoveSpeed(float units_per_second) noexcept;
    [[nodiscard]] float getMoveSpeed() const noexcept;

    void setMouseSensitivity(float degrees_per_pixel) noexcept;
    [[nodiscard]] float getMouseSensitivity() const noexcept;

    void setSprintMultiplier(float mult) noexcept;
    [[nodiscard]] float getSprintMultiplier() const noexcept;

    void setSlowMultiplier(float mult) noexcept;
    [[nodiscard]] float getSlowMultiplier() const noexcept;

    // -------------------------------------------------------------------------
    // Key / mouse bindings
    // -------------------------------------------------------------------------

    void setMoveForwardKey(vne::events::KeyCode key) noexcept;
    void setMoveBackwardKey(vne::events::KeyCode key) noexcept;
    void setMoveLeftKey(vne::events::KeyCode key) noexcept;
    void setMoveRightKey(vne::events::KeyCode key) noexcept;
    void setMoveUpKey(vne::events::KeyCode key) noexcept;
    void setMoveDownKey(vne::events::KeyCode key) noexcept;

    /**
     * Single sprint key (replaces default Left/Right Shift pair).
     * @param key eUnknown restores default Shift keys.
     */
    void setSpeedBoostKey(vne::events::KeyCode key) noexcept;

    /**
     * Single slow key (replaces default Left/Right Control pair).
     * @param key eUnknown restores default Ctrl keys.
     */
    void setSlowKey(vne::events::KeyCode key) noexcept;

    void setLookButton(MouseButton btn,
                       vne::events::ModifierKey modifier = vne::events::ModifierKey::eModNone) noexcept;

    // -------------------------------------------------------------------------
    // DOF (input gating)
    // -------------------------------------------------------------------------

    void setLookEnabled(bool enabled) noexcept;
    void setMoveEnabled(bool enabled) noexcept;
    void setZoomEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isLookEnabled() const noexcept;
    [[nodiscard]] bool isMoveEnabled() const noexcept;
    [[nodiscard]] bool isZoomEnabled() const noexcept;

    // -------------------------------------------------------------------------
    // Optional discrete move-speed keys (unbound by default)
    // -------------------------------------------------------------------------

    void setIncreaseMoveSpeedKey(vne::events::KeyCode key) noexcept;
    void setDecreaseMoveSpeedKey(vne::events::KeyCode key) noexcept;
    void setMoveSpeedStep(float delta) noexcept;
    void setMoveSpeedMin(float min_speed) noexcept;
    void setMoveSpeedMax(float max_speed) noexcept;

    // -------------------------------------------------------------------------
    // Convenience
    // -------------------------------------------------------------------------

    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches
    // -------------------------------------------------------------------------

    [[nodiscard]] InputMapper& inputMapper() noexcept;
    [[nodiscard]] FreeLookManipulator& freeLookManipulator() noexcept;

   private:
    void rebuild() noexcept;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vne::interaction
