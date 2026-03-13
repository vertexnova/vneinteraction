/* ---------------------------------------------------------------------
 * Copyright (c) 2026
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vertexnova/logging/logging.h>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vneinteraction.tests")

class LoggingGuard {
   public:
    LoggingGuard() {
        auto cfg = vne::log::Logging::defaultLoggerConfig();
        cfg.log_level = vne::log::LogLevel::eInfo;
        cfg.async = false;
        vne::log::Logging::configureLogger(cfg);
    }

    ~LoggingGuard() { vne::log::Logging::shutdown(); }

    LoggingGuard(const LoggingGuard&) = delete;
    LoggingGuard& operator=(const LoggingGuard&) = delete;
};

}  // namespace

int main(int argc, char** argv) {
    LoggingGuard logging_guard;

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    VNE_LOG_INFO << "Starting vneinteraction test suite";

    return RUN_ALL_TESTS();
}
