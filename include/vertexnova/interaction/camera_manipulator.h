#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @file camera_manipulator.h
 * @brief Interface for camera manipulators (orbit, arcball, FPS, fly, etc.).
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <memory>

namespace vne::scene {
class ICamera;
}

namespace vne::interaction {

/**
 * @brief Abstract interface for camera manipulators (orbit, arcball, FPS, etc.).
 *
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API ICameraManipulator {
   public:
    virtual ~ICameraManipulator() = default;

    /**
     * @brief Check if this manipulator supports perspective projection.
     * @return true if perspective cameras are supported
     */
    [[nodiscard]] virtual bool supportsPerspective() const noexcept = 0;

    /**
     * @brief Check if this manipulator supports orthographic projection.
     * @return true if orthographic cameras are supported
     */
    [[nodiscard]] virtual bool supportsOrthographic() const noexcept = 0;

    /**
     * @brief Set the camera to manipulate.
     * @param camera Shared pointer to the camera (may be nullptr to detach)
     */
    virtual void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept = 0;

    /**
     * @brief Enable or disable this manipulator.
     * @param enabled true to enable, false to disable
     */
    virtual void setEnabled(bool enabled) noexcept = 0;

    /**
     * @brief Check if this manipulator is enabled.
     * @return true if enabled
     */
    [[nodiscard]] virtual bool isEnabled() const noexcept = 0;

    /**
     * @brief Set the viewport dimensions in pixels.
     * @param width_px Viewport width in pixels
     * @param height_px Viewport height in pixels
     */
    virtual void setViewportSize(float width_px, float height_px) noexcept = 0;

    /**
     * @brief Update the camera based on accumulated state (damping, inertia, etc.).
     * @param delta_time Time since last update in seconds
     */
    virtual void update(double delta_time) noexcept = 0;

    /**
     * @brief Handle mouse movement input.
     * @param x Current cursor X position in viewport pixels
     * @param y Current cursor Y position in viewport pixels
     * @param delta_x Horizontal movement delta in pixels
     * @param delta_y Vertical movement delta in pixels
     * @param delta_time Time since last input in seconds
     */
    virtual void onMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept = 0;

    /**
     * @brief Handle mouse button press or release.
     * @param button Mouse button index (0=left, 1=right, 2=middle)
     * @param pressed true if pressed, false if released
     * @param x Cursor X position in viewport pixels
     * @param y Cursor Y position in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    virtual void onMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept = 0;

    /**
     * @brief Handle mouse scroll wheel input.
     * @param scroll_x Horizontal scroll delta
     * @param scroll_y Vertical scroll delta
     * @param mouse_x Cursor X position in viewport pixels
     * @param mouse_y Cursor Y position in viewport pixels
     * @param delta_time Time since last input in seconds
     */
    virtual void onMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept = 0;

    /**
     * @brief Handle keyboard input.
     * @param key Key code (e.g. GLFW key code)
     * @param pressed true if pressed, false if released
     * @param delta_time Time since last input in seconds
     */
    virtual void onKeyboard(int key, bool pressed, double delta_time) noexcept = 0;

    /**
     * @brief Handle touch pan gesture.
     * @param pan Pan gesture data with delta_x_px and delta_y_px
     * @param delta_time Time since last input in seconds
     */
    virtual void onTouchPan(const TouchPan& pan, double delta_time) noexcept = 0;

    /**
     * @brief Handle touch pinch (zoom) gesture.
     * @param pinch Pinch gesture data with scale and center position
     * @param delta_time Time since last input in seconds
     */
    virtual void onTouchPinch(const TouchPinch& pinch, double delta_time) noexcept = 0;

    /**
     * @brief Get the current scene scale value.
     * @return Scene scale factor (typically 1.0 for manipulators that do not use scene scale)
     */
    [[nodiscard]] virtual float getSceneScale() const noexcept = 0;

    /**
     * @brief Get the zoom speed used for scroll/pinch events (> 1.0).
     * @return Zoom speed factor
     */
    [[nodiscard]] virtual float getZoomSpeed() const noexcept = 0;

    /**
     * @brief Reset manipulator state and damping.
     */
    virtual void resetState() noexcept = 0;

    /**
     * @brief Adjust camera to frame the given axis-aligned bounding box.
     * @param min_world Minimum corner of the AABB in world space
     * @param max_world Maximum corner of the AABB in world space
     */
    virtual void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept = 0;

    /**
     * @brief Get world units per pixel for screen-to-world conversions.
     * @return World units per pixel at the center of the viewport
     */
    [[nodiscard]] virtual float getWorldUnitsPerPixel() const noexcept = 0;

    /**
     * @brief Apply a semantic camera command (intent-based input, remappable, testable).
     * @param action The action type (e.g. eRotateDelta, ePanDelta)
     * @param payload Payload with position, deltas, zoom factor, etc.
     * @param delta_time Time since last input in seconds
     */
    virtual void applyCommand(CameraActionType action,
                              const CameraCommandPayload& payload,
                              double delta_time) noexcept = 0;
};

}  // namespace vne::interaction
