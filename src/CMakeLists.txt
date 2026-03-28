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
    vertexnova/interaction/version.cpp
    vertexnova/interaction/input_mapper.cpp
    vertexnova/interaction/behavior_utils.cpp
    vertexnova/interaction/camera_behavior_base.cpp
    vertexnova/interaction/trackball_behavior.cpp
    vertexnova/interaction/orbit_behavior.cpp
    vertexnova/interaction/orbit_arcball_behavior.cpp
    vertexnova/interaction/free_look_behavior.cpp
    vertexnova/interaction/ortho_pan_zoom_behavior.cpp
    vertexnova/interaction/follow_behavior.cpp
    vertexnova/interaction/camera_rig.cpp
    vertexnova/interaction/inspect_controller.cpp
    vertexnova/interaction/navigation_3d_controller.cpp
    vertexnova/interaction/ortho_2d_controller.cpp
    vertexnova/interaction/follow_controller.cpp
    vertexnova/interaction/controller_event_dispatch.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/export.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/version.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/interaction_types.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_behavior_base.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/trackball_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/orbit_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_rig.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/orbit_arcball_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/free_look_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/ortho_pan_zoom_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/follow_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/input_mapper.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/inspect_controller.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/navigation_3d_controller.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/ortho_2d_controller.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/follow_controller.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/interaction.h
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

# Link vne::events for the event-based API (onEvent).
if(TARGET vne::events)
    target_link_libraries(vneinteraction PUBLIC vne::events)
endif()

# Link only to vne::scene; it brings vne::math, vne::common, vne::logging transitively.
if(TARGET vne::scene)
    target_link_libraries(vneinteraction PUBLIC vne::scene)
else()
    if(TARGET vne::common)
        target_link_libraries(vneinteraction PUBLIC vne::common)
    endif()
    if(TARGET vne::logging)
        target_link_libraries(vneinteraction PUBLIC vne::logging)
    endif()
    if(TARGET vne::math)
        target_link_libraries(vneinteraction PUBLIC vne::math)
    endif()
endif()

target_compile_features(vneinteraction PUBLIC cxx_std_20)

if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "visionOS")
    target_link_libraries(vneinteraction PUBLIC "-framework Foundation")
endif()

if(_lib_type STREQUAL "SHARED")
    target_compile_definitions(vneinteraction
        PRIVATE VNE_INTERACTION_BUILDING_DLL
        INTERFACE VNE_INTERACTION_DLL
    )
    if(MSVC)
        target_compile_options(vneinteraction PRIVATE /wd4251 /wd4275)
    endif()
endif()

add_library(vne::interaction ALIAS vneinteraction)

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})
