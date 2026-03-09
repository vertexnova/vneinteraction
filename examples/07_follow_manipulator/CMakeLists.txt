#==============================================================================
# 07_follow_manipulator Example: static and dynamic target following with damping
#==============================================================================

add_executable(example_07_follow_manipulator main.cpp)

target_link_libraries(example_07_follow_manipulator PRIVATE vne::interaction)

target_include_directories(example_07_follow_manipulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_07_follow_manipulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
