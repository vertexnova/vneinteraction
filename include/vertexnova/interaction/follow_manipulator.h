#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/scene/camera/camera.h"
#include <vertexnova/math/core/core.h>
#include <functional>
#include <memory>

namespace vne::interaction {

class FollowManipulator final : public ICameraManipulator {
public:
    using TargetProvider = std::function<vne::math::Vec3f()>;

    FollowManipulator() noexcept = default;
    ~FollowManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) noexcept override;
    void setEnabled(bool enabled) noexcept override { enabled_ = enabled; }
    void setViewportSize(float width_px, float height_px) noexcept override;
    void update(double delta_time) noexcept override;

    void handleMouseMove(float x, float y, float delta_x, float delta_y, double delta_time) noexcept override;
    void handleMouseButton(int button, bool pressed, float x, float y, double delta_time) noexcept override;
    void handleMouseScroll(float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    void setTargetPosition(const vne::math::Vec3f& target_world) noexcept { target_world_ = target_world; }
    void setTargetProvider(TargetProvider provider) noexcept { target_provider_ = std::move(provider); }
    void setOffsetWorld(const vne::math::Vec3f& offset) noexcept { offset_world_ = offset; }
    void setDamping(float damping) noexcept { damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getDamping() const noexcept { return damping_; }
    [[nodiscard]] vne::math::Vec3f getOffsetWorld() const noexcept { return offset_world_; }

private:
    [[nodiscard]] vne::math::Vec3f getTargetWorld() const noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    vne::math::Vec3f target_world_{0.0f, 0.0f, 0.0f};
    TargetProvider target_provider_;
    vne::math::Vec3f offset_world_{0.0f, 1.0f, 5.0f};
    float damping_ = 8.0f;
};

}  // namespace vne::interaction
