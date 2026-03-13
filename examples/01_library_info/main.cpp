/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 01: Library info — version query and behavior system overview.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/interaction.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/version.h"

#include "common/logging_guard.h"

#include <iostream>

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    std::cout << "VneInteraction " << vne::interaction::get_version() << "\n\n";

    std::cout << "Behaviors:\n"
              << "  - OrbitArcballBehavior   (arcball / Euler orbit)\n"
              << "  - FreeLookBehavior (FPS / Fly)\n"
              << "  - OrthoPanZoomBehavior  (orthographic 2D pan+zoom)\n"
              << "  - FollowBehavior   (smooth follow)\n\n";

    std::cout << "InputMapper presets:\n"
              << "  - orbitPreset()  (LMB=rotate, RMB=pan, scroll=zoom)\n"
              << "  - fpsPreset()    (RMB=look, WASD=move, scroll=zoom)\n"
              << "  - gamePreset()   (orbit + free-look hybrid)\n"
              << "  - cadPreset()    (MMB=pan, Shift+MMB=rotate)\n"
              << "  - orthoPreset()  (RMB/MMB=pan, scroll=zoom)\n\n";

    std::cout << "Controllers:\n"
              << "  - InspectController  (3D object inspection)\n"
              << "  - Navigation3DController (FPS/Fly/Game 3D traversal)\n"
              << "  - Ortho2DController     (orthographic 2D slices, maps)\n"
              << "  - FollowController   (end-effector follow-cam)\n";

    return 0;
}
