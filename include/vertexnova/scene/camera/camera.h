#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/math/core/core.h>

#include <memory>

namespace vne::scene {

/**
 * @brief Minimal camera interface for use with interaction manipulators.
 * @threadsafe Not thread-safe; use from a single thread (e.g. main/render).
 */
class VNE_INTERACTION_API ICamera {
   public:
    virtual ~ICamera() = default;

    [[nodiscard]] virtual vne::math::Vec3f getPosition() const = 0;
    [[nodiscard]] virtual vne::math::Vec3f getTarget() const = 0;
    [[nodiscard]] virtual vne::math::Vec3f getUp() const = 0;

    virtual void setPosition(const vne::math::Vec3f& position) = 0;
    virtual void setTarget(const vne::math::Vec3f& target) = 0;
    virtual void setUp(const vne::math::Vec3f& up) = 0;
    virtual void updateMatrices() = 0;
};

}  // namespace vne::scene
