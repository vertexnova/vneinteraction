#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/scene/camera/camera.h"

#include <vertexnova/math/core/core.h>

namespace vne::scene {

/**
 * @brief Perspective (projection) camera with FOV and viewport.
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API PerspectiveCamera : public ICamera {
   public:
    PerspectiveCamera() = default;
    ~PerspectiveCamera() override = default;

    [[nodiscard]] vne::math::Vec3f getPosition() const override { return position_; }
    [[nodiscard]] vne::math::Vec3f getTarget() const override { return target_; }
    [[nodiscard]] vne::math::Vec3f getUp() const override { return up_; }

    void setPosition(const vne::math::Vec3f& position) override { position_ = position; }
    void setTarget(const vne::math::Vec3f& target) override { target_ = target; }
    void setUp(const vne::math::Vec3f& up) override { up_ = up; }
    void updateMatrices() override {}

    [[nodiscard]] float getFieldOfView() const noexcept { return field_of_view_deg_; }
    void setFieldOfView(float fov_deg) noexcept { field_of_view_deg_ = fov_deg; }
    void setViewport(float width_px, float height_px) noexcept {
        viewport_width_ = width_px;
        viewport_height_ = height_px;
    }
    [[nodiscard]] float getViewportWidth() const noexcept { return viewport_width_; }
    [[nodiscard]] float getViewportHeight() const noexcept { return viewport_height_; }

   private:
    vne::math::Vec3f position_{0.0f, 0.0f, 5.0f};
    vne::math::Vec3f target_{0.0f, 0.0f, 0.0f};
    vne::math::Vec3f up_{0.0f, 1.0f, 0.0f};
    float field_of_view_deg_ = 60.0f;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
};

}  // namespace vne::scene
