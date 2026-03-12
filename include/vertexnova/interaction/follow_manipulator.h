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

    /**
     * @brief Check if this manipulator supports perspective projection.
     * @return true (follow supports perspective)
     */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }

    /**
     * @brief Check if this manipulator supports orthographic projection.
     * @return true (follow supports orthographic)
     */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /** Forward ICameraBehavior::setEnabled to CameraManipulatorBase. */
    void setEnabled(bool enabled) noexcept override { CameraManipulatorBase::setEnabled(enabled); }
    /** Forward ICameraBehavior::isEnabled to CameraManipulatorBase. */
    [[nodiscard]] bool isEnabled() const noexcept override { return CameraManipulatorBase::isEnabled(); }

    /**
     * @brief Set the camera to follow.
     * @param camera Shared pointer to the camera (may be nullptr)
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /**
     * @brief Update the camera position and orientation based on target tracking.
     * @param delta_time Time since last update in seconds
     */
    void update(double delta_time) noexcept override;

    /**
     * @brief Apply a semantic camera command to this behavior.
     * @param action Action type (zoom, fit, reset, etc.)
     * @param payload Payload with parameters for the action
     * @param delta_time Time since last update in seconds
     */
    void applyCommand(CameraActionType action,
                      const CameraCommandPayload& payload,
                      double delta_time) noexcept override;

    /**
     * @brief Handle mouse movement (forwarded from controller).
     * @param x Current cursor X in viewport pixels
     * @param y Current cursor Y in viewport pixels
     * @param delta_x Horizontal delta in pixels
     * @param delta_y Vertical delta in pixels
     * @param delta_time Time since last input in seconds
     */
    void onMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;

    /**
     * @brief Handle mouse button press/release (forwarded from controller).
     * @param button Mouse button index
     * @param pressed true if pressed, false if released
     * @param x Cursor X in viewport pixels
     * @param y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void onMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;

    /**
     * @brief Handle mouse scroll wheel input for zoom (forwarded from controller).
     * @param scroll_x Horizontal scroll delta
     * @param scroll_y Vertical scroll delta
     * @param mouse_x Cursor X in viewport pixels
     * @param mouse_y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void onMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;

    /**
     * @brief Handle keyboard input (forwarded from controller).
     * @param key Key code
     * @param pressed true if pressed, false if released
     * @param delta_time Time since last input in seconds
     */
    void onKeyboard(int key, bool pressed, double delta_time) noexcept override;

    /**
     * @brief Handle touch pan gesture (forwarded from controller).
     * @param pan Pan gesture with delta_x_px and delta_y_px
     * @param delta_time Time since last input in seconds
     */
    void onTouchPan(const TouchPan& pan, double delta_time) noexcept override;

    /**
     * @brief Handle touch pinch (zoom) gesture (forwarded from controller).
     * @param pinch Pinch gesture with scale and center position
     * @param delta_time Time since last input in seconds
     */
    void onTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    /** Reset manipulator state. */
    void resetState() noexcept override {}

    /**
     * @brief Adjust camera to frame the given AABB.
     * @param min_world Minimum corner of AABB in world space
     * @param max_world Maximum corner of AABB in world space
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;

    /**
     * @brief Get world units per pixel (for measuring screen-space distances).
     * @return World-space distance per pixel at center of view
     */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /**
     * @brief Set the static target position in world space.
     * @param target Target position in world coordinates
     */
    void setTargetWorld(const vne::math::Vec3f& target) noexcept { target_world_ = target; }

    /**
     * @brief Set a dynamic target provider function (called each update).
     * @param provider Callable that returns the current target position
     */
    void setTargetProvider(TargetProvider provider) noexcept { target_provider_ = std::move(provider); }

    /**
     * @brief Get the current target position in world space.
     * @return Target position (from static or dynamic provider)
     */
    [[nodiscard]] vne::math::Vec3f getTargetWorld() const noexcept;

    /**
     * @brief Set the world-space offset from target to camera.
     * @param offset Offset vector in world space
     */
    void setOffsetWorld(const vne::math::Vec3f& offset) noexcept { offset_world_ = offset; }

    /**
     * @brief Get the world-space offset from target to camera.
     * @return Reference to the current offset vector
     */
    [[nodiscard]] const vne::math::Vec3f& getOffsetWorld() const noexcept { return offset_world_; }

    /**
     * @brief Set the damping factor for smooth camera motion (default: 8.0).
     * @param damping Damping factor (clamped to >= 0)
     */
    void setDamping(float damping) noexcept { damping_ = std::max(0.0f, damping); }

    /**
     * @brief Get the current damping factor.
     * @return Current damping factor
     */
    [[nodiscard]] float getDamping() const noexcept { return damping_; }

    /**
     * @brief Set the zoom method (dolly, scene scale, or FOV).
     * @param method Zoom method
     */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }

    /**
     * @brief Get the current zoom method.
     * @return Current zoom method
     */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /**
     * @brief Set the zoom speed for scroll/pinch events.
     * @param speed Zoom speed (clamped to >= 0.01)
     */
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
