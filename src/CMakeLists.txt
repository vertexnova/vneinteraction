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
    vertexnova/interaction/camera_input_adapter.cpp
    vertexnova/interaction/camera_manipulator_base.cpp
    vertexnova/interaction/camera_manipulator_factory.cpp
    vertexnova/interaction/camera_system_controller.cpp
    vertexnova/interaction/orbit_style_base.cpp
    vertexnova/interaction/free_camera_base.cpp
    vertexnova/interaction/orbit_manipulator.cpp
    vertexnova/interaction/arcball_manipulator.cpp
    vertexnova/interaction/fps_manipulator.cpp
    vertexnova/interaction/fly_manipulator.cpp
    vertexnova/interaction/ortho_pan_zoom_manipulator.cpp
    vertexnova/interaction/follow_manipulator.cpp
    vertexnova/interaction/version.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/export.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/version.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/interaction_types.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_behavior.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_controller.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_manipulator_factory.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/camera_system_controller.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/orbit_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/arcball_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/fps_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/fly_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/ortho_pan_zoom_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/follow_manipulator.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/interaction.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/detail/camera_manipulator_base.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/detail/orbit_style_base.h
    ${VNE_INCLUDE_DIR}/vertexnova/interaction/detail/free_camera_base.h
)

set(PRIVATE_HEADER_FILES
    vertexnova/interaction/camera_input_adapter.h
)

# One library per build: static or shared, always named vneinteraction (alias vne::interaction).
if(VNE_INTERACTION_LIB_TYPE STREQUAL "shared")
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(vneinteraction ${_lib_type} ${SOURCE_FILES} ${HEADER_FILES} ${PRIVATE_HEADER_FILES})

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
# Avoid direct links to avoid duplicate library warnings (ld: ignoring duplicate libraries).
if(TARGET vne::scene)
    target_link_libraries(vneinteraction PUBLIC vne::scene)
else()
    # Fallback when vnescene is not used (e.g. minimal build)
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

# iOS (and visionOS): link Foundation so the linker resolves symbols (vnelogging and deps may use it).
# Explicit -framework form ensures Xcode/iOS SDK finds the framework.
if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "visionOS")
    target_link_libraries(vneinteraction PUBLIC "-framework Foundation")
endif()

# Set export/import defines for shared builds. The header uses them only on Windows
# (VNE_INTERACTION_API becomes __declspec(dllexport)/dllimport); on other platforms they are no-ops.
if(_lib_type STREQUAL "SHARED")
    target_compile_definitions(vneinteraction
        PRIVATE VNE_INTERACTION_BUILDING_DLL
        INTERFACE VNE_INTERACTION_DLL
    )
    # MSVC: C4251 (member type needs dll-interface), C4275 (base needs dll-interface).
    # Expected when exporting classes that hold vne::math types, std::string, std::shared_ptr, or
    # use detail/base classes; suppressing so /WX does not fail the build.
    if(MSVC)
        target_compile_options(vneinteraction PRIVATE /wd4251 /wd4275)
    endif()
endif()

add_library(vne::interaction ALIAS vneinteraction)

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})
source_group("Headers/Private" FILES ${PRIVATE_HEADER_FILES})
