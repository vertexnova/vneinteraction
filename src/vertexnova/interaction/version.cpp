/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/version.h"
#include "config.h"

namespace vne::interaction {

const char* get_version() {
    return PROJECT_VERSION;
}

}  // namespace vne::interaction
