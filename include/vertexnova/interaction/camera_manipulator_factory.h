#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/export.h"
#include "vertexnova/interaction/interaction_types.h"

#include <memory>

namespace vne::interaction {

/**
 * @brief Factory that creates camera manipulators by type.
 *
 * create() returns a non-null shared_ptr for every value defined in CameraManipulatorType.
 * Passing an out-of-range integer cast to CameraManipulatorType is undefined behaviour
 * and will trigger an assertion in debug builds.
 * @threadsafe Not thread-safe.
 */
class VNE_INTERACTION_API CameraManipulatorFactory {
   public:
    CameraManipulatorFactory() = default;
    ~CameraManipulatorFactory() = default;

    /// Create a manipulator for the given type. Never returns nullptr for valid enum values.
    [[nodiscard]] std::shared_ptr<ICameraManipulator> create(CameraManipulatorType type) const;
};

}  // namespace vne::interaction
