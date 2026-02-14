#==============================================================================
# Copyright (c) 2024 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vneinteraction)
    return()
endif()

add_executable(vneinteraction_tests test_interaction.cpp)
target_link_libraries(vneinteraction_tests PRIVATE vne::interaction vne::scene)
add_test(NAME vneinteraction_tests COMMAND vneinteraction_tests)
