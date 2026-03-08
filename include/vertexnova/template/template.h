#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#if defined(VNE_TEMPLATE_BUILDING_DLL)
#define VNE_TEMPLATE_API __declspec(dllexport)
#elif defined(VNE_TEMPLATE_DLL)
#define VNE_TEMPLATE_API __declspec(dllimport)
#else
#define VNE_TEMPLATE_API
#endif
#else
#define VNE_TEMPLATE_API
#endif

namespace vne {
namespace template_ns {

/** @brief Returns the project version string. */
VNE_TEMPLATE_API const char* get_version();

/** @brief Returns a greeting string (minimal API placeholder). */
VNE_TEMPLATE_API const char* hello();

}  // namespace template_ns
}  // namespace vne
