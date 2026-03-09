#==============================================================================
# 02_orbit_manipulator Example: orbit camera configuration and input simulation
#==============================================================================

add_executable(example_02_orbit_manipulator main.cpp)

target_link_libraries(example_02_orbit_manipulator PRIVATE vne::interaction)

target_include_directories(example_02_orbit_manipulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_02_orbit_manipulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
