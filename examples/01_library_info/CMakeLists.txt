#==============================================================================
# 01_library_info Example: version query and manipulator capability listing
#==============================================================================

add_executable(example_01_library_info main.cpp)

target_link_libraries(example_01_library_info PRIVATE vne::interaction)

target_include_directories(example_01_library_info PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_01_library_info PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
