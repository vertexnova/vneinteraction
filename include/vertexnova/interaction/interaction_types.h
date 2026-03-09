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

#include <cstdint>

namespace vne::interaction {

enum class CameraManipulatorType : std::uint8_t {
    eOrbit = 0,
    eArcball = 1,
    eFps = 2,
    eFly = 3,
    eOrthoPanZoom = 4,
    eFollow = 5,
};

enum class CenterOfInterestSpace : std::uint8_t {
    eWorldSpace = 0,
    eCameraSpace = 1,
};

enum class ViewDirection : std::uint8_t {
    eFront = 0,
    eBack = 1,
    eLeft = 2,
    eRight = 3,
    eTop = 4,
    eBottom = 5,
    eIso = 6,
};

enum class ZoomMethod : std::uint8_t {
    eDollyToCoi = 0,
    eSceneScale = 1,
    eChangeFov = 2,
};

enum class UpAxis : std::uint8_t {
    eY = 0,
    eZ = 1,
};

enum class MouseButton : int {
    eLeft = 0,
    eRight = 1,
    eMiddle = 2,
};

/// Button indices for rotate/pan (e.g. left/right mouse). Used by orbit-style manipulators.
struct VNE_INTERACTION_API ButtonMap {
    int rotate = static_cast<int>(MouseButton::eLeft);
    int pan = static_cast<int>(MouseButton::eRight);
};

struct VNE_INTERACTION_API TouchPan final {
    float delta_x_px = 0.0f;
    float delta_y_px = 0.0f;
};

struct VNE_INTERACTION_API TouchPinch final {
    float scale = 1.0f;
    float center_x_px = 0.0f;
    float center_y_px = 0.0f;
};

}  // namespace vne::interaction
