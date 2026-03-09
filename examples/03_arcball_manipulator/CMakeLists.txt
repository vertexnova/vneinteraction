#==============================================================================
# 03_arcball_manipulator Example: arcball drag, pan, inertia, and fitToAABB
#==============================================================================

add_executable(example_03_arcball_manipulator main.cpp)

target_link_libraries(example_03_arcball_manipulator PRIVATE vne::interaction)

target_include_directories(example_03_arcball_manipulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_03_arcball_manipulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
