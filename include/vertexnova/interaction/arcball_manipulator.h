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

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/detail/orbit_style_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

/**
 * @brief Arcball-style camera manipulator with smooth quaternion-based rotation.
 *
 * Implements an arcball (virtual trackball) interaction model where screen coordinates
 * map to a 3D sphere. Rotating the mouse around this sphere creates an intuitive
 * 3-degree-of-freedom rotation.
 *
 * Features:
 * - Smooth interpolation using unit quaternions to avoid gimbal lock
 * - Persistent accumulated orientation prevents numerical drift via periodic normalization
 * - Support for inertia-based momentum scrolling
 * - Multiple zoom methods and resolution-independent interaction
 * - Both perspective and orthographic camera support
 *
 * The arcball center tracks the current center of interest, allowing intuitive panning
 * and smooth continuity between drag sessions.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, OrbitStyleBase
 */
class VNE_INTERACTION_API ArcballManipulator final : public OrbitStyleBase {
   public:
    /** Construct arcball manipulator with default settings. */
    ArcballManipulator() noexcept;
    /** Destroy arcball manipulator. */
    ~ArcballManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    ArcballManipulator(const ArcballManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    ArcballManipulator& operator=(const ArcballManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    ArcballManipulator(ArcballManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    ArcballManipulator& operator=(ArcballManipulator&&) noexcept = default;

    /**
     * @brief Check if this manipulator supports perspective projection.
     * @return true (arcball supports perspective)
     */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }

    /**
     * @brief Check if this manipulator supports orthographic projection.
     * @return true (arcball supports orthographic)
     */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /**
     * @brief Set the camera to manipulate.
     * @param camera Shared pointer to the camera (may be nullptr)
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;

    /**
     * @brief Set the viewport dimensions for screen-to-world mapping.
     * @param width_px Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    void setViewportSize(float width_px, float height_px) noexcept override;

    /**
     * @brief Apply a semantic camera command.
     * @param action Action type (rotate, pan, zoom, etc.)
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
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;

    /**
     * @brief Handle mouse scroll wheel input for zoom.
     * @param scroll_x Horizontal scroll delta
     * @param scroll_y Vertical scroll delta
     * @param mouse_x Cursor X in viewport pixels
     * @param mouse_y Cursor Y in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;

    /**
     * @brief Handle touch pan gesture.
     * @param pan Pan gesture with delta_x_px and delta_y_px
     * @param delta_time Time since last input in seconds
     */
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;

    /** Reset manipulator state and animation. */
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
     * @brief Set the world-space up vector (default: +Y).
     * @param world_up Unit-length up vector
     */
    void setWorldUp(const vne::math::Vec3f& world_up) noexcept;

    /**
     * @brief Set the zoom interaction method.
     * @param method Zoom method (dolly, scene scale, or FOV)
     */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }

    /**
     * @brief Get the current zoom method.
     * @return Current zoom method
     */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }

    /**
     * @brief Set the center of interest (pivot point) in world or camera space.
     * @param coi Center of interest position
     * @param space Whether coi is in world or camera space
     */
    void setCenterOfInterest(const vne::math::Vec3f& coi, CenterOfInterestSpace space) noexcept;

    /**
     * @brief Get the center of interest in world space.
     * @return Center of interest position in world coordinates
     */
    [[nodiscard]] vne::math::Vec3f getCenterOfInterestWorld() const noexcept { return coi_world_; }

    /**
     * @brief Get the distance from camera to center of interest.
     * @return Orbit distance in world units
     */
    [[nodiscard]] float getOrbitDistance() const noexcept { return orbit_distance_; }

    /**
     * @brief Set the distance from camera to center of interest.
     * @param distance Orbit distance in world units (clamped to > 0)
     */
    void setOrbitDistance(float distance) noexcept;
    /**
     * @brief Set the rotation speed multiplier (default: 1.0).
     * @param speed Speed multiplier (clamped to >= 0)
     */
    void setRotationSpeed(float speed) noexcept { rotation_speed_ = std::max(0.0f, speed); }

    /**
     * @brief Get the rotation speed multiplier.
     * @return Current rotation speed
     */
    [[nodiscard]] float getRotationSpeed() const noexcept { return rotation_speed_; }

    /**
     * @brief Set the pan speed multiplier (default: 1.0).
     * @param speed Speed multiplier (clamped to >= 0)
     */
    void setPanSpeed(float speed) noexcept { pan_speed_ = std::max(0.0f, speed); }

    /**
     * @brief Get the pan speed multiplier.
     * @return Current pan speed
     */
    [[nodiscard]] float getPanSpeed() const noexcept { return pan_speed_; }

    /**
     * @brief Set the zoom speed for scroll/pinch zoom (default: 1.5).
     * @param speed Zoom speed (clamped to >= 0.01)
     */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }

    /**
     * @brief Set the FOV zoom speed for scroll/pinch (perspective only, default: 1.5).
     * @param speed FOV zoom speed (clamped to >= 0.01)
     */
    void setFovZoomSpeed(float speed) noexcept { fov_zoom_speed_ = std::max(0.01f, speed); }

    /**
     * @brief Get the FOV zoom speed.
     * @return Current FOV zoom speed
     */
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }

    /**
     * @brief Set the rotation damping factor for inertia (default: 4.0).
     * @param damping Damping factor (clamped to >= 0)
     */
    void setRotationDamping(float damping) noexcept { rot_damping_ = std::max(0.0f, damping); }

    /**
     * @brief Get the rotation damping factor.
     * @return Current rotation damping
     */
    [[nodiscard]] float getRotationDamping() const noexcept { return rot_damping_; }

    /**
     * @brief Set the pan damping factor for momentum panning (default: 0.0).
     * @param damping Damping factor (clamped to >= 0)
     */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }

    /**
     * @brief Get the pan damping factor.
     * @return Current pan damping
     */
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /**
     * @brief Set the mouse button mapping for rotate/pan actions.
     * @param map Button map with rotate and pan button indices
     */
    void setButtonMap(const ButtonMap& map) noexcept { button_map_ = map; }

    /**
     * @brief Get the current mouse button mapping.
     * @return Reference to the current button map
     */
    [[nodiscard]] const ButtonMap& getButtonMap() const noexcept { return button_map_; }

   private:
    /** Compute forward direction vector from current orientation. */
    [[nodiscard]] vne::math::Vec3f computeFront() const noexcept override;
    /** Project screen pixel coordinates to a unit vector on the arcball sphere (camera-space). */
    [[nodiscard]] vne::math::Vec3f projectToArcball(float x_px, float y_px) const noexcept;
    /** Synchronize manipulator state from current camera properties. */
    void syncFromCamera() noexcept override;
    /** Apply manipulator state to update the camera. */
    void applyToCamera() noexcept override;
    /** Begin rotation from screen position. */
    void beginRotate(float x_px, float y_px) noexcept override;
    /** Handle rotation drag with pixel coordinates. */
    void dragRotate(float x_px, float y_px, double delta_time) noexcept override;
    /** End rotation and set up inertia-based momentum if enabled. */
    void endRotate(double delta_time) noexcept override;
    /** Apply rotational inertia decay based on damping. */
    void applyInertia(double delta_time) noexcept override;
    /** Called when the center of interest changes to update arcball. */
    void onPivotChanged() noexcept override;

    float arcball_start_x_ = 0.0f;  //!< Screen-space X coordinate where arcball rotation started (pixels)
    float arcball_start_y_ = 0.0f;  //!< Screen-space Y coordinate where arcball rotation started (pixels)

    vne::math::Quatf orientation_;    //!< Persistent accumulated rotation quaternion (canonical rotation state)
    uint32_t normalize_counter_ = 0;  //!< Counter for periodic quaternion normalization to prevent drift

    float inertia_rot_speed_ = 0.0f;     //!< Inertia angular velocity in radians per second
    vne::math::Vec3f inertia_rot_axis_;  //!< Inertia rotation axis in world space for momentum rotation
};

}  // namespace vne::interaction
