#==============================================================================
# Copyright (c) 2024 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vneinteraction)
    return()
endif()

add_executable(example_controller example_controller.cpp)
target_link_libraries(example_controller PRIVATE vne::interaction vne::scene)
