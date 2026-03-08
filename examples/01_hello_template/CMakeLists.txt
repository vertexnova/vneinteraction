#==============================================================================
# 01_hello_template Example: minimal vnetemplate usage (hello, get_version)
#==============================================================================

add_executable(example_01_hello_template main.cpp)

target_link_libraries(example_01_hello_template PRIVATE vne::template vne::logging)

target_include_directories(example_01_hello_template PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_01_hello_template PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
