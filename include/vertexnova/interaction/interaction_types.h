#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2025 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include <cstdint>

namespace vne::interaction {

enum class CameraManipulatorType : std::uint8_t {
    eOrbitArcball = 0,
    eFpsFly = 1,
    eOrthoPanZoom = 2,
    eFollow = 3,
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

struct TouchPan final {
    float delta_x_px = 0.0f;
    float delta_y_px = 0.0f;
};

struct TouchPinch final {
    float scale = 1.0f;
    float center_x_px = 0.0f;
    float center_y_px = 0.0f;
};

}  // namespace vne::interaction
