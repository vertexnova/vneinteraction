#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

cmake_minimum_required(VERSION 3.19)

# Single source of truth for version (updated by release-please on release)
set(VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/VERSION")
if(EXISTS "${VERSION_FILE}")
    file(READ "${VERSION_FILE}" VERSION_FROM_FILE)
    string(STRIP "${VERSION_FROM_FILE}" VERSION_FROM_FILE)
    if(VERSION_FROM_FILE)
        set(PROJECT_VERSION_INIT "${VERSION_FROM_FILE}")
    endif()
endif()
if(NOT DEFINED PROJECT_VERSION_INIT)
    set(PROJECT_VERSION_INIT "1.0.0")
endif()

project(vneinteraction VERSION ${PROJECT_VERSION_INIT} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake modules from vnecmake submodule (cmake/vnecmake)
set(VNE_DEPS_INTERNAL_DIR ${PROJECT_SOURCE_DIR}/deps/internal)
set(VNE_DEPS_EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/deps/external)
set(VNECMAKE_MODULES "${PROJECT_SOURCE_DIR}/cmake/vnecmake/modules")
if(NOT EXISTS "${VNECMAKE_MODULES}/VneUseDep.cmake")
    message(FATAL_ERROR "vnecmake not found at cmake/vnecmake. Run: git submodule update --init --recursive")
endif()
list(APPEND CMAKE_MODULE_PATH "${VNECMAKE_MODULES}")

include(ProjectSetup)

#==============================================================================
# Platform Detection
#==============================================================================
if(DEFINED VNE_TARGET_PLATFORM)
    message(STATUS "Using manually specified target platform: ${VNE_TARGET_PLATFORM}")
else()
    if(EMSCRIPTEN)
        set(VNE_TARGET_PLATFORM "Web")
    elseif(WIN32)
        set(VNE_TARGET_PLATFORM "Windows")
    elseif(APPLE)
        if(VISIONOS OR ${CMAKE_SYSTEM_NAME} STREQUAL "visionOS")
            set(VNE_TARGET_PLATFORM "visionOS")
        elseif(IOS OR ${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
            set(VNE_TARGET_PLATFORM "iOS")
        else()
            set(VNE_TARGET_PLATFORM "macOS")
        endif()
    elseif(UNIX)
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(VNE_TARGET_PLATFORM "Linux")
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
            set(VNE_TARGET_PLATFORM "Android")
        else()
            message(FATAL_ERROR "Unsupported UNIX platform: ${CMAKE_SYSTEM_NAME}")
        endif()
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()
message(STATUS "Detected platform: ${VNE_TARGET_PLATFORM}")

#==============================================================================
# Platform-specific compile definitions
#==============================================================================
if(VNE_TARGET_PLATFORM STREQUAL "macOS")
    add_compile_definitions(VNE_PLATFORM_MACOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "iOS")
    add_compile_definitions(VNE_PLATFORM_IOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "visionOS")
    add_compile_definitions(VNE_PLATFORM_VISIONOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "Windows")
    add_compile_definitions(VNE_PLATFORM_WIN)
elseif(VNE_TARGET_PLATFORM STREQUAL "Linux")
    add_compile_definitions(VNE_PLATFORM_LINUX)
elseif(VNE_TARGET_PLATFORM STREQUAL "Android")
    add_compile_definitions(VNE_PLATFORM_ANDROID)
elseif(VNE_TARGET_PLATFORM STREQUAL "Web")
    add_compile_definitions(VNE_PLATFORM_WEB)
endif()

#==============================================================================
# Internal Libraries (deps/internal) - use if already in build, else add
#==============================================================================
include(VneUseDep)

function(_vneinteraction_configure_vnecommon_dep)
    get_property(_vt_build_examples_ty CACHE BUILD_EXAMPLES PROPERTY TYPE)
    if(NOT _vt_build_examples_ty STREQUAL "NOTFOUND" AND _vt_build_examples_ty)
        set(_vt_build_examples_saved "${BUILD_EXAMPLES}")
    endif()
    vne_use_dep(TARGET vne::common SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vnecommon" BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnecommon"
        CACHE_VARS BUILD_EXAMPLES OFF)
    if(DEFINED _vt_build_examples_saved)
        set(BUILD_EXAMPLES "${_vt_build_examples_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_EXAMPLES CACHE)
    endif()
endfunction()
_vneinteraction_configure_vnecommon_dep()

function(_vneinteraction_configure_vnelogging_dep)
    get_property(_vt_build_tests_ty CACHE BUILD_TESTS PROPERTY TYPE)
    if(NOT _vt_build_tests_ty STREQUAL "NOTFOUND" AND _vt_build_tests_ty)
        set(_vt_build_tests_saved "${BUILD_TESTS}")
    endif()
    get_property(_vt_build_examples_ty CACHE BUILD_EXAMPLES PROPERTY TYPE)
    if(NOT _vt_build_examples_ty STREQUAL "NOTFOUND" AND _vt_build_examples_ty)
        set(_vt_build_examples_saved "${BUILD_EXAMPLES}")
    endif()
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    vne_use_dep(TARGET vne::logging SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vnelogging" BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnelogging"
        CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF)
    if(DEFINED _vt_build_tests_saved)
        set(BUILD_TESTS "${_vt_build_tests_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_TESTS CACHE)
    endif()
    if(DEFINED _vt_build_examples_saved)
        set(BUILD_EXAMPLES "${_vt_build_examples_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_EXAMPLES CACHE)
    endif()
endfunction()
_vneinteraction_configure_vnelogging_dep()

function(_vneinteraction_configure_vnemath_dep)
    get_property(_vt_build_tests_ty CACHE BUILD_TESTS PROPERTY TYPE)
    if(NOT _vt_build_tests_ty STREQUAL "NOTFOUND" AND _vt_build_tests_ty)
        set(_vt_build_tests_saved "${BUILD_TESTS}")
    endif()
    get_property(_vt_build_examples_ty CACHE BUILD_EXAMPLES PROPERTY TYPE)
    if(NOT _vt_build_examples_ty STREQUAL "NOTFOUND" AND _vt_build_examples_ty)
        set(_vt_build_examples_saved "${BUILD_EXAMPLES}")
    endif()
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(VNE_MATH_TESTS OFF CACHE BOOL "" FORCE)
    vne_use_dep(TARGET vne::math SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vnemath" BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnemath"
        CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF VNE_MATH_TESTS OFF)
    if(DEFINED _vt_build_tests_saved)
        set(BUILD_TESTS "${_vt_build_tests_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_TESTS CACHE)
    endif()
    if(DEFINED _vt_build_examples_saved)
        set(BUILD_EXAMPLES "${_vt_build_examples_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_EXAMPLES CACHE)
    endif()
endfunction()
_vneinteraction_configure_vnemath_dep()

function(_vneinteraction_configure_vnescene_dep)
    get_property(_vt_build_tests_ty CACHE BUILD_TESTS PROPERTY TYPE)
    if(NOT _vt_build_tests_ty STREQUAL "NOTFOUND" AND _vt_build_tests_ty)
        set(_vt_build_tests_saved "${BUILD_TESTS}")
    endif()
    get_property(_vt_build_examples_ty CACHE BUILD_EXAMPLES PROPERTY TYPE)
    if(NOT _vt_build_examples_ty STREQUAL "NOTFOUND" AND _vt_build_examples_ty)
        set(_vt_build_examples_saved "${BUILD_EXAMPLES}")
    endif()
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(VNE_SCENE_TESTS OFF CACHE BOOL "" FORCE)
    set(VNE_SCENE_EXAMPLES OFF CACHE BOOL "" FORCE)
    vne_use_dep(TARGET vne::scene SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vnescene" BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnescene"
        CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF VNE_SCENE_TESTS OFF VNE_SCENE_EXAMPLES OFF)
    if(DEFINED _vt_build_tests_saved)
        set(BUILD_TESTS "${_vt_build_tests_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_TESTS CACHE)
    endif()
    if(DEFINED _vt_build_examples_saved)
        set(BUILD_EXAMPLES "${_vt_build_examples_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_EXAMPLES CACHE)
    endif()
    unset(VNE_SCENE_TESTS CACHE)
    unset(VNE_SCENE_EXAMPLES CACHE)
endfunction()
_vneinteraction_configure_vnescene_dep()

function(_vneinteraction_configure_vneevents_dep)
    get_property(_vt_build_tests_ty CACHE BUILD_TESTS PROPERTY TYPE)
    if(NOT _vt_build_tests_ty STREQUAL "NOTFOUND" AND _vt_build_tests_ty)
        set(_vt_build_tests_saved "${BUILD_TESTS}")
    endif()
    get_property(_vt_build_examples_ty CACHE BUILD_EXAMPLES PROPERTY TYPE)
    if(NOT _vt_build_examples_ty STREQUAL "NOTFOUND" AND _vt_build_examples_ty)
        set(_vt_build_examples_saved "${BUILD_EXAMPLES}")
    endif()
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    vne_use_dep(TARGET vne::events SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vneevents" BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vneevents"
        CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF)
    if(DEFINED _vt_build_tests_saved)
        set(BUILD_TESTS "${_vt_build_tests_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_TESTS CACHE)
    endif()
    if(DEFINED _vt_build_examples_saved)
        set(BUILD_EXAMPLES "${_vt_build_examples_saved}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_EXAMPLES CACHE)
    endif()
endfunction()
_vneinteraction_configure_vneevents_dep()

# iOS/Xcode: ensure vnescene (and any dep) target has signing set so build does not fail with
# "Signing for vnescene requires a development team". Prefer DEVELOPMENT_TEAM when provided;
# otherwise disable code signing for the library target.
if(CMAKE_GENERATOR STREQUAL "Xcode" AND (IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS"))
    if(TARGET vnescene)
        if(DEFINED CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM AND CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM)
            set_target_properties(vnescene PROPERTIES
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}")
        else()
            set_target_properties(vnescene PROPERTIES
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
                XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
                XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
        endif()
    endif()
endif()

# Include dirs and config
set(VNE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(VNE_SRC_DIR ${PROJECT_SOURCE_DIR}/src)
configure_file(${PROJECT_SOURCE_DIR}/configs/config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

#==============================================================================
# Build Options
#==============================================================================
# Option naming and deps:
# - Use VNE_INTERACTION_* for vneinteraction-only options so there is no conflict when
#   this project is used as a submodule (parent can set VNE_INTERACTION_TESTS=OFF
#   without affecting BUILD_TESTS used by other deps).
# - BUILD_TESTS / BUILD_EXAMPLES are global; we force them (via VneUseDep
#   CACHE_VARS) before add_subdirectory for each dep so deps do not build their
#   own tests/examples when used as internal libs. We restore after each dep.
# - ENABLE_COVERAGE, ENABLE_DOXYGEN, WARNINGS_AS_ERRORS are global: if multiple
#   libs in deps/internal define the same option name, the first to run wins the
#   default; -D from the command line applies to all. No duplicate option()
#   definition (CMake uses cache), but behavior is "one value for the whole tree."
#==============================================================================
if(CMAKE_PROJECT_NAME STREQUAL "vneinteraction")
    set(VNE_INTERACTION_DEV_DEFAULT ON)
else()
    set(VNE_INTERACTION_DEV_DEFAULT OFF)
endif()
option(VNE_INTERACTION_DEV "Dev build: enable tests (local development)" ${VNE_INTERACTION_DEV_DEFAULT})
option(VNE_INTERACTION_CI "CI build: enable tests (e.g. -DVNE_INTERACTION_CI=ON in pipelines)" OFF)

if(VNE_INTERACTION_CI)
    set(VNE_INTERACTION_DEV OFF CACHE BOOL "" FORCE)
endif()

option(VNE_INTERACTION_TESTS "Build vneinteraction test suite (turn OFF when used as submodule)" ON)
option(VNE_INTERACTION_EXAMPLES "Build vneinteraction example programs (turn OFF when used as submodule)" OFF)
set(VNE_INTERACTION_LIB_TYPE "shared" CACHE STRING "Library type for vneinteraction: static or shared (one per build)")
set_property(CACHE VNE_INTERACTION_LIB_TYPE PROPERTY STRINGS "static" "shared")
if(NOT VNE_INTERACTION_LIB_TYPE STREQUAL "static" AND NOT VNE_INTERACTION_LIB_TYPE STREQUAL "shared")
    message(FATAL_ERROR
        "VNE_INTERACTION_LIB_TYPE must be 'static' or 'shared'. Got: ${VNE_INTERACTION_LIB_TYPE}")
endif()
option(ENABLE_DOXYGEN "Enable Doxygen documentation builds" OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer (Clang/GCC, Linux/macOS only)" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

#==============================================================================
# Incompatible options: coverage and ASan/UBSan must not both be enabled
# (--coverage + -fsanitize=... together produce noisy/incorrect results).
#==============================================================================
if(ENABLE_COVERAGE AND ENABLE_ASAN)
    message(FATAL_ERROR
        "ENABLE_COVERAGE and ENABLE_ASAN cannot both be ON. "
        "Choose one: coverage (gcov/lcov) or ASan/UBSan. "
        "Reconfigure with -DENABLE_COVERAGE=OFF or -DENABLE_ASAN=OFF.")
endif()

#==============================================================================
# Coverage Support (gcov/lcov for CI; .gcda files generated when tests run)
#==============================================================================
if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled (gcov/lcov); compile and link flags will add --coverage")
    include(FindCoverage OPTIONAL)
    # Ensure coverage flags are applied (GCC/Clang) so .gcno/.gcda are emitted
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

#==============================================================================
# AddressSanitizer + UndefinedBehaviorSanitizer
# Linux/macOS only; incompatible with coverage (--coverage + ASan = noise).
#==============================================================================
if(ENABLE_ASAN)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(WARNING "ENABLE_ASAN is only supported with GCC/Clang on Linux/macOS — ignoring.")
    endif()
endif()

if(VNE_INTERACTION_CI)
    # CI enables tests by default, but allow explicit -DVNE_INTERACTION_TESTS=OFF to override
    # (e.g. iOS/Android cross-compile builds: Xcode requires bundle ID for test targets).
    if(NOT DEFINED VNE_INTERACTION_TESTS OR VNE_INTERACTION_TESTS)
        set(VNE_INTERACTION_TESTS ON CACHE BOOL "" FORCE)
    endif()
    set(VNE_INTERACTION_EXAMPLES OFF CACHE BOOL "" FORCE)
    message(STATUS "VneInteraction: CI build -> VNE_INTERACTION_TESTS=${VNE_INTERACTION_TESTS}, VNE_INTERACTION_EXAMPLES=OFF")
elseif(VNE_INTERACTION_DEV)
    set(VNE_INTERACTION_TESTS ON CACHE BOOL "" FORCE)
    set(VNE_INTERACTION_EXAMPLES ON CACHE BOOL "" FORCE)
    message(STATUS "VneInteraction: DEV build -> VNE_INTERACTION_TESTS=ON, VNE_INTERACTION_EXAMPLES=ON")
endif()

#==============================================================================
# Compiler Warnings
#==============================================================================
add_library(InteractionWarnings INTERFACE)
include(ProjectWarnings)
set_project_warnings(InteractionWarnings)
add_library(vne::interaction::Warnings ALIAS InteractionWarnings)

#==============================================================================
# Build Settings Interface Library
#==============================================================================
add_library(InteractionBuildSettings INTERFACE)
target_compile_features(InteractionBuildSettings INTERFACE cxx_std_20)
add_library(vne::interaction::BuildSettings ALIAS InteractionBuildSettings)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_BUILD_TYPE_DEBUG VNE_DEVELOPER_BUILD)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_BUILD_TYPE_RELEASE)
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_BUILD_TYPE_RELWITHDEBINFO)
elseif(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_BUILD_TYPE_MINSIZEREL)
endif()

if(VNE_TARGET_PLATFORM STREQUAL "Windows")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_WIN)
elseif(VNE_TARGET_PLATFORM STREQUAL "Linux")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_LINUX)
elseif(VNE_TARGET_PLATFORM STREQUAL "macOS")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_MACOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "iOS")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_IOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "visionOS")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_VISIONOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "Android")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_ANDROID)
elseif(VNE_TARGET_PLATFORM STREQUAL "Web")
    target_compile_definitions(InteractionBuildSettings INTERFACE VNE_PLATFORM_WEB)
endif()

#==============================================================================
# Add Library
#==============================================================================
add_subdirectory(src)

#==============================================================================
# Tests
#==============================================================================
if(VNE_INTERACTION_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#==============================================================================
# Examples
#==============================================================================
if(VNE_INTERACTION_EXAMPLES)
    add_subdirectory(examples)
endif()

#==============================================================================
# Documentation
#==============================================================================
if(ENABLE_DOXYGEN)
    find_package(Doxygen QUIET OPTIONAL_COMPONENTS dot)
    if(Doxygen_FOUND)
        set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/docs/doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/doxyfile)
        set(DOXYGEN_HAVE_DOT ${DOXYGEN_DOT_FOUND})
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(vneinteraction_doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        message(STATUS "Doxygen target: vneinteraction_doc_doxygen")
    else()
        message(STATUS "Doxygen not found. Install doxygen to build documentation.")
    endif()
endif()

#==============================================================================
# Installation
# Use COMPONENT vneinteraction so that `cmake --install . --component vneinteraction`
# installs only this library — not the internal deps (vnecommon, vnelogging)
# which register their own install() rules via add_subdirectory.
#==============================================================================
include(GNUInstallDirs)
install(TARGETS vneinteraction InteractionWarnings InteractionBuildSettings
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT vneinteraction
)
install(DIRECTORY include/vertexnova/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova
    COMPONENT vneinteraction
    FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_BINARY_DIR}/config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova/interaction
    COMPONENT vneinteraction)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/vneinteraction
        COMPONENT vneinteraction)
endif()
