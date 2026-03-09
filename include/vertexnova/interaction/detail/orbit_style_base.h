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
 * Non-exported base for orbit-style manipulators (Orbit, Arcball).
 * Shared pan/zoom, fitToAABB, getWorldUnitsPerPixel, input dispatch.
 * Subclasses implement computeFront(), syncFromCamera(), applyToCamera(),
 * beginRotate(), dragRotate(), endRotate(), and override applyInertia() for rotation.
 */
class OrbitStyleBase : public CameraManipulatorBase {
   protected:
    vne::math::Vec3f world_up_;
    vne::math::Vec3f coi_world_;
    float orbit_distance_ = 5.0f;
    float fov_zoom_speed_ = 1.05f;
    ButtonMap button_map_;
    bool rotating_ = false;
    bool panning_ = false;
    float last_x_ = 0.0f;
    float last_y_ = 0.0f;
    vne::math::Vec3f inertia_pan_velocity_;
    float rot_damping_ = 8.0f;
    float pan_damping_ = 10.0f;
    float rotation_speed_ = 0.2f;
    float pan_speed_ = 1.0f;
    bool shift_ = false;

    OrbitStyleBase() = default;
    ~OrbitStyleBase() override = default;

    [[nodiscard]] virtual vne::math::Vec3f computeFront() const noexcept = 0;
    virtual void syncFromCamera() noexcept = 0;
    virtual void applyToCamera() noexcept = 0;
    virtual void beginRotate(float x_px, float y_px) noexcept = 0;
    virtual void dragRotate(float delta_x_px, float delta_y_px, double delta_time) noexcept = 0;
    virtual void endRotate(double delta_time) noexcept = 0;

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
    void update(double delta_time) noexcept override;
    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;
    void resetState() noexcept override;
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;
};

}  // namespace vne::interaction
