#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vneinteraction)
    return()
endif()

# 01 - Library Info: version query and manipulator capability listing
add_subdirectory(01_library_info)

# 02 - Orbit Manipulator: orbit drag, inertia, zoom, fitToAABB
add_subdirectory(02_orbit_manipulator)

# 03 - Arcball Manipulator: virtual-trackball drag, pan, inertia, fitToAABB
add_subdirectory(03_arcball_manipulator)

# 04 - FPS Manipulator: WASD movement, sprint, mouse-look
add_subdirectory(04_fps_manipulator)

# 05 - Fly Manipulator: 6-DOF free-flight with WASD+QE and mouse-look
add_subdirectory(05_fly_manipulator)

# 06 - OrthoPanZoom Manipulator: orthographic pan, scroll/pinch zoom
add_subdirectory(06_ortho_pan_zoom_manipulator)

# 07 - Follow Manipulator: static and dynamic target following with damping
add_subdirectory(07_follow_manipulator)

# 08 - CameraSystemController: facade with runtime manipulator hot-swap
add_subdirectory(08_camera_system_controller)
