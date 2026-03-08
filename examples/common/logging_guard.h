#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Common logging configuration for VneTemplate examples
 * ----------------------------------------------------------------------
 */

#include <vertexnova/logging/logging.h>

// Create a logger category for examples
CREATE_VNE_LOGGER_CATEGORY("vnetemplate.examples")

namespace vne::template_ns::examples {

/**
 * @class LoggingGuard
 * @brief RAII guard for console logging configuration in examples.
 *
 * Initializes the logging system with console output in its constructor
 * and shuts it down in its destructor. Use at the start of main() in example programs.
 *
 * Usage:
 * @code
 * int main() {
 *     LoggingGuard logging_guard;
 *     // ... example code ...
 *     return 0;
 * }
 * @endcode
 */
class LoggingGuard {
   public:
    LoggingGuard() {
        vne::log::LoggerConfig config;
        config.name = vne::log::kDefaultLoggerName;
        config.sink = vne::log::LogSinkType::eConsole;
        config.console_pattern = "[%l] [%n] %v";
        config.log_level = vne::log::LogLevel::eInfo;
        config.async = false;

        vne::log::Logging::configureLogger(config);
    }

    ~LoggingGuard() { vne::log::Logging::shutdown(); }

    LoggingGuard(const LoggingGuard&) = delete;
    LoggingGuard& operator=(const LoggingGuard&) = delete;
};

}  // namespace vne::template_ns::examples
