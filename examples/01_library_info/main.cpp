/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Example 01: Library info — version, enumerate all public types and
 * InputMapper presets by actually calling the API.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_rig.h"
#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/interaction.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/version.h"

#include "common/logging_guard.h"

int main() {
    vne::interaction::examples::LoggingGuard logging_guard;

    // ── Version ───────────────────────────────────────────────────────────────
    VNE_LOG_INFO << "VneInteraction version: " << vne::interaction::get_version();

    // ── Controllers ──────────────────────────────────────────────────────────
    // Instantiate each controller to confirm linkage and default construction.
    {
        vne::interaction::Inspect3DController c;
        VNE_LOG_INFO << "Inspect3DController: rotation_mode="
                     << (c.getRotationMode() == vne::interaction::OrbitalRotationMode::eOrbit ? "eOrbit" : "eTrackball")
                     << "  pivot_mode="
                     << (c.getPivotMode() == vne::interaction::OrbitPivotMode::eCoi     ? "eCoi"
                         : c.getPivotMode() == vne::interaction::OrbitPivotMode::eFixed ? "eFixed"
                                                                                        : "eViewCenter")
                     << "  rotation_enabled=" << c.isRotationEnabled()
                     << "  double_click_pivot=" << c.isPivotOnDoubleClickEnabled();
    }
    {
        vne::interaction::Navigation3DController c;
        VNE_LOG_INFO << "Navigation3DController: mode="
                     << (c.getMode() == vne::interaction::FreeLookMode::eFps ? "eFps" : "eFly")
                     << "  move_speed=" << c.getMoveSpeed() << "  sensitivity=" << c.getMouseSensitivity()
                     << "  sprint=" << c.getSprintMultiplier() << "  slow=" << c.getSlowMultiplier()
                     << "  look=" << c.isLookEnabled() << "  move=" << c.isMoveEnabled()
                     << "  zoom=" << c.isZoomEnabled();
    }
    {
        vne::interaction::Ortho2DController c;
        VNE_LOG_INFO << "Ortho2DController: rotation_enabled=" << c.isRotationEnabled();
    }
    {
        vne::interaction::FollowController c;
        VNE_LOG_INFO << "FollowController: lag=" << c.getLag() << "  offset=(" << c.getOffset().x() << ","
                     << c.getOffset().y() << "," << c.getOffset().z() << ")";
    }

    // ── ZoomMethod enum ───────────────────────────────────────────────────────
    // All three methods are valid on every manipulator (via CameraManipulatorBase).
    VNE_LOG_INFO << "ZoomMethod values: eSceneScale=" << static_cast<int>(vne::interaction::ZoomMethod::eSceneScale)
                 << "  eChangeFov=" << static_cast<int>(vne::interaction::ZoomMethod::eChangeFov)
                 << "  eDollyToCoi=" << static_cast<int>(vne::interaction::ZoomMethod::eDollyToCoi);

    // ── InputMapper presets ───────────────────────────────────────────────────
    // Each preset returns a vector<InputRule>; size confirms the preset is non-empty.
    const auto orbit = vne::interaction::InputMapper::orbitPreset();
    const auto fps = vne::interaction::InputMapper::fpsPreset();
    const auto game = vne::interaction::InputMapper::gamePreset();
    const auto cad = vne::interaction::InputMapper::cadPreset();
    const auto ortho = vne::interaction::InputMapper::orthoPreset();

    VNE_LOG_INFO << "InputMapper presets (rule counts):"
                 << "  orbit=" << orbit.size() << "  fps=" << fps.size() << "  game=" << game.size()
                 << "  cad=" << cad.size() << "  ortho=" << ortho.size();

    // ── CameraRig factory methods ─────────────────────────────────────────────
    auto rig_orbit = vne::interaction::CameraRig::makeOrbit();
    auto rig_tb = vne::interaction::CameraRig::makeTrackball();
    auto rig_fps = vne::interaction::CameraRig::makeFps();
    auto rig_fly = vne::interaction::CameraRig::makeFly();
    auto rig_ortho = vne::interaction::CameraRig::makeOrtho2D();
    auto rig_follow = vne::interaction::CameraRig::makeFollow();

    VNE_LOG_INFO << "CameraRig factories — manipulator counts:"
                 << "  orbit=" << rig_orbit.manipulators().size() << "  trackball=" << rig_tb.manipulators().size()
                 << "  fps=" << rig_fps.manipulators().size() << "  fly=" << rig_fly.manipulators().size()
                 << "  ortho2d=" << rig_ortho.manipulators().size() << "  follow=" << rig_follow.manipulators().size();

    // ── Rotation / pivot mode enums ───────────────────────────────────────────
    VNE_LOG_INFO << "OrbitalRotationMode:  eOrbit=" << static_cast<int>(vne::interaction::OrbitalRotationMode::eOrbit)
                 << "  eTrackball=" << static_cast<int>(vne::interaction::OrbitalRotationMode::eTrackball);
    VNE_LOG_INFO << "TrackballProjectionMode:  eHyperbolic="
                 << static_cast<int>(vne::interaction::TrackballProjectionMode::eHyperbolic)
                 << "  eRim=" << static_cast<int>(vne::interaction::TrackballProjectionMode::eRim);
    VNE_LOG_INFO << "OrbitPivotMode:  eCoi=" << static_cast<int>(vne::interaction::OrbitPivotMode::eCoi)
                 << "  eViewCenter=" << static_cast<int>(vne::interaction::OrbitPivotMode::eViewCenter)
                 << "  eFixed=" << static_cast<int>(vne::interaction::OrbitPivotMode::eFixed);
    VNE_LOG_INFO << "FreeLookMode:  eFps=" << static_cast<int>(vne::interaction::FreeLookMode::eFps)
                 << "  eFly=" << static_cast<int>(vne::interaction::FreeLookMode::eFly);
    VNE_LOG_INFO << "ViewDirection:  eFront/eBack/eLeft/eRight/eTop/eBottom/eIso = 0..6";

    VNE_LOG_INFO << "01_library_info: done.";
    return 0;
}
