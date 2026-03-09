#==============================================================================
# 05_fly_manipulator Example: free-flight 6-DOF camera with WASD+QE+mouse-look
#==============================================================================

add_executable(example_05_fly_manipulator main.cpp)

target_link_libraries(example_05_fly_manipulator PRIVATE vne::interaction)

target_include_directories(example_05_fly_manipulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_05_fly_manipulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
