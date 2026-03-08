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

#include "common/logging_guard.h"
#include "vertexnova/template/template.h"

int main() {
    vne::template_ns::examples::LoggingGuard logging_guard;

    using namespace vne::template_ns;

    VNE_LOG_INFO << hello();
    VNE_LOG_INFO << "Version: " << get_version();

    return 0;
}
