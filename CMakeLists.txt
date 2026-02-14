#==============================================================================
# Copyright (c) 2024 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

cmake_minimum_required(VERSION 3.16)
project(vneinteraction VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VNE_DEPS_INTERNAL_DIR ${PROJECT_SOURCE_DIR}/deps/internal)
set(VNE_DEPS_EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/deps/external)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
if(EXISTS "${PROJECT_SOURCE_DIR}/cmake/vnecmake/modules/ProjectSetup.cmake")
    list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/vnecmake/modules")
endif()

include(ProjectSetup)

if(APPLE AND NOT IOS)
    set(VNE_TARGET_PLATFORM "macOS")
elseif(UNIX AND NOT APPLE)
    set(VNE_TARGET_PLATFORM "Linux")
elseif(WIN32)
    set(VNE_TARGET_PLATFORM "Windows")
else()
    set(VNE_TARGET_PLATFORM "Unknown")
endif()

if(NOT TARGET vne::common AND EXISTS "${VNE_DEPS_INTERNAL_DIR}/vnecommon/CMakeLists.txt")
    add_subdirectory(deps/internal/vnecommon ${CMAKE_BINARY_DIR}/deps/internal/vnecommon)
endif()

if(NOT TARGET vne::math)
    if(EXISTS "${VNE_DEPS_EXTERNAL_DIR}/vnemath/CMakeLists.txt")
        add_subdirectory(deps/external/vnemath ${CMAKE_BINARY_DIR}/deps/external/vnemath)
    elseif(EXISTS "${PROJECT_SOURCE_DIR}/../vnemath/CMakeLists.txt")
        add_subdirectory(${PROJECT_SOURCE_DIR}/../vnemath ${CMAKE_BINARY_DIR}/vnemath)
    else()
        find_package(VneMath REQUIRED)
    endif()
endif()

if(NOT TARGET vne::scene)
    if(EXISTS "${VNE_DEPS_EXTERNAL_DIR}/vnescene/CMakeLists.txt")
        add_subdirectory(deps/external/vnescene ${CMAKE_BINARY_DIR}/deps/external/vnescene)
    elseif(EXISTS "${PROJECT_SOURCE_DIR}/../vnescene/CMakeLists.txt")
        add_subdirectory(${PROJECT_SOURCE_DIR}/../vnescene ${CMAKE_BINARY_DIR}/vnescene)
    else()
        find_package(VneScene REQUIRED)
    endif()
endif()

set(VNE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(VNE_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

configure_file(configs/config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

add_library(InteractionWarnings INTERFACE)
include(ProjectWarnings)
set_project_warnings(InteractionWarnings)
add_library(vne::interaction::Warnings ALIAS InteractionWarnings)

add_library(InteractionBuildSettings INTERFACE)
target_compile_features(InteractionBuildSettings INTERFACE cxx_std_20)
add_library(vne::interaction::BuildSettings ALIAS InteractionBuildSettings)

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

add_subdirectory(src)

if(BUILD_TESTS AND EXISTS "${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()
if(BUILD_EXAMPLES AND EXISTS "${PROJECT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
install(TARGETS vneinteraction InteractionWarnings InteractionBuildSettings
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/vertexnova/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_BINARY_DIR}/config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova/interaction)
