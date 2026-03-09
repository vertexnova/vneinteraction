#==============================================================================
# 04_fps_manipulator Example: FPS camera WASD movement and mouse-look
#==============================================================================

add_executable(example_04_fps_manipulator main.cpp)

target_link_libraries(example_04_fps_manipulator PRIVATE vne::interaction)

target_include_directories(example_04_fps_manipulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_04_fps_manipulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
