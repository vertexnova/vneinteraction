/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * --------------------------------------------------------------------- */

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/interaction_types.h"

#include <gtest/gtest.h>

namespace vne_interaction_test {

TEST(InputMapper, OrbitPresetReturnsNonEmpty) {
    auto rules = vne::interaction::InputMapper::orbitPreset();
    EXPECT_FALSE(rules.empty());
}

TEST(InputMapper, FpsPresetReturnsNonEmpty) {
    auto rules = vne::interaction::InputMapper::fpsPreset();
    EXPECT_FALSE(rules.empty());
}

TEST(InputMapper, GamePresetReturnsNonEmpty) {
    auto rules = vne::interaction::InputMapper::gamePreset();
    EXPECT_FALSE(rules.empty());
}

TEST(InputMapper, CadPresetReturnsNonEmpty) {
    auto rules = vne::interaction::InputMapper::cadPreset();
    EXPECT_FALSE(rules.empty());
}

TEST(InputMapper, OrthoPresetReturnsNonEmpty) {
    auto rules = vne::interaction::InputMapper::orthoPreset();
    EXPECT_FALSE(rules.empty());
}

TEST(InputMapper, SetRulesClearRules) {
    vne::interaction::InputMapper m;
    m.setRules(vne::interaction::InputMapper::orbitPreset());
    EXPECT_FALSE(m.rules().empty());
    m.clearRules();
    EXPECT_TRUE(m.rules().empty());
}

TEST(InputMapper, AddRule) {
    vne::interaction::InputMapper m;
    vne::interaction::InputRule r;
    r.trigger = vne::interaction::InputRule::Trigger::eScroll;
    r.on_delta = vne::interaction::CameraActionType::eZoomAtCursor;
    m.addRule(r);
    EXPECT_EQ(m.rules().size(), 1u);
}

TEST(InputMapper, CallbackFiredOnMatch) {
    vne::interaction::InputMapper m;
    m.setRules(vne::interaction::InputMapper::orbitPreset());
    bool fired = false;
    m.setActionCallback(
        [&fired](vne::interaction::CameraActionType a, const vne::interaction::CameraCommandPayload&, double) {
            if (a != vne::interaction::CameraActionType::eNone)
                fired = true;
        });
    m.onMouseButton(0, true, 640.0f, 360.0f, 0.016);
    EXPECT_TRUE(fired);
}

}  // namespace vne_interaction_test
