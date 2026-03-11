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
#include "vertexnova/interaction/detail/camera_manipulator_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::scene {
class OrthographicCamera;
}

namespace vne::interaction {

/**
 * @brief Orthographic projection camera manipulator with pan and zoom.
 *
 * Specialized for orthographic (2D/technical) views providing intuitive panning
 * and zooming without perspective effects. Designed for technical/2D work:
 * - Architectural drawings
 * - Technical blueprints
 * - 2D game development
 * - UI layering
 *
 * Features:
 * - Pan: Move the view by dragging (mouse button configurable)
 * - Zoom: Scale the camera.height via scroll wheel or pinch gestures
 * - Zoom-to-cursor: Maintains cursor position on screen when zooming
 * - Inertia-based momentum panning for smooth interaction
 * - Fit-to-view support for framing the entire scene
 *
 * Orthographic cameras only. Zoom to cursor preserves screen-space position,
 * keeping the point under the cursor stationary as zoom magnitude changes.
 *
 * @threadsafe Not thread-safe. All methods must be called from a single thread.
 * @see ICameraManipulator, CameraManipulatorBase
 */
class VNE_INTERACTION_API OrthoPanZoomManipulator final : public CameraManipulatorBase {
   public:
    /** Construct orthographic pan-zoom manipulator with default settings. */
    OrthoPanZoomManipulator() noexcept = default;
    /** Destroy pan-zoom manipulator. */
    ~OrthoPanZoomManipulator() noexcept override = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    OrthoPanZoomManipulator(const OrthoPanZoomManipulator&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    OrthoPanZoomManipulator& operator=(const OrthoPanZoomManipulator&) = delete;

    /** Rule of Five: default move constructor (movable). */
    OrthoPanZoomManipulator(OrthoPanZoomManipulator&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    OrthoPanZoomManipulator& operator=(OrthoPanZoomManipulator&&) noexcept = default;

    /** Check if this manipulator supports perspective projection (always false for ortho). */
    [[nodiscard]] bool supportsPerspective() const noexcept override { return false; }
    /** Check if this manipulator supports orthographic projection (always true). */
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    /** Set the orthographic camera to manipulate. */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    /** Set the viewport dimensions in pixels. */
    void setViewportSize(float width_px, float height_px) noexcept override;
    /** Update the camera based on accumulated pan inertia. */
    void update(double delta_time) noexcept override;
    /** Apply a semantic camera command to this manipulator. */
    void applyCommand(CameraActionType action,
                      const CameraCommandPayload& payload,
                      double delta_time) noexcept override;

    /** Handle mouse movement input for panning. */
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    /** Handle mouse button press/release for pan start/end. */
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    /** Handle mouse scroll wheel input for zoom. */
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    /** Handle keyboard input (forwarded from controller). */
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    /** Handle touch pan gesture. */
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    /** Handle touch pinch (zoom) gesture. */
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    /** Set the zoom method (for orthographic, typically scene scale). */
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    /** Get the current zoom method. */
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    /** Set the zoom speed for scroll/pinch events (default: 1.2). */
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    /** Set the panning damping factor for inertia (default: 10.0). */
    void setPanDamping(float damping) noexcept { pan_damping_ = std::max(0.0f, damping); }
    /** Get the panning damping factor. */
    [[nodiscard]] float getPanDamping() const noexcept { return pan_damping_; }

    /** Reset manipulator state and damping. */
    void resetState() noexcept override;
    /** Adjust camera to frame the given bounding box. */
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    /** Get the current scene scale value for CSS/scaling operations. */
    [[nodiscard]] float getSceneScale() const noexcept override { return scene_scale_; }
    /** Get world units per pixel for screen-to-world conversions. */
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

   private:
    /** Get the orthographic camera (cast from camera_ with validity check). */
    [[nodiscard]] std::shared_ptr<vne::scene::OrthographicCamera> getOrtho() const noexcept;
    /** Apply panning motion to camera position. */
    void pan(float delta_x_px, float delta_y_px, double delta_time) noexcept;
    /** Zoom the camera while keeping the cursor position fixed on screen. */
    void zoomToCursor(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;
    /** Apply pan velocity decay based on damping. */
    void applyInertia(double delta_time) noexcept;
    /** Apply zoom adjustment using the configured zoom method. */
    void applyZoom(float zoom_factor, float mouse_x_px, float mouse_y_px) noexcept;

    bool panning_ = false;                             //!< Whether the mouse button is currently held for panning
    vne::math::Vec3f pan_velocity_{0.0f, 0.0f, 0.0f};  //!< Current pan velocity in world space units
    float pan_damping_ = 10.0f;  //!< Damping factor for panning inertia (exponential decay per second)
};

}  // namespace vne::interaction
