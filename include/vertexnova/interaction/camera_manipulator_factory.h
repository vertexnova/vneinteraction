#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   yes
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
    /** Construct default factory. */
    CameraManipulatorFactory() = default;
    /** Destroy factory. */
    ~CameraManipulatorFactory() = default;

    /** Rule of Five: delete copy constructor (non-copyable). */
    CameraManipulatorFactory(const CameraManipulatorFactory&) = delete;
    /** Rule of Five: delete copy assignment operator (non-copyable). */
    CameraManipulatorFactory& operator=(const CameraManipulatorFactory&) = delete;

    /** Rule of Five: default move constructor (movable). */
    CameraManipulatorFactory(CameraManipulatorFactory&&) noexcept = default;
    /** Rule of Five: default move assignment operator (movable). */
    CameraManipulatorFactory& operator=(CameraManipulatorFactory&&) noexcept = default;

    /** Create a manipulator for the given type. Never returns nullptr for valid enum values. */
    [[nodiscard]] std::shared_ptr<ICameraManipulator> create(CameraManipulatorType type) const;
};

}  // namespace vne::interaction
