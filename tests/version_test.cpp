/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/version.h"

#include <gtest/gtest.h>

#include <cstring>

namespace vne_interaction_test {

class VersionTest : public testing::Test {};

TEST_F(VersionTest, ReturnsNonNullPointer) {
    const char* ver = vne::interaction::get_version();
    ASSERT_NE(ver, nullptr);
}

TEST_F(VersionTest, ReturnsNonEmptyString) {
    const char* ver = vne::interaction::get_version();
    EXPECT_STRNE(ver, "");
}

TEST_F(VersionTest, ReturnsExpectedVersion) {
    const char* ver = vne::interaction::get_version();
    EXPECT_STREQ(ver, "1.0.0");
}

TEST_F(VersionTest, ContainsMajorVersion) {
    const char* ver = vne::interaction::get_version();
    ASSERT_NE(ver, nullptr);
    EXPECT_NE(std::strstr(ver, "1"), nullptr);
}

TEST_F(VersionTest, IsStableAcrossMultipleCalls) {
    const char* ver_a = vne::interaction::get_version();
    const char* ver_b = vne::interaction::get_version();
    ASSERT_NE(ver_a, nullptr);
    ASSERT_NE(ver_b, nullptr);
    EXPECT_STREQ(ver_a, ver_b);
}

}  // namespace vne_interaction_test
