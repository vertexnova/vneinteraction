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
 * - **eGame** — WASD fly + RMB orbit: walk/fly and inspect simultaneously
 *
 * ### Speed
 * @code
 * ctrl.setMoveSpeed(10.0f);
 * ctrl.setSprintMultiplier(5.0f);
 * @endcode
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/camera_rig.h"

#include <memory>

namespace vne::events {
class Event;
}

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

class InputMapper;
class FreeLookBehavior;
class OrbitArcballBehavior;

/** Navigation mode for Navigation3DController. */
enum class NavigateMode : std::uint8_t {
    eFps = 0,   //!< FPS: WASD + mouse look, world-up fixed, pitch clamped (default)
    eFly = 1,   //!< Fly: WASD + mouse look, unconstrained
    eGame = 2,  //!< Game hybrid: WASD fly + RMB orbit simultaneously
};

/**
 * @brief High-level camera controller for 3D environment traversal.
 *
 * Wraps a CameraRig (FreeLookBehavior, optionally + OrbitArcballBehavior) and an
 * InputMapper with a sensible preset for each NavigateMode.
 *
 * Covers: FPS games, flight/space sims, architectural walkthroughs,
 * game editors, drone simulators.
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API Navigation3DController {
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

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept;
    void onResize(float width_px, float height_px) noexcept;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept;
    void onUpdate(double delta_time) noexcept;

    // -------------------------------------------------------------------------
    // Mode
    // -------------------------------------------------------------------------

    /** Switch navigation mode. Rebuilds rig and input rules. */
    void setMode(NavigateMode mode) noexcept;
    [[nodiscard]] NavigateMode getMode() const noexcept;

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
    // Convenience
    // -------------------------------------------------------------------------

    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches
    // -------------------------------------------------------------------------

    [[nodiscard]] InputMapper& inputMapper() noexcept;
    [[nodiscard]] FreeLookBehavior& freeLookBehavior() noexcept;

    /**
     * @brief Access the orbit behavior (only valid in eGame mode).
     * Returns nullptr in eFps / eFly modes.
     * @warning The returned pointer is invalidated by setMode() when switching
     *          away from eGame. Do not hold the pointer across mode changes.
     */
    [[nodiscard]] OrbitArcballBehavior* orbitArcballBehavior() noexcept;

   private:
    void rebuild() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vne::interaction
