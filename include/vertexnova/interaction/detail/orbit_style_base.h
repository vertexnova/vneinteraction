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
 * @brief Base for orbit-style manipulators (Orbit, Arcball).
 *
 * Shared pan/zoom, fitToAABB, getWorldUnitsPerPixel, input dispatch.
 * Subclasses implement computeFront(), syncFromCamera(), applyToCamera(),
 * beginRotate(), dragRotate(), endRotate(), and override applyInertia() for rotation.
 * Exported so virtuals are visible across the DLL boundary (e.g. when linking tests).
 */
class VNE_INTERACTION_API OrbitStyleBase : public CameraManipulatorBase {
   protected:
    vne::math::Vec3f world_up_;
    vne::math::Vec3f coi_world_;
    float orbit_distance_ = 5.0f;
    float fov_zoom_speed_ = 1.05f;
    ButtonMap button_map_;
    OrbitInteractionState interaction_;
    vne::math::Vec3f inertia_pan_velocity_;
    float rot_damping_ = 8.0f;
    float pan_damping_ = 10.0f;
    float rotation_speed_ = 0.2f;
    float pan_speed_ = 1.0f;
    bool shift_ = false;
    RotationPivotMode pivot_mode_ = RotationPivotMode::eCoi;

    // Smooth fitToAABB animation targets
    float target_orbit_distance_ = 5.0f;
    vne::math::Vec3f target_coi_world_;
    bool animating_fit_ = false;

    OrbitStyleBase() = default;
    ~OrbitStyleBase() override = default;

    [[nodiscard]] virtual vne::math::Vec3f computeFront() const noexcept = 0;
    virtual void syncFromCamera() noexcept = 0;
    virtual void applyToCamera() noexcept = 0;
    virtual void beginRotate(float x_px, float y_px) noexcept = 0;
    virtual void dragRotate(float delta_x_px, float delta_y_px, double delta_time) noexcept = 0;
    virtual void endRotate(double delta_time) noexcept = 0;
    /// Called when coi_world_ changes due to pivot mode logic (eViewCenter pan end,
    /// fitToAABB animation convergence). Subclasses resync their rotation state.
    virtual void onPivotChanged() noexcept {}

    [[nodiscard]] vne::math::Vec3f computeRight(const vne::math::Vec3f& front) const noexcept;
    [[nodiscard]] vne::math::Vec3f computeUp(const vne::math::Vec3f& front,
                                             const vne::math::Vec3f& right) const noexcept;
    void beginPan(float x_px, float y_px) noexcept;
    void dragPan(float x_px, float y_px, float delta_x_px, float delta_y_px, double delta_time) noexcept;
    void endPan(double delta_time) noexcept;
    void zoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void zoomOrthoToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    void doPanInertia(double delta_time) noexcept;
    virtual void applyInertia(double delta_time) noexcept;

    [[nodiscard]] bool isPerspective() const noexcept;
    [[nodiscard]] bool isOrthographic() const noexcept;

   public:
    /**
     * @brief Update camera and inertia state.
     * @param delta_time Time since last update in seconds
     */
    void update(double delta_time) noexcept override;

    /**
     * @brief Apply a semantic camera command.
     * @param action Action type (rotate, pan, zoom, fit, reset, etc.)
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

    /** Reset manipulator state and inertia. */
    void resetState() noexcept override;

    /**
     * @brief Adjust camera to frame the given bounding box.
     * @param min_world Minimum corner of AABB in world space
     * @param max_world Maximum corner of AABB in world space
     */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;

    /**
     * @brief Get world units per pixel for screen-to-world conversions.
     * @return World-space distance per pixel at center of view
     */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    /**
     * @brief Set the rotation pivot mode (COI vs view center).
     * @param mode Pivot mode
     */
    void setPivotMode(RotationPivotMode mode) noexcept { pivot_mode_ = mode; }

    /**
     * @brief Get the current rotation pivot mode.
     * @return Current pivot mode
     */
    [[nodiscard]] RotationPivotMode getPivotMode() const noexcept { return pivot_mode_; }
};

}  // namespace vne::interaction
