#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/detail/free_camera_base.h"

#include <vertexnova/math/core/core.h>

#include <algorithm>
#include <memory>

namespace vne::interaction {

class VNE_INTERACTION_API FpsManipulator final : public FreeCameraBase {
   public:
    FpsManipulator() noexcept;
    ~FpsManipulator() noexcept override = default;

    [[nodiscard]] bool supportsPerspective() const noexcept override { return true; }
    [[nodiscard]] bool supportsOrthographic() const noexcept override { return true; }

    void setViewportSize(float width_px, float height_px) noexcept override;

    void resetState() noexcept override;
    void fitToAABB(const vne::math::Vec3f& min_world, const vne::math::Vec3f& max_world) noexcept override;
    [[nodiscard]] float getWorldUnitsPerPixel() const noexcept override;

    void setZoomMethod(ZoomMethod method) noexcept { zoom_method_ = method; }
    [[nodiscard]] ZoomMethod getZoomMethod() const noexcept { return zoom_method_; }
    void setWorldUp(const vne::math::Vec3f& up) noexcept;
    void setMoveSpeed(float units_per_sec) noexcept { move_speed_ = std::max(0.0f, units_per_sec); }
    [[nodiscard]] float getMoveSpeed() const noexcept { return move_speed_; }
    void setMouseSensitivity(float deg_per_pixel) noexcept { mouse_sensitivity_ = std::max(0.0f, deg_per_pixel); }
    [[nodiscard]] float getMouseSensitivity() const noexcept { return mouse_sensitivity_; }
    void setZoomSpeed(float units_or_factor) noexcept { zoom_speed_ = std::max(0.01f, units_or_factor); }
    [[nodiscard]] float getZoomSpeed() const noexcept { return zoom_speed_; }
    void setFovZoomSpeed(float factor) noexcept { fov_zoom_speed_ = std::max(0.01f, factor); }
    [[nodiscard]] float getFovZoomSpeed() const noexcept { return fov_zoom_speed_; }
    void setSprintMultiplier(float mult) noexcept { sprint_mult_ = std::max(1.0f, mult); }
    [[nodiscard]] float getSprintMultiplier() const noexcept { return sprint_mult_; }
    void setSlowMultiplier(float mult) noexcept { slow_mult_ = std::clamp(mult, 0.0f, 1.0f); }
    [[nodiscard]] float getSlowMultiplier() const noexcept { return slow_mult_; }

   private:
    [[nodiscard]] vne::math::Vec3f upVector() const noexcept override;

    vne::math::Vec3f world_up_{0.0f, 1.0f, 0.0f};
};

}  // namespace vne::interaction
