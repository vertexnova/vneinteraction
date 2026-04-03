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
 * @file follow_controller.h
 * @brief FollowController — high-level camera controller for target tracking.
 *
 * Smoothly follows a moving target (e.g. a game character, vehicle, or
 * animated object) with configurable offset and lag.
 *
 * @code
 * auto ctrl = vne::interaction::FollowController{};
 * ctrl.setCamera(camera);
 * // Follow a dynamic transform (only translation from the matrix is used for the target point):
 * ctrl.setTarget([&]{ return player.worldTransform(); });
 * ctrl.setOffset({0.0f, 2.0f, 5.0f});   // world space: same default semantics as FollowManipulator
 * ctrl.setLag(0.1f);                    // smooth follow
 * @endcode
 *
 * ### Static target
 * @code
 * vne::math::Mat4f target_transform = ...;
 * ctrl.setTarget(target_transform);
 * @endcode
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/camera_controller.h"
#include "vertexnova/interaction/camera_rig.h"

#include <vertexnova/math/core/core.h>

#include <functional>
#include <memory>

namespace vne::events {
class Event;
}

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

class InputMapper;
class FollowManipulator;

/**
 * @brief High-level camera controller for smooth target following.
 *
 * Wraps a CameraRig (FollowManipulator). No user input is required;
 * the camera autonomously tracks the target transform each frame.
 *
 * Covers: third-person game cameras, cinematic rigs, vehicle chase cams.
 * Target orientation from @c Mat4f is not used to rotate the offset — only the
 * translation column; offset is world-space (see @ref FollowManipulator::setOffset).
 *
 * @threadsafe Not thread-safe. Call all methods from the same thread.
 */
class VNE_INTERACTION_API FollowController : public ICameraController {
   public:
    /** Callback type returning the current target world transform. */
    using TargetCallback = std::function<vne::math::Mat4f()>;

    FollowController();
    ~FollowController();

    FollowController(const FollowController&) = delete;
    FollowController& operator=(const FollowController&) = delete;
    FollowController(FollowController&&) noexcept;
    FollowController& operator=(FollowController&&) noexcept;

    // -------------------------------------------------------------------------
    // Core setup
    // -------------------------------------------------------------------------

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void onResize(float width_px, float height_px) noexcept override;

    // -------------------------------------------------------------------------
    // Per-frame
    // -------------------------------------------------------------------------

    /** No user input is required; call onUpdate() each frame to advance tracking. */
    void onUpdate(double delta_time) noexcept override;

    /** Optional: feed events to allow user-controlled zoom or offset adjustment. */
    void onEvent(const vne::events::Event& event, double delta_time = 0.0) noexcept override;

    // -------------------------------------------------------------------------
    // Target
    // -------------------------------------------------------------------------

    /**
     * @brief Set a dynamic target via callback (called every onUpdate()).
     * @param callback Returns current world-space transform; only translation is used.
     */
    void setTarget(TargetCallback callback) noexcept;

    /**
     * @brief Set a static target transform (updated once, not polled).
     * Only the translation (fourth column) is read; call again when the target moves.
     */
    void setTarget(const vne::math::Mat4f& world_transform) noexcept;

    // -------------------------------------------------------------------------
    // Follow parameters
    // -------------------------------------------------------------------------

    /**
     * @brief World-space vector from target position to desired eye: @c desired_eye = target + offset.
     * Not rotated by target orientation; see @ref FollowManipulator::setOffset for axis convention.
     */
    void setOffset(const vne::math::Vec3f& world_offset) noexcept;
    [[nodiscard]] vne::math::Vec3f getOffset() const noexcept;

    /**
     * @brief Smoothing lag factor [0, 1].
     * 0 = instant snap, 1 = never catches up. Typical: 0.05 – 0.2.
     */
    void setLag(float lag) noexcept;
    [[nodiscard]] float getLag() const noexcept;

    // -------------------------------------------------------------------------
    // Convenience
    // -------------------------------------------------------------------------

    void reset() noexcept;

    // -------------------------------------------------------------------------
    // Escape hatches
    // -------------------------------------------------------------------------

    [[nodiscard]] InputMapper& inputMapper() noexcept;
    [[nodiscard]] FollowManipulator& followManipulator() noexcept;

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vne::interaction
