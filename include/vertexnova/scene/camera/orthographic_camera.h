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
 * @brief Orthographic camera with width/height (ortho size).
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API OrthographicCamera : public ICamera {
   public:
    OrthographicCamera() = default;
    ~OrthographicCamera() override = default;

    [[nodiscard]] vne::math::Vec3f getPosition() const override { return position_; }
    [[nodiscard]] vne::math::Vec3f getTarget() const override { return target_; }
    [[nodiscard]] vne::math::Vec3f getUp() const override { return up_; }

    void setPosition(const vne::math::Vec3f& position) override { position_ = position; }
    void setTarget(const vne::math::Vec3f& target) override { target_ = target; }
    void setUp(const vne::math::Vec3f& up) override { up_ = up; }
    void updateMatrices() override {}

    [[nodiscard]] float getWidth() const noexcept { return width_; }
    [[nodiscard]] float getHeight() const noexcept { return height_; }
    void setWidth(float w) noexcept { width_ = w; }
    void setHeight(float h) noexcept { height_ = h; }

    void setBounds(float left, float right, float bottom, float top, float near_plane, float far_plane) noexcept {
        width_ = right - left;
        height_ = top - bottom;
        near_plane_ = near_plane;
        far_plane_ = far_plane;
    }
    [[nodiscard]] float getNearPlane() const noexcept { return near_plane_; }
    [[nodiscard]] float getFarPlane() const noexcept { return far_plane_; }

   private:
    vne::math::Vec3f position_{0.0f, 0.0f, 5.0f};
    vne::math::Vec3f target_{0.0f, 0.0f, 0.0f};
    vne::math::Vec3f up_{0.0f, 1.0f, 0.0f};
    float width_ = 10.0f;
    float height_ = 10.0f;
    float near_plane_ = 0.1f;
    float far_plane_ = 1000.0f;
};

}  // namespace vne::scene
