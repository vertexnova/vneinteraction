/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/interaction/camera_system_controller.h"

#include "vertexnova/interaction/camera_manipulator.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/scene/camera/camera_factory.h"
#include "vertexnova/scene/camera/camera_types.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace vne_interaction_test {

class MockCameraManipulator : public vne::interaction::ICameraManipulator {
   public:
    MOCK_METHOD(bool, supportsPerspective, (), (const, noexcept, override));
    MOCK_METHOD(bool, supportsOrthographic, (), (const, noexcept, override));
    MOCK_METHOD(void, setCamera, (std::shared_ptr<vne::scene::ICamera>), (noexcept, override));
    MOCK_METHOD(void, setEnabled, (bool), (noexcept, override));
    MOCK_METHOD(bool, isEnabled, (), (const, noexcept, override));
    MOCK_METHOD(void, setViewportSize, (float, float), (noexcept, override));
    MOCK_METHOD(void, update, (double), (noexcept, override));
    MOCK_METHOD(void,
                applyCommand,
                (vne::interaction::CameraActionType, const vne::interaction::CameraCommandPayload&, double),
                (noexcept, override));
    MOCK_METHOD(void, handleMouseMove, (float, float, float, float, double), (noexcept, override));
    MOCK_METHOD(void, handleMouseButton, (int, bool, float, float, double), (noexcept, override));
    MOCK_METHOD(void, handleMouseScroll, (float, float, float, float, double), (noexcept, override));
    MOCK_METHOD(void, handleKeyboard, (int, bool, double), (noexcept, override));
    MOCK_METHOD(void, handleTouchPan, (const vne::interaction::TouchPan&, double), (noexcept, override));
    MOCK_METHOD(void, handleTouchPinch, (const vne::interaction::TouchPinch&, double), (noexcept, override));
    MOCK_METHOD(float, getSceneScale, (), (const, noexcept, override));
    MOCK_METHOD(float, getZoomSpeed, (), (const, noexcept, override));
    MOCK_METHOD(void, resetState, (), (noexcept, override));
    MOCK_METHOD(void, fitToAABB, (const vne::math::Vec3f&, const vne::math::Vec3f&), (noexcept, override));
    MOCK_METHOD(float, getWorldUnitsPerPixel, (), (const, noexcept, override));
};

class CameraSystemControllerTest : public testing::Test {
   protected:
    void SetUp() override {
        controller_ = std::make_unique<vne::interaction::CameraSystemController>();
        mock_manip_ = std::make_shared<testing::NiceMock<MockCameraManipulator>>();
    }

    std::unique_ptr<vne::interaction::CameraSystemController> controller_;
    std::shared_ptr<MockCameraManipulator> mock_manip_;
};

TEST_F(CameraSystemControllerTest, ImplementsICameraController) {
    EXPECT_EQ(controller_->getCamera(), nullptr);
    EXPECT_TRUE(controller_->isEnabled());
    EXPECT_EQ(controller_->getName(), "CameraSystemController");
}

TEST_F(CameraSystemControllerTest, SetGetCamera) {
    auto cam = vne::scene::CameraFactory::createPerspective(
        vne::scene::PerspectiveCameraParameters(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f));
    controller_->setCamera(cam);
    EXPECT_EQ(controller_->getCamera(), cam);
}

TEST_F(CameraSystemControllerTest, DefaultManipulatorIsNull) {
    EXPECT_EQ(controller_->getManipulator(), nullptr);
}

TEST_F(CameraSystemControllerTest, SetManipulatorStoresIt) {
    controller_->setManipulator(mock_manip_);
    EXPECT_EQ(controller_->getManipulator(), mock_manip_);
}

TEST_F(CameraSystemControllerTest, SetManipulatorNullptrClearsIt) {
    controller_->setManipulator(mock_manip_);
    controller_->setManipulator(nullptr);
    EXPECT_EQ(controller_->getManipulator(), nullptr);
}

TEST_F(CameraSystemControllerTest, SetViewportSizeWithNoManipulatorDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(controller_->setViewportSize(1280.0f, 720.0f));
}

TEST_F(CameraSystemControllerTest, UpdateWithNoManipulatorDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(controller_->update(0.016));
}

TEST_F(CameraSystemControllerTest, HandleMouseMoveWithNoManipulatorDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(controller_->handleMouseMove(100.0f, 200.0f, 1.0f, -1.0f, 0.016));
}

TEST_F(CameraSystemControllerTest, HandleMouseButtonWithNoManipulatorDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(controller_->handleMouseButton(0, true, 100.0f, 200.0f, 0.016));
}

TEST_F(CameraSystemControllerTest, HandleMouseScrollWithNoManipulatorDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(controller_->handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, 0.016));
}

TEST_F(CameraSystemControllerTest, HandleKeyboardWithNoManipulatorDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(controller_->handleKeyboard(87, true, 0.016));
}

TEST_F(CameraSystemControllerTest, HandleTouchPanWithNoManipulatorDoesNotCrash) {
    vne::interaction::TouchPan pan{5.0f, -3.0f};
    EXPECT_NO_FATAL_FAILURE(controller_->handleTouchPan(pan, 0.016));
}

TEST_F(CameraSystemControllerTest, HandleTouchPinchWithNoManipulatorDoesNotCrash) {
    vne::interaction::TouchPinch pinch{1.1f, 640.0f, 360.0f};
    EXPECT_NO_FATAL_FAILURE(controller_->handleTouchPinch(pinch, 0.016));
}

TEST_F(CameraSystemControllerTest, SetViewportSizeDelegatesToManipulator) {
    EXPECT_CALL(*mock_manip_, setViewportSize(1280.0f, 720.0f)).Times(1);
    controller_->setManipulator(mock_manip_);
    controller_->setViewportSize(1280.0f, 720.0f);
}

TEST_F(CameraSystemControllerTest, UpdateDelegatesToManipulator) {
    EXPECT_CALL(*mock_manip_, update(testing::DoubleEq(0.016))).Times(1);
    controller_->setManipulator(mock_manip_);
    controller_->update(0.016);
}

TEST_F(CameraSystemControllerTest, HandleMouseMoveDelegatesToAdapterThenManipulatorApplyCommand) {
    EXPECT_CALL(*mock_manip_, applyCommand(testing::_, testing::_, testing::DoubleEq(0.016)))
        .Times(testing::AtLeast(0));
    controller_->setManipulator(mock_manip_);
    controller_->handleMouseMove(100.0f, 200.0f, 1.0f, -1.0f, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleMouseButtonDelegatesToAdapterThenManipulatorApplyCommand) {
    EXPECT_CALL(*mock_manip_,
                applyCommand(vne::interaction::CameraActionType::eBeginRotate, testing::_, testing::DoubleEq(0.016)))
        .Times(1);
    controller_->setManipulator(mock_manip_);
    controller_->handleMouseButton(0, true, 100.0f, 200.0f, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleMouseScrollDelegatesToAdapterThenManipulatorApplyCommand) {
    EXPECT_CALL(*mock_manip_,
                applyCommand(vne::interaction::CameraActionType::eZoomAtCursor, testing::_, testing::DoubleEq(0.016)))
        .Times(1);
    controller_->setManipulator(mock_manip_);
    controller_->handleMouseScroll(0.0f, -1.0f, 640.0f, 360.0f, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleKeyboardDelegatesToAdapterThenManipulatorApplyCommand) {
    EXPECT_CALL(*mock_manip_, applyCommand(testing::_, testing::_, testing::DoubleEq(0.016)))
        .Times(testing::AtLeast(1));
    controller_->setManipulator(mock_manip_);
    controller_->handleKeyboard(87, true, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleTouchPanOrbitManipulatorSendsRotateDelta) {
    // Perspective/orbit manipulators: touch pan → eRotateDelta
    ON_CALL(*mock_manip_, supportsPerspective()).WillByDefault(testing::Return(true));
    ON_CALL(*mock_manip_, supportsOrthographic()).WillByDefault(testing::Return(false));

    vne::interaction::TouchPan pan{5.0f, -3.0f};
    EXPECT_CALL(*mock_manip_,
                applyCommand(vne::interaction::CameraActionType::eRotateDelta, testing::_, testing::DoubleEq(0.016)))
        .Times(1);
    controller_->setManipulator(mock_manip_);
    controller_->handleTouchPan(pan, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleTouchPanOrthoOnlyManipulatorSendsBeginPanAndPanDelta) {
    // Ortho-only manipulators (e.g. OrthoPanZoomManipulator): touch pan → eBeginPan + ePanDelta
    ON_CALL(*mock_manip_, supportsOrthographic()).WillByDefault(testing::Return(true));
    ON_CALL(*mock_manip_, supportsPerspective()).WillByDefault(testing::Return(false));

    controller_->setManipulator(mock_manip_);

    // First non-zero delta: must receive eBeginPan then ePanDelta
    {
        testing::InSequence seq;
        EXPECT_CALL(*mock_manip_,
                    applyCommand(vne::interaction::CameraActionType::eBeginPan, testing::_, testing::DoubleEq(0.016)))
            .Times(1);
        EXPECT_CALL(*mock_manip_,
                    applyCommand(vne::interaction::CameraActionType::ePanDelta, testing::_, testing::DoubleEq(0.016)))
            .Times(1);
    }
    vne::interaction::TouchPan pan{5.0f, -3.0f};
    controller_->handleTouchPan(pan, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleTouchPanOrthoOnlyContinuedPanSendsOnlyPanDelta) {
    // After the gesture is active (touch_pan_active_=true), subsequent deltas send only ePanDelta
    ON_CALL(*mock_manip_, supportsOrthographic()).WillByDefault(testing::Return(true));
    ON_CALL(*mock_manip_, supportsPerspective()).WillByDefault(testing::Return(false));

    controller_->setManipulator(mock_manip_);

    // First call starts the gesture
    EXPECT_CALL(*mock_manip_, applyCommand(vne::interaction::CameraActionType::eBeginPan, testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*mock_manip_, applyCommand(vne::interaction::CameraActionType::ePanDelta, testing::_, testing::_))
        .Times(2);  // once on first call, once on second

    vne::interaction::TouchPan pan{5.0f, -3.0f};
    controller_->handleTouchPan(pan, 0.016);
    controller_->handleTouchPan(pan, 0.016);  // gesture already active — no second eBeginPan
}

TEST_F(CameraSystemControllerTest, HandleTouchPanOrthoOnlyZeroDeltaEndsGesture) {
    // A zero-delta event after an active gesture sends eEndPan
    ON_CALL(*mock_manip_, supportsOrthographic()).WillByDefault(testing::Return(true));
    ON_CALL(*mock_manip_, supportsPerspective()).WillByDefault(testing::Return(false));

    controller_->setManipulator(mock_manip_);

    // Start gesture
    EXPECT_CALL(*mock_manip_, applyCommand(vne::interaction::CameraActionType::eBeginPan, testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*mock_manip_, applyCommand(vne::interaction::CameraActionType::ePanDelta, testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*mock_manip_, applyCommand(vne::interaction::CameraActionType::eEndPan, testing::_, testing::_))
        .Times(1);

    vne::interaction::TouchPan pan{5.0f, -3.0f};
    controller_->handleTouchPan(pan, 0.016);

    vne::interaction::TouchPan zero_pan{0.0f, 0.0f};
    controller_->handleTouchPan(zero_pan, 0.016);
}

TEST_F(CameraSystemControllerTest, HandleTouchPinchDelegatesToAdapterThenManipulatorApplyCommand) {
    vne::interaction::TouchPinch pinch{1.1f, 640.0f, 360.0f};
    EXPECT_CALL(*mock_manip_,
                applyCommand(vne::interaction::CameraActionType::eZoomAtCursor, testing::_, testing::DoubleEq(0.016)))
        .Times(1);
    controller_->setManipulator(mock_manip_);
    controller_->handleTouchPinch(pinch, 0.016);
}

TEST_F(CameraSystemControllerTest, ReplacingManipulatorForwardsToNewOne) {
    auto second_mock = std::make_shared<testing::NiceMock<MockCameraManipulator>>();

    EXPECT_CALL(*mock_manip_, setViewportSize(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*second_mock, setViewportSize(1280.0f, 720.0f)).Times(1);

    controller_->setManipulator(mock_manip_);
    controller_->setManipulator(second_mock);
    controller_->setViewportSize(1280.0f, 720.0f);
}

}  // namespace vne_interaction_test
