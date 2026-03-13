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
 * @file track_behavior.h
 * @brief TrackBehavior — autonomous smooth target-following camera behavior.
 *
 * Consolidates FollowManipulator into a composable ICameraBehavior.
 * The camera smoothly moves toward `target + offset` each frame using
 * exponential approach. Handles eZoomAtCursor and eResetView actions.
 */

#include "vertexnova/interaction/camera_behavior.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <functional>
#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Autonomous smooth-follow camera behavior.
 *
 * Each frame, the camera eye is smoothly interpolated toward
 * `getTargetWorld() + offset_world_` using exponential decay:
 *
 *   new_eye = eye + (desired_eye - eye) * (1 - exp(-damping * dt))
 *
 * The target point can be set directly or provided by a callback
 * (e.g. a moving scene object).
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 */
class TrackBehavior final : public ICameraBehavior {
   public:
    TrackBehavior() noexcept = default;
    ~TrackBehavior() noexcept override = default;

    TrackBehavior(const TrackBehavior&) = delete;
    TrackBehavior& operator=(const TrackBehavior&) = delete;
    TrackBehavior(TrackBehavior&&) noexcept = default;
    TrackBehavior& operator=(TrackBehavior&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ICameraBehavior
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a camera action.
     * Handles: eZoomAtCursor, eResetView. All other actions are ignored.
     */
    bool onAction(CameraActionType action, const CameraCommandPayload& payload, double delta_time) noexcept override;

    /** Advance the follow interpolation for one frame. */
    void update(double delta_time) noexcept override;

    /** Attach camera. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /** Set viewport dimensions in pixels. */
    void setViewportSize(float width_px, float height_px) noexcept override;

    /** No stateful interaction to reset; no-op. */
    void resetState() noexcept override {}

    [[nodiscard]] bool isEnabled() const noexcept override { return enabled_; }
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }

    // -------------------------------------------------------------------------
    // Track-specific API
    // -------------------------------------------------------------------------

    /**
     * @brief Set a callback that returns the current target position each frame.
     * @param provider Callable returning Vec3f; if nullptr, target_world_ is used.
     */
    void setTargetProvider(std::function<vne::math::Vec3f()> provider) noexcept {
        target_provider_ = std::move(provider);
    }

    /**
     * @brief Set a fixed world-space target position.
     * Clears any installed target provider.
     */
    void setTargetWorld(const vne::math::Vec3f& target) noexcept {
        target_world_ = target;
        target_provider_ = nullptr;
    }

    /** Get the current target position (from provider or fixed). */
    [[nodiscard]] vne::math::Vec3f getTargetWorld() const noexcept;

    /**
     * @brief Set the camera offset from the target (world space).
     * Default: (0, 2, 5) — behind and above the target.
     */
    void setOffset(const vne::math::Vec3f& offset) noexcept { offset_world_ = offset; }
    [[nodiscard]] vne::math::Vec3f getOffset() const noexcept { return offset_world_; }

    /**
     * @brief Set the follow damping factor (>= 0; higher = faster catch-up).
     * Default: 5.0. Uses: new_eye = eye + (desired - eye) * (1 - exp(-damping * dt))
     */
    void setDamping(float damping) noexcept { damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getDamping() const noexcept { return damping_; }

    /** Set zoom method. */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /** Set zoom speed (>= 0.01). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

    /** Get world units per pixel. */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept;

    /**
     * @brief Move target to AABB center.
     * @param min_world AABB min corner
     * @param max_world AABB max corner
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept;

   private:
    void applyZoom(float zoom_factor) noexcept;

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> perspCamera() const noexcept;
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> orthoCamera() const noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;

    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    float scene_scale_ = 1.0f;

    vne::math::Vec3f target_world_{0.0f, 0.0f, 0.0f};
    vne::math::Vec3f offset_world_{0.0f, 2.0f, 5.0f};
    float damping_ = 5.0f;

    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
    float zoom_speed_ = 1.1f;

    std::function<vne::math::Vec3f()> target_provider_;
};

}  // namespace vne::interaction
