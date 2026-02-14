#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include <memory>

namespace vne::interaction {

class CameraManipulatorFactory {
public:
    CameraManipulatorFactory() = delete;

    [[nodiscard]] static std::unique_ptr<ICameraManipulator> create(CameraManipulatorType type) noexcept;
};

}  // namespace vne::interaction
