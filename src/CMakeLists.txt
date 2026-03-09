#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

set(SOURCE_FILES
    vertexnova/interaction/camera_manipulator_factory.cpp
    vertexnova/interaction/camera_system_controller.cpp
    vertexnova/interaction/orbit_manipulator.cpp
    vertexnova/interaction/arcball_manipulator.cpp
    vertexnova/interaction/fps_manipulator.cpp
    vertexnova/interaction/fly_manipulator.cpp
    vertexnova/interaction/ortho_pan_zoom_manipulator.cpp
    vertexnova/interaction/follow_manipulator.cpp
    vertexnova/interaction/version.cpp
)

set(HEADER_FILES
)

# One library per build: static or shared, always named vneinteraction (alias vne::interaction).
if(VNE_INTERACTION_LIB_TYPE STREQUAL "shared")
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(vneinteraction ${_lib_type} ${SOURCE_FILES} ${HEADER_FILES})

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
)

if(TARGET vne::common)
    target_link_libraries(vneinteraction PUBLIC vne::common)
endif()
if(TARGET vne::logging)
    target_link_libraries(vneinteraction PUBLIC vne::logging)
endif()
if(TARGET vne::math)
    target_link_libraries(vneinteraction PUBLIC vne::math)
endif()
if(TARGET vne::scene)
    target_link_libraries(vneinteraction PUBLIC vne::scene)
endif()

target_compile_features(vneinteraction PUBLIC cxx_std_20)

# Set export/import defines for shared builds. The header uses them only on Windows
# (VNE_INTERACTION_API becomes __declspec(dllexport)/dllimport); on other platforms they are no-ops.
if(_lib_type STREQUAL "SHARED")
    target_compile_definitions(vneinteraction
        PRIVATE VNE_INTERACTION_BUILDING_DLL
        INTERFACE VNE_INTERACTION_DLL
    )
endif()

add_library(vne::interaction ALIAS vneinteraction)

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})
