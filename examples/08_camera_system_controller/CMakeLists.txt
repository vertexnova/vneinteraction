#==============================================================================
# 08_camera_system_controller Example: facade with runtime manipulator hot-swap
#==============================================================================

add_executable(example_08_camera_system_controller main.cpp)

target_link_libraries(example_08_camera_system_controller PRIVATE vne::interaction)

target_include_directories(example_08_camera_system_controller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_08_camera_system_controller PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
