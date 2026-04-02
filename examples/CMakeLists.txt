#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vneinteraction)
    return()
endif()

#------------------------------------------------------------------------------
# Helper macro: register one example executable with standard settings.
#------------------------------------------------------------------------------
macro(vne_add_example target_name)
    add_executable(${target_name} main.cpp)
    target_link_libraries(${target_name} PRIVATE vne::interaction)
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
endmacro()

add_subdirectory(01_library_info)
add_subdirectory(02_medical_3d_inspect)
add_subdirectory(03_medical_2d_slices)
add_subdirectory(04_game_editor_camera)
add_subdirectory(05_robotic_simulator)
add_subdirectory(06_custom_input_bindings)
add_subdirectory(07_camera_rig_composition)
add_subdirectory(08_camera_state_save_restore)
