/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 01 - Library Info
 * Demonstrates: querying the library version and listing all available
 * manipulator types via CameraManipulatorFactory.
 * ----------------------------------------------------------------------
 */

#include "common/logging_guard.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/version.h"

#include <array>
#include <string_view>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // --- Version info ---
    VNE_LOG_INFO << "VneInteraction version: " << vne::interaction::get_version();

    // --- Factory: create every manipulator type and report capabilities ---
    const vne::interaction::CameraManipulatorFactory factory;

    struct TypeInfo {
        vne::interaction::CameraManipulatorType type;
        std::string_view name;
    };
    static constexpr std::array types{
        TypeInfo{vne::interaction::CameraManipulatorType::eOrbit, "Orbit"},
        TypeInfo{vne::interaction::CameraManipulatorType::eArcball, "Arcball"},
        TypeInfo{vne::interaction::CameraManipulatorType::eFps, "FPS"},
        TypeInfo{vne::interaction::CameraManipulatorType::eFly, "Fly"},
        TypeInfo{vne::interaction::CameraManipulatorType::eOrthoPanZoom, "OrthoPanZoom"},
        TypeInfo{vne::interaction::CameraManipulatorType::eFollow, "Follow"},
    };

    VNE_LOG_INFO << "Available manipulators:";
    for (const auto& info : types) {
        auto manipulator = factory.create(info.type);
        if (!manipulator) {
            VNE_LOG_INFO << "  [" << info.name << "] creation failed";
            continue;
        }
        VNE_LOG_INFO << "  [" << info.name << "]"
                     << "  perspective=" << (manipulator->supportsPerspective() ? "yes" : "no")
                     << "  orthographic=" << (manipulator->supportsOrthographic() ? "yes" : "no")
                     << "  sceneScale=" << manipulator->getSceneScale();
    }

    return 0;
}
