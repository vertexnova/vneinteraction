#==============================================================================
# 06_ortho_pan_zoom_manipulator Example: orthographic pan, scroll/pinch zoom
#==============================================================================

add_executable(example_06_ortho_pan_zoom_manipulator main.cpp)

target_link_libraries(example_06_ortho_pan_zoom_manipulator PRIVATE vne::interaction)

target_include_directories(example_06_ortho_pan_zoom_manipulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_06_ortho_pan_zoom_manipulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
