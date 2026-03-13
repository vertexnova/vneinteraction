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
 * @file navigate_controller.h
 * @brief NavigateController — high-level camera controller for environment traversal.
 *
 * Covers FPS, Fly, and game-hybrid camera needs in three lines:
 *
 * @code
 * auto ctrl = vne::interaction::NavigateController{};
 * ctrl.setCamera(camera);
 * ctrl.setViewportSize(1280, 720);
 * // In your loop:
 * ctrl.onEvent(event);
 * ctrl.update(dt);
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
class OrbitBehavior;

/** Navigation mode for NavigateController. */
enum class NavigateMode : std::uint8_t {
    eFps  = 0,  //!< FPS: WASD + mouse look, world-up fixed, pitch clamped (default)
    eFly  = 1,  //!< Fly: WASD + mouse look, unconstrained
    eGame = 2,  //!< Game hybrid: WASD fly + RMB orbit simultaneously
};

/**
 * @brief High-level camera controller for environment traversal.
 *
 * Wraps a CameraRig (FreeLookBehavior, optionally + OrbitBehavior) and an
 * InputMapper with a sensible preset for each NavigateMode.
 *
 * Covers: FPS games, flight/space sims, architectural walkthroughs,
 * game editors, drone simulators.
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API NavigateController {
public:
    NavigateController();
    ~NavigateController();

    NavigateController(const NavigateController&) = delete;
    NavigateController& operator=(const NavigateController&) = delete;
    NavigateController(NavigateController&&) noexcept;
    NavigateController& operator=(NavigateController&&) noexcept;

    // -------------------------------------------------------------------------
    // Core setup
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept;
    void setViewportSize(float width_px, float height_px) noexcept;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    void onEvent(const vne::events::Event& event) noexcept;
    void update(double delta_time) noexcept;

    // -------------------------------------------------------------------------
    // Mode
    // -------------------------------------------------------------------------

    /** Switch navigation mode. Rebuilds rig and input rules. */
    void setMode(NavigateMode mode) noexcept;
    [[nodiscard]] NavigateMode getMode() const noexcept { return mode_; }

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

    void fitToAABB(const vne::math::Vec3f& min_world,
                   const vne::math::Vec3f& max_world) noexcept;

    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches
    // -------------------------------------------------------------------------

    [[nodiscard]] InputMapper& inputMapper() noexcept;
    [[nodiscard]] FreeLookBehavior& freeLookBehavior() noexcept;

    /**
     * @brief Access the orbit behavior (only valid in eGame mode).
     * Returns nullptr in eFps / eFly modes.
     */
    [[nodiscard]] OrbitBehavior* orbitBehavior() noexcept;

private:
    void rebuild() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;

    NavigateMode mode_ = NavigateMode::eFps;
};

}  // namespace vne::interaction
