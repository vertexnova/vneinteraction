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

#include "vertexnova/interaction/camera_behavior.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/detail/camera_manipulator_base.h"

#include <vertexnova/math/core/core.h>

#include <functional>
#include <memory>

namespace vne::interaction {

/**
 * @brief Follow/tracking camera behavior with target tracking and offset control.
 *
 * Implements both ICameraManipulator (for factory and controller compatibility) and
 * ICameraBehavior (conceptual family: autonomous/semi-autonomous camera following,
 * not direct input manipulator).
 *
 * The camera follows a target position in 3D space with a configurable offset and
 * damping for smooth motion. Useful for chase cameras, cinematic following, and
 * third-person perspectives.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, ICameraBehavior, CameraManipulatorBase
 */
class VNE_INTERACTION_API FollowManipulator final : public CameraManipulatorBase, public ICameraBehavior {
   public:
    /** Callable type that provides the target position dynamically. */
    using TargetProvider = std::function<vne::math::Vec3f()>;

    /** Construct default follow manipulator. */
    FollowManipulator() noexcept = default;
    /** Destroy follow manipulator. */
    ~FollowManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    FollowManipulator(const FollowManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    FollowManipulator& operator=(const FollowManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    FollowManipulator(FollowManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    FollowManipulator& operator=(FollowManipulator&&) noexcept = default;

    /** Check if this manipulator supports perspective projection. */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    /** Check if this manipulator supports orthographic projection. */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /** Forward ICameraBehavior::setEnabled to CameraManipulatorBase. */
    void setEnabled(bool enabled) noexcept override { CameraManipulatorBase::setEnabled(enabled); }
    /** Forward ICameraBehavior::isEnabled to CameraManipulatorBase. */
    [[nodiscard]] bool isEnabled() const noexcept override { return CameraManipulatorBase::isEnabled(); }

    /** Set the camera to follow. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    /** Update the camera position and orientation based on target tracking. */
    void update(double delta_time) noexcept override;
    /** Apply a semantic camera command to this behavior. */
    void applyCommand(CameraActionType action,
                      const CameraCommandPayload& payload,
                      double delta_time) noexcept override;

    /** Handle mouse movement (forwarded from controller). */
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    /** Handle mouse button press/release (forwarded from controller). */
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    /** Handle mouse scroll wheel input for zoom (forwarded from controller). */
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    /** Handle keyboard input (forwarded from controller). */
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    /** Handle touch pan gesture (forwarded from controller). */
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    /** Handle touch pinch (zoom) gesture (forwarded from controller). */
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    /** Reset manipulator state. */
    void resetState() noexcept override {}
    /** Adjust camera to frame the given AABB. */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    /** Get world units per pixel (for measuring screen-space distances). */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /** Set the static target position in world space. */
    void setTargetWorld(const vne::math::Vec3f& target) noexcept { target_world_ = target; }
    /** Set a dynamic target provider function (called each update). */
    void setTargetProvider(TargetProvider provider) noexcept { target_provider_ = std::move(provider); }
    /** Get the current target position in world space. */
    [[nodiscard]] vne::math::Vec3f getTargetWorld() const noexcept;

    /** Set the world-space offset from target to camera. */
    void setOffsetWorld(const vne::math::Vec3f& offset) noexcept { offset_world_ = offset; }
    /** Get the world-space offset from target to camera. */
    [[nodiscard]] const vne::math::Vec3f& getOffsetWorld() const noexcept { return offset_world_; }
    /** Set the damping factor for smooth camera motion (default: 8.0). */
    void setDamping(float damping) noexcept { damping_ = std::max(0.0f, damping); }
    /** Get the current damping factor. */
    [[nodiscard]] float getDamping() const noexcept { return damping_; }
    /** Set the zoom method (dolly, scene scale, or FOV). */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    /** Get the current zoom method. */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    /** Set the zoom speed for scroll/pinch events. */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }

   private:
    /** Apply zoom adjustment using the configured zoom method. */
    void applyZoom(float zoom_factor) noexcept;

    vne::math::Vec3f target_world_{0.0f, 0.0f, 0.0f};  //!< Static target position in world space
    vne::math::Vec3f offset_world_{0.0f, 0.0f, 5.0f};  //!< World-space offset from target to desired camera
    TargetProvider target_provider_;                   //!< Optional dynamic target provider callable
    float damping_ = 8.0f;                             //!< Damping factor for smooth camera following
};

}  // namespace vne::interaction
