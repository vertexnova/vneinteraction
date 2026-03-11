#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

/**
 * @file version.h
 * @brief Library version query API.
 */

#include "vertexnova/interaction/export.h"

namespace vne::interaction {

/**
 * @brief Returns the project version string.
 * @return Version string (e.g. "1.0.0")
 */
VNE_INTERACTION_API const char* get_version();

}  // namespace vne::interaction
