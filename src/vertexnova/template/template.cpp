/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/template/template.h"
#include "config.h"

namespace vne {
namespace template_ns {

const char* get_version() {
    return PROJECT_VERSION;
}

const char* hello() {
    return "Hello from VneTemplate";
}

}  // namespace template_ns
}  // namespace vne
