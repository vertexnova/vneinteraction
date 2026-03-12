#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/detail/camera_manipulator_base.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

/**
 * @brief Base for free-look manipulators (FPS, Fly).
 *
 * Shared WASD movement, look, zoom, and input handling.
 * Subclasses implement upVector() (FPS: world_up_, Fly: camera up).
 * Exported so virtuals are visible across the DLL boundary (e.g. when linking tests).
 */
class VNE_INTERACTION_API FreeCameraBase : public CameraManipulatorBase {
   protected:
    float yaw_deg_ = 0.0f;
    float pitch_deg_ = 0.0f;
    float move_speed_ = 3.0f;
    float mouse_sensitivity_ = 0.15f;
    float fov_zoom_speed_ = 1.05f;
    float sprint_mult_ = 4.0f;
    float slow_mult_ = 0.2f;
    FreeLookInputState input_state_;

    FreeCameraBase() = default;
    ~FreeCameraBase() override = default;

    [[nodiscard]] virtual vne::math::Vec3f upVector() const noexcept = 0;

    [[nodiscard]] vne::math::Vec3f front() const noexcept;
    [[nodiscard]] vne::math::Vec3f right(const vne::math::Vec3f& front_vec) const noexcept;
    void syncAnglesFromCamera() noexcept;
    void applyAnglesToCamera() noexcept;
    void applyZoom(float zoom_step_or_factor) noexcept;

   public:
    /**
     * @brief Set the camera to manipulate.
     * @param camera Shared pointer to the camera (may be nullptr)
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /**
     * @brief Update camera based on input state.
     * @param delta_time Time since last update in seconds
     */
    void update(double delta_time) noexcept override;

    /**
     * @brief Apply a semantic camera command.
     * @param action Action type (move, look, zoom, fit, reset, etc.)
     * @param payload Payload with parameters for the action
     * @param delta_time Time since last update in seconds
     */
    void applyCommand(CameraActionType action,
                      const CameraCommandPayload& payload,
                      double delta_time) noexcept override;

    /**
     * @brief Handle mouse movement input.
     * @param x Current cursor X in viewport pixels
     * @param y Current cursor Y in viewport pixels
     * @param delta_x Horizontal delta in pixels
     * @param delta_y Vertical delta in pixels
     * @param delta_time Time since last input in seconds
     */
    void onMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;

    /**
     * @brief Handle mouse button press/release.
     * @param button Mouse button index
     * @param pressed true if pressed, false if released
     * @param x Cursor X in viewport pixels
     * @param y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void onMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;

    /**
     * @brief Handle mouse scroll wheel input for zoom.
     * @param scroll_x Horizontal scroll delta
     * @param scroll_y Vertical scroll delta
     * @param mouse_x Cursor X in viewport pixels
     * @param mouse_y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void onMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;

    /**
     * @brief Handle keyboard input.
     * @param key Key code
     * @param pressed true if pressed, false if released
     * @param delta_time Time since last input in seconds
     */
    void onKeyboard(int key, bool pressed, double delta_time) noexcept override;

    /**
     * @brief Handle touch pan gesture.
     * @param pan Pan gesture with delta_x_px and delta_y_px
     * @param delta_time Time since last input in seconds
     */
    void onTouchPan(const TouchPan& pan, double delta_time) noexcept override;

    /**
     * @brief Handle touch pinch (zoom) gesture.
     * @param pinch Pinch gesture with scale and center position
     * @param delta_time Time since last input in seconds
     */
    void onTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;
};

}  // namespace vne::interaction
