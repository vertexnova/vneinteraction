#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file export.h
 * @brief `VNE_INTERACTION_API` — DLL export/import macro for shared-library builds.
 *
 * On Windows, define @c VNE_INTERACTION_BUILDING_DLL when compiling the library and
 * @c VNE_INTERACTION_DLL when linking the DLL from an app. Other platforms expand to empty.
 */

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#if defined(VNE_INTERACTION_BUILDING_DLL)
#define VNE_INTERACTION_API __declspec(dllexport)
#elif defined(VNE_INTERACTION_DLL)
#define VNE_INTERACTION_API __declspec(dllimport)
#else
#define VNE_INTERACTION_API
#endif
#else
#define VNE_INTERACTION_API
#endif
