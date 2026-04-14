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
 * InputMapper tests: rule matching, callback firing, scroll zoom, and modifier precedence.
 */

#include "vertexnova/interaction/input_mapper.h"
#include "vertexnova/interaction/interaction_types.h"

#include <vertexnova/events/types.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace vne_interaction_test {

namespace {
// Mirrors InputMapper::onMouseScroll in input_mapper.cpp (anonymous constants). Update both if the curve changes.
constexpr float kWheelZoomFactorPerLine = 0.9f;
constexpr float kWheelScrollYAbsMax = 25.0f;
constexpr float kWheelZoomFactorMin = 0.5f;
constexpr float kWheelZoomFactorMax = 2.0f;

[[nodiscard]] float referenceScrollToZoomFactor(float scroll_y) noexcept {
    const float clamped_dy = std::clamp(scroll_y, -kWheelScrollYAbsMax, kWheelScrollYAbsMax);
    float factor = std::pow(kWheelZoomFactorPerLine, clamped_dy);
    return std::clamp(factor, kWheelZoomFactorMin, kWheelZoomFactorMax);
}

vne::interaction::InputMapper makeMapperWithScrollZoom() {
    vne::interaction::InputMapper m;
    vne::interaction::InputRule r;
    r.trigger = vne::interaction::InputRule::Trigger::eScroll;
    r.on_delta = vne::interaction::CameraActionType::eZoomAtCursor;
    m.addRule(r);
    return m;
}
}  // namespace

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

TEST(InputMapper, OnMouseScroll_ZeroScrollDoesNotEmit) {
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    int call_count = 0;
    m.setActionCallback([&call_count](vne::interaction::CameraActionType,
                                      const vne::interaction::CameraCommandPayload&,
                                      double) { ++call_count; });
    m.onMouseScroll(0.0f, 0.0f, 100.0f, 200.0f, 0.016);
    EXPECT_EQ(call_count, 0);
}

TEST(InputMapper, OnMouseScroll_LegacyNotchOneLineDown) {
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    std::optional<float> zoom;
    vne::interaction::CameraActionType seen = vne::interaction::CameraActionType::eNone;
    m.setActionCallback(
        [&zoom, &seen](vne::interaction::CameraActionType a, const vne::interaction::CameraCommandPayload& p, double) {
            seen = a;
            zoom = p.zoom_factor;
        });
    m.onMouseScroll(0.0f, 1.0f, 10.0f, 20.0f, 0.016);
    ASSERT_TRUE(zoom.has_value());
    EXPECT_EQ(seen, vne::interaction::CameraActionType::eZoomAtCursor);
    EXPECT_FLOAT_EQ(*zoom, referenceScrollToZoomFactor(1.0f));
    EXPECT_FLOAT_EQ(*zoom, kWheelZoomFactorPerLine);
}

TEST(InputMapper, OnMouseScroll_LegacyNotchOneLineUp) {
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    std::optional<float> zoom;
    m.setActionCallback([&zoom](vne::interaction::CameraActionType,
                                const vne::interaction::CameraCommandPayload& p,
                                double) { zoom = p.zoom_factor; });
    m.onMouseScroll(0.0f, -1.0f, 0.0f, 0.0f, 0.016);
    ASSERT_TRUE(zoom.has_value());
    EXPECT_FLOAT_EQ(*zoom, referenceScrollToZoomFactor(-1.0f));
    EXPECT_NEAR(static_cast<double>(*zoom), 1.0 / static_cast<double>(kWheelZoomFactorPerLine), 1e-5);
}

TEST(InputMapper, OnMouseScroll_SmallTrackpadDelta) {
    constexpr float kDy = 0.02f;
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    std::optional<float> zoom;
    m.setActionCallback([&zoom](vne::interaction::CameraActionType,
                                const vne::interaction::CameraCommandPayload& p,
                                double) { zoom = p.zoom_factor; });
    m.onMouseScroll(0.0f, kDy, 640.0f, 360.0f, 0.016);
    ASSERT_TRUE(zoom.has_value());
    EXPECT_FLOAT_EQ(*zoom, referenceScrollToZoomFactor(kDy));
    EXPECT_GT(*zoom, 0.99f);
    EXPECT_LT(*zoom, 1.0f);
}

TEST(InputMapper, OnMouseScroll_LargePositiveScrollHitsFactorClampMin) {
    // |dy| clamped to 25; 0.9^25 < 0.5 → zoom_factor floors at kWheelZoomFactorMin
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    std::optional<float> zoom;
    m.setActionCallback([&zoom](vne::interaction::CameraActionType,
                                const vne::interaction::CameraCommandPayload& p,
                                double) { zoom = p.zoom_factor; });
    m.onMouseScroll(0.0f, 100.0f, 0.0f, 0.0f, 0.016);
    ASSERT_TRUE(zoom.has_value());
    EXPECT_FLOAT_EQ(*zoom, kWheelZoomFactorMin);
    EXPECT_FLOAT_EQ(referenceScrollToZoomFactor(100.0f), kWheelZoomFactorMin);
}

TEST(InputMapper, OnMouseScroll_LargeNegativeScrollHitsFactorClampMax) {
    // dy clamped to -25; 0.9^(-25) >> kWheelZoomFactorMax → caps at 2
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    std::optional<float> zoom;
    m.setActionCallback([&zoom](vne::interaction::CameraActionType,
                                const vne::interaction::CameraCommandPayload& p,
                                double) { zoom = p.zoom_factor; });
    m.onMouseScroll(0.0f, -100.0f, 0.0f, 0.0f, 0.016);
    ASSERT_TRUE(zoom.has_value());
    EXPECT_FLOAT_EQ(*zoom, kWheelZoomFactorMax);
    EXPECT_FLOAT_EQ(referenceScrollToZoomFactor(-100.0f), kWheelZoomFactorMax);
}

TEST(InputMapper, OnMouseScroll_PassesMousePositionInPayload) {
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    vne::interaction::CameraCommandPayload captured{};
    m.setActionCallback([&captured](vne::interaction::CameraActionType,
                                    const vne::interaction::CameraCommandPayload& p,
                                    double) { captured = p; });
    m.onMouseScroll(0.0f, 1.0f, 123.0f, 456.0f, 0.016);
    EXPECT_FLOAT_EQ(captured.x_px, 123.0f);
    EXPECT_FLOAT_EQ(captured.y_px, 456.0f);
}

TEST(InputMapper, OrbitPreset_ShiftLeftClickSelectsPanOverRotate) {
    vne::interaction::InputMapper m;
    m.setRules(vne::interaction::InputMapper::orbitPreset());
    vne::interaction::CameraActionType last = vne::interaction::CameraActionType::eNone;
    m.setActionCallback([&last](vne::interaction::CameraActionType a,
                                const vne::interaction::CameraCommandPayload&,
                                double) { last = a; });

    const int k_left = static_cast<int>(vne::events::MouseButton::eLeft);
    m.onKey(static_cast<int>(vne::events::KeyCode::eLeftShift), true, 0.0);
    m.onMouseButton(k_left, true, 0.0f, 0.0f, 0.0);
    EXPECT_EQ(last, vne::interaction::CameraActionType::eBeginPan);
}

TEST(InputMapper, CadPreset_ShiftMiddleClickSelectsRotateOverPan) {
    vne::interaction::InputMapper m;
    m.setRules(vne::interaction::InputMapper::cadPreset());
    vne::interaction::CameraActionType last = vne::interaction::CameraActionType::eNone;
    m.setActionCallback([&last](vne::interaction::CameraActionType a,
                                const vne::interaction::CameraCommandPayload&,
                                double) { last = a; });

    const int k_middle = static_cast<int>(vne::events::MouseButton::eMiddle);
    m.onKey(static_cast<int>(vne::events::KeyCode::eLeftShift), true, 0.0);
    m.onMouseButton(k_middle, true, 0.0f, 0.0f, 0.0);
    EXPECT_EQ(last, vne::interaction::CameraActionType::eBeginRotate);
}

TEST(InputMapper, EqualModifierSpecificityUsesEarlierRule) {
    vne::interaction::InputMapper m;
    const int k_left = static_cast<int>(vne::events::MouseButton::eLeft);

    vne::interaction::InputRule first{};
    first.trigger = vne::interaction::InputRule::Trigger::eMouseButton;
    first.code = k_left;
    first.modifier_mask = vne::interaction::kModShift;
    first.on_press = vne::interaction::CameraActionType::eBeginPan;
    first.on_release = vne::interaction::CameraActionType::eEndPan;
    first.on_delta = vne::interaction::CameraActionType::ePanDelta;

    vne::interaction::InputRule second = first;
    second.on_press = vne::interaction::CameraActionType::eBeginRotate;
    second.on_release = vne::interaction::CameraActionType::eEndRotate;
    second.on_delta = vne::interaction::CameraActionType::eRotateDelta;

    m.setRules(std::vector<vne::interaction::InputRule>{first, second});

    vne::interaction::CameraActionType last = vne::interaction::CameraActionType::eNone;
    m.setActionCallback([&last](vne::interaction::CameraActionType a,
                                const vne::interaction::CameraCommandPayload&,
                                double) { last = a; });

    m.onKey(static_cast<int>(vne::events::KeyCode::eLeftShift), true, 0.0);
    m.onMouseButton(k_left, true, 0.0f, 0.0f, 0.0);
    EXPECT_EQ(last, vne::interaction::CameraActionType::eBeginPan);
}

TEST(InputMapper, KeyReleaseUsesRuleFromPressWhenModifiersChange) {
    vne::interaction::InputMapper m;
    const int k_w = static_cast<int>(vne::events::KeyCode::eW);

    vne::interaction::InputRule w_shift{};
    w_shift.trigger = vne::interaction::InputRule::Trigger::eKey;
    w_shift.code = k_w;
    w_shift.modifier_mask = vne::interaction::kModShift;
    w_shift.on_press = vne::interaction::CameraActionType::eIncreaseMoveSpeed;
    w_shift.on_release = vne::interaction::CameraActionType::eDecreaseMoveSpeed;

    vne::interaction::InputRule w_plain{};
    w_plain.trigger = vne::interaction::InputRule::Trigger::eKey;
    w_plain.code = k_w;
    w_plain.modifier_mask = vne::interaction::kModNone;
    w_plain.on_press = vne::interaction::CameraActionType::eMoveForward;
    w_plain.on_release = vne::interaction::CameraActionType::eMoveBackward;

    // Shift+W rule is listed first; re-matching on release while Shift is held would hit it first.
    m.setRules(std::vector<vne::interaction::InputRule>{w_shift, w_plain});

    vne::interaction::CameraActionType last = vne::interaction::CameraActionType::eNone;
    m.setActionCallback([&last](vne::interaction::CameraActionType a,
                                const vne::interaction::CameraCommandPayload&,
                                double) { last = a; });

    m.onKey(k_w, true, 0.016);
    EXPECT_EQ(last, vne::interaction::CameraActionType::eMoveForward);

    m.onKey(static_cast<int>(vne::events::KeyCode::eLeftShift), true, 0.016);

    m.onKey(k_w, false, 0.016);
    EXPECT_EQ(last, vne::interaction::CameraActionType::eMoveBackward)
        << "release must use the rule that matched press, not the first Shift+W rule";
}

TEST(InputMapper, OnMouseScroll_UnclampedMidRangeMatchesPow) {
    // Two-line zoom in: no clamp on factor; documents pow curve between notches
    vne::interaction::InputMapper m = makeMapperWithScrollZoom();
    std::optional<float> zoom;
    m.setActionCallback([&zoom](vne::interaction::CameraActionType,
                                const vne::interaction::CameraCommandPayload& p,
                                double) { zoom = p.zoom_factor; });
    m.onMouseScroll(0.0f, 2.0f, 0.0f, 0.0f, 0.016);
    ASSERT_TRUE(zoom.has_value());
    const float expected = std::pow(kWheelZoomFactorPerLine, 2.0f);
    EXPECT_FLOAT_EQ(*zoom, expected);
    EXPECT_FLOAT_EQ(*zoom, referenceScrollToZoomFactor(2.0f));
}

}  // namespace vne_interaction_test
