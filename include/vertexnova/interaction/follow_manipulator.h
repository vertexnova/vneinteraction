#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/math/core/core.h>

#include <functional>
#include <memory>

namespace vne::interaction {

class VNE_INTERACTION_API FollowManipulator final : public ICameraManipulator {
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
    void handleMouseScroll(
        float scroll_x, float scroll_y, float mouse_x, float mouse_y, double delta_time) noexcept override;
    void handleKeyboard(int key, bool pressed, double delta_time) noexcept override;
    void handleTouchPan(const TouchPan& pan, double delta_time) noexcept override;
    void handleTouchPinch(const TouchPinch& pinch, double delta_time) noexcept override;

    void resetState() noexcept override {}
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getSceneScale() const noexcept override { return scene_scale_; }
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    void setTargetWorld(const vne::math::Vec3f& target) noexcept { target_world_ = target; }
    void setTargetProvider(TargetProvider provider) noexcept { target_provider_ = std::move(provider); }
    [[nodiscard]] vne::math::Vec3f getTargetWorld() const noexcept;

    void setOffsetWorld(const vne::math::Vec3f& offset) noexcept { offset_world_ = offset; }
    [[nodiscard]] const vne::math::Vec3f& getOffsetWorld() const noexcept { return offset_world_; }
    void setDamping(float damping) noexcept { damping_ = std::max(0.0f, damping); }
    [[nodiscard]] float getDamping() const noexcept { return damping_; }
    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    void setZoomSpeed(float speed) noexcept { zoom_speed_ = std::max(0.01f, speed); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }

   private:
    void applyZoom(float zoom_factor) noexcept;

    std::shared_ptr<vne::scene::ICamera> camera_;
    bool enabled_ = true;
    float viewport_width_ = 1280.0f;
    float viewport_height_ = 720.0f;
    vne::math::Vec3f target_world_{0.0f, 0.0f, 0.0f};
    vne::math::Vec3f offset_world_{0.0f, 0.0f, 5.0f};
    TargetProvider target_provider_;
    float damping_ = 8.0f;
    float zoom_speed_ = 1.1f;
    float scene_scale_ = 1.0f;
    ZoomMethod zoom_method_ = ZoomMethod::eDollyToCoi;
};

}  // namespace vne::interaction
