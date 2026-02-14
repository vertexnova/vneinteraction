#==============================================================================
# Copyright (c) 2024 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

set(SOURCE_FILES
    vertexnova/interaction/camera_manipulator_factory.cpp
    vertexnova/interaction/camera_system_controller.cpp
    vertexnova/interaction/orbit_arcball_manipulator.cpp
    vertexnova/interaction/fps_fly_manipulator.cpp
    vertexnova/interaction/ortho_pan_zoom_manipulator.cpp
    vertexnova/interaction/follow_manipulator.cpp
)

add_library(vneinteraction STATIC ${SOURCE_FILES})

add_library(vne::interaction ALIAS vneinteraction)

target_include_directories(vneinteraction
    PUBLIC
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${VNE_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(vneinteraction
    PUBLIC
        vne::interaction::Warnings
        vne::interaction::BuildSettings
        vne::scene
        vne::math
)

if(TARGET vne::common)
    target_link_libraries(vneinteraction PUBLIC vne::common)
endif()

target_compile_features(vneinteraction PUBLIC cxx_std_20)
