#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include <vertexnova/math/core/constants.h>

namespace vne::interaction {

[[nodiscard]] inline float degToRad(float deg) noexcept {
    return deg * (vne::math::kPi / 180.0f);
}
[[nodiscard]] inline float radToDeg(float rad) noexcept {
    return rad * (180.0f / vne::math::kPi);
}

}  // namespace vne::interaction
