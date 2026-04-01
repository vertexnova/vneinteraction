/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 01: Library info — version query and camera manipulator overview.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/interaction.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/version.h"

#include "common/logging_guard.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    VNE_LOG_INFO << "VneInteraction " << vne::interaction::get_version();

    VNE_LOG_INFO << "Manipulators: OrbitalCameraManipulator (trackball / orbit), FreeLookManipulator (FPS / Fly), "
                 << "Ortho2DManipulator (orthographic 2D pan+zoom+rotate), FollowManipulator (smooth follow); "
                 << "orbit/trackball math: OrbitBehavior, TrackballBehavior";

    VNE_LOG_INFO << "InputMapper presets: orbitPreset(), fpsPreset(), gamePreset(), cadPreset(), orthoPreset()";

    VNE_LOG_INFO << "Controllers: Inspect3DController, Navigation3DController, Ortho2DController, FollowController";

    return 0;
}
