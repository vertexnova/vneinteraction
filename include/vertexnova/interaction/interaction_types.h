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
 * @file interaction_types.h
 * @brief Single source for shared interaction types: actions, state blobs, input bindings,
 *        and behavioral enums.
 *
 * Consolidates camera actions, state blobs, input bindings, and behavioral enums in one header.
 *
 * @par Contents
 * - @ref CameraActionType, @ref CameraCommandPayload, @ref GestureAction
 * - @ref TrackballCameraState, @ref FreeCameraState, @ref FreeLookInputState, @ref OrbitalInteractionState
 * - @ref InputRule, @ref MouseBinding, @ref KeyBinding, touch structs, modifier constants
 * - Behavioral enums: @ref FreeLookMode, @ref FreeLookRotationMode, @ref ZoomMethod,
 *   @ref OrbitPivotMode, @ref UpAxis, @ref ViewDirection, @ref CenterOfInterestSpace,
 *   @ref TrackballProjectionMode
 *
 * @par Math helpers
 * Orbit/trackball geometry is implemented in internal library sources
 * (\c src/vertexnova/interaction/detail/), not as installed types.
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/events/types.h>
#include <vertexnova/math/core/core.h>

#include <cstdint>

namespace vne::interaction {

// =============================================================================
// Camera action / command layer (intent between input and manipulator)
// =============================================================================

/**
 * @brief High-level gesture actions for remappable bindings.
 *
 * Used with InputMapper::bindGesture, bindScroll, bindDoubleClick to customize
 * controls without exposing InputRule or CameraActionType.
 */
enum class GestureAction : std::uint8_t {
    eRotate = 0,    //!< Orbit / trackball rotate (button + drag)
    ePan = 1,       //!< Pan (button + drag)
    eZoom = 2,      //!< Zoom (scroll wheel)
    eLook = 3,      //!< FPS-style look (button + drag)
    eSetPivot = 4,  //!< Set orbit pivot via double-click (maps to orbit COI; see OrbitalCameraManipulator)
};

/**
 * @brief Semantic camera commands emitted by @ref InputMapper to @ref CameraRig.
 *
 * Manipulators respond to the subset of actions they implement; unhandled actions are ignored.
 */
enum class CameraActionType : std::uint8_t {
    eNone = 0,               //!< Sentinel: no action for this event phase (used in InputRule)
    eBeginRotate = 1,        //!< Begin rotate the camera by like Left mouse button or touch rotate
    eRotateDelta = 2,        //!< Rotate the camera by like Left mouse button or touch rotate
    eEndRotate = 3,          //!< End rotate the camera by like Left mouse button or touch rotate
    eBeginPan = 4,           //!< Begin pan the camera by like Middle mouse button or touch pan
    ePanDelta = 5,           //!< Pan the camera by like Middle mouse button or touch pan
    eEndPan = 6,             //!< End pan the camera by like Middle mouse button or touch pan
    eZoomAtCursor = 7,       //!< Zoom the camera at the cursor by like scroll wheel or touch pinch
    eLookDelta = 8,          //!< Look the camera by like Right mouse button or touch pan
    eBeginLook = 9,          //!< Begin look the camera by like Right mouse button or touch pan
    eEndLook = 10,           //!< End look the camera by like Right mouse button or touch pan
    eMoveForward = 11,       //!< Move forward the camera by like W key or mouse wheel
    eMoveBackward = 12,      //!< Move backward the camera by like S key or mouse wheel
    eMoveLeft = 13,          //!< Move left the camera by like A key or mouse wheel
    eMoveRight = 14,         //!< Move right the camera by like D key or mouse wheel
    eMoveUp = 15,            //!< Move up the camera by like E key or mouse wheel
    eMoveDown = 16,          //!< Move down the camera by like Q key or mouse wheel
    eSprintModifier = 17,    //!< Sprint the movement speed by like Shift key or mouse wheel
    eSlowModifier = 18,      //!< Slow the movement speed by like Ctrl key or mouse wheel
    eOrbitPanModifier = 19,  //!< Orbit: Shift+LMB pan alias; navigation: sprint-like where mapped.
    eResetView = 20,         //!< Reset the view of the camera
    eSetPivotAtCursor =
        21,  //!< Double-click: set COI along view direction (camera + front * orbit distance); payload x/y ignored
    eIncreaseMoveSpeed = 22,         //!< Increase the movement speed by like Shift key or mouse wheel
    eDecreaseMoveSpeed = 23,         //!< Decrease the movement speed by like Ctrl key or mouse wheel
    eIncreaseInteractionSpeed = 24,  //!< Increase the interaction speed by like Shift key or mouse wheel
    eDecreaseInteractionSpeed = 25,  //!< Decrease the interaction speed by like Ctrl key or mouse wheel
};

/** Payload for actions that carry pointer/cursor or delta data. */
struct VNE_INTERACTION_API CameraCommandPayload {
    float x_px = 0.0f;         //!< Absolute X position in screen pixels
    float y_px = 0.0f;         //!< Absolute Y position in screen pixels
    float delta_x_px = 0.0f;   //!< Relative X delta in screen pixels
    float delta_y_px = 0.0f;   //!< Relative Y delta in screen pixels
    float zoom_factor = 1.0f;  //!< Zoom scaling factor
    bool pressed = false;      //!< Button pressed state
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)   // dll-interface for member type from another lib
#pragma warning(disable : 26495)  // uninitialized member (ctor initializes all)
#endif

// =============================================================================
// Grouped input / interaction state
// =============================================================================

/**
 * @brief Per-frame input state for FreeLookManipulator (held keys).
 */
struct VNE_INTERACTION_API FreeLookInputState {
    bool move_forward = false;   //!< Move forward by like W key or mouse wheel
    bool move_backward = false;  //!< Move backward by like S key or mouse wheel
    bool move_left = false;      //!< Move left by like A key or mouse wheel
    bool move_right = false;     //!< Move right by like D key or mouse wheel
    bool move_up = false;        //!< Move up by like E key or mouse wheel
    bool move_down = false;      //!< Move down by like Q key or mouse wheel
    bool sprint = false;         //!< Sprint the movement speed by like Shift key or mouse wheel
    bool slow = false;           //!< Slow the movement speed by like Ctrl key or mouse wheel
    bool looking = false;        //!< Looking with the mouse by like Right mouse button or touch pan
};

/**
 * @brief Drag/pan interaction state for OrbitalCameraManipulator.
 */
struct VNE_INTERACTION_API OrbitalInteractionState {
    bool rotating = false;        //!< Rotating the camera by like Left mouse button or touch rotate
    bool panning = false;         //!< Panning the camera by like Middle mouse button or touch pan
    bool modifier_shift = false;  //!< Modifier shift key by like Shift key or touch modifier shift
    float last_x_px = 0.0f;       //!< Last x position in pixels
    float last_y_px = 0.0f;       //!< Last y position in pixels
};

// =============================================================================
// Camera pose state blobs (manipulators operate on these, then apply to ICamera)
// =============================================================================

/**
 * @brief Serializable state snapshot for @ref OrbitalCameraManipulator — orbit pivot (COI), camera–pivot
 *        distance, virtual-trackball orientation quaternion, and world up.
 */
struct VNE_INTERACTION_API TrackballCameraState {
    vne::math::Vec3f coi_world;  //!< Center of interest in world space
    float distance = 5.0f;       //!< Camera-to-pivot distance
    vne::math::Quatf rotation;   //!< Orientation quaternion
    vne::math::Vec3f world_up;   //!< World up vector

    TrackballCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f, 1.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

/**
 * @brief Serializable state for free-look camera (FreeLookManipulator).
 */
struct VNE_INTERACTION_API FreeCameraState {
    vne::math::Vec3f position;     //!< Camera position in world space
    vne::math::Quatf orientation;  //!< Camera-to-world rotation

    FreeCameraState() noexcept
        : position(0.0f, 0.0f, 0.0f)
        , orientation(0.0f, 0.0f, 0.0f, 1.0f) {}
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// =============================================================================
// Input configuration (InputMapper rules)
// =============================================================================

// InputRule / MouseBinding store mouse buttons as int codes — must match vne::events::MouseButton values.
static_assert(static_cast<int>(vne::events::MouseButton::eLeft) == 0, "MouseButton::eLeft must remain 0 for bindings");
static_assert(static_cast<int>(vne::events::MouseButton::eRight) == 1,
              "MouseButton::eRight must remain 1 for bindings");
static_assert(static_cast<int>(vne::events::MouseButton::eMiddle) == 2,
              "MouseButton::eMiddle must remain 2 for bindings");

//! Mouse button: use vne::events::MouseButton (eLeft, eRight, eMiddle) for compatibility.
using MouseButton = vne::events::MouseButton;

//! Modifier key bitmask constants for InputRule::modifier_mask.
static constexpr int kModNone = 0;        //!< No modifier key held
static constexpr int kModShift = 1 << 0;  //!< Shift key held
static constexpr int kModCtrl = 1 << 1;   //!< Ctrl key held
static constexpr int kModAlt = 1 << 2;    //!< Alt key held

/**
 * @brief Mouse button + modifier binding for gesture remapping.
 *
 * Uses vne::events::ModifierKey for modifier_mask (eModNone, eModShift, eModCtrl, eModAlt).
 */
struct VNE_INTERACTION_API MouseBinding {
    MouseButton button = MouseButton::eLeft;  //!< Mouse button for this binding.
    vne::events::ModifierKey modifier_mask =
        vne::events::ModifierKey::eModNone;  //!< Required modifiers; @c eModNone = none.
};

/**
 * @brief Keyboard key + optional modifier for rebindable actions.
 *
 * Used by controllers and @ref InputMapper::bindKey for data-driven key rules.
 */
struct VNE_INTERACTION_API KeyBinding {
    vne::events::KeyCode key = vne::events::KeyCode::eUnknown;  //!< Key for this binding.
    vne::events::ModifierKey modifier_mask =
        vne::events::ModifierKey::eModNone;  //!< Required modifiers; @c eModNone = none.
};

/** Touch pan gesture data with screen pixel deltas. */
struct VNE_INTERACTION_API TouchPan final {
    float delta_x_px = 0.0f;  //!< Horizontal pan delta in screen pixels
    float delta_y_px = 0.0f;  //!< Vertical pan delta in screen pixels
};

/**
 * @brief Touch pinch (zoom) gesture data with scale and center position.
 *
 * @note @ref InputMapper::onTouchPinch passes @c scale through as @c zoom_factor for @c eZoomAtCursor
 *       (same wheel/dolly convention as scroll).
 */
struct VNE_INTERACTION_API TouchPinch final {
    float scale = 1.0f;        //!< Zoom scale factor (<1 zooms in, >1 zooms out)
    float center_x_px = 0.0f;  //!< Pinch center X position in screen pixels
    float center_y_px = 0.0f;  //!< Pinch center Y position in screen pixels
};

/**
 * @brief A single data-driven input binding rule.
 *
 * Maps one input trigger (button press, key, scroll, touch, double-click) to
 * CameraActionType values that are emitted to the active CameraRig.
 *
 * - For mouse/key triggers: on_press emitted on press, on_release on release,
 *   on_delta emitted each frame that a move delta arrives while the button is held.
 * - For scroll/touch triggers: on_delta emitted per event.
 * - For double-click: on_press emitted once.
 * - Use eNone as a sentinel to mean "no action for this event phase".
 */
struct VNE_INTERACTION_API InputRule {
    /** What kind of input triggers this rule. */
    enum class Trigger : std::uint8_t {
        eMouseButton = 0,    //!< Mouse button; code = MouseButton int (0=left, 1=right, 2=middle)
        eKey = 1,            //!< Keyboard key; code = vne::events::KeyCode:: value (e.g. KeyCode::eW)
        eScroll = 2,         //!< Mouse scroll wheel; code = 0
        eTouchPan = 3,       //!< Touch pan gesture; code = 0
        eTouchPinch = 4,     //!< Touch pinch gesture; code = 0
        eMouseDblClick = 5,  //!< Mouse button double-click; code = MouseButton int
    };

    Trigger trigger = Trigger::eMouseButton;
    int code = 0;  //!< Button index or key code; 0 for scroll/touch
    int modifier_mask =
        kModNone;  //!< Required modifier bitmask (non-negative; only @c kModShift|kModCtrl|kModAlt); 0 = none required

    CameraActionType on_press = CameraActionType::eNone;    //!< Emitted on press / double-click
    CameraActionType on_release = CameraActionType::eNone;  //!< Emitted on release
    CameraActionType on_delta = CameraActionType::eNone;    //!< Emitted per delta/scroll/touch event
};

// =============================================================================
// Behavioral enums
// =============================================================================

/** Space for center-of-interest specification. */
enum class CenterOfInterestSpace : std::uint8_t {
    eWorldSpace = 0,   //!< Pivot point in absolute world coordinates
    eCameraSpace = 1,  //!< Pivot point in camera-relative coordinates
};

/** Preset view direction for camera framing. */
enum class ViewDirection : std::uint8_t {
    eFront = 0,   //!< Looking along +Z axis
    eBack = 1,    //!< Looking along -Z axis
    eLeft = 2,    //!< Looking along -X axis
    eRight = 3,   //!< Looking along +X axis
    eTop = 4,     //!< Looking down along -Y axis
    eBottom = 5,  //!< Looking up along +Y axis
    eIso = 6,     //!< Isometric view at 45-45 degrees
};

/** Zoom behavior method selection (declared in preferred UI / iteration order). */
enum class ZoomMethod : std::uint8_t {
    eSceneScale = 0,  //!< XY scene scale in the view matrix (virtual zoom)
    eChangeFov = 1,   //!< Adjust field of view (perspective) or ortho half-extents
    eDollyToCoi = 2,  //!< Move camera along view direction toward center of interest
};

/** Movement mode for FreeLookManipulator (FPS or Fly). */
enum class FreeLookMode : std::uint8_t {
    eFps = 0,  //!< FPS: world-up fixed, pitch clamped [-89°, 89°] (default)
    eFly = 1,  //!< Fly: unconstrained, up follows camera
};

/** Rotation style for FreeLookManipulator mouse look. */
enum class FreeLookRotationMode : std::uint8_t {
    eYawPitch = 0,   //!< Mouse drag = yaw + pitch deltas (default).
    eTrackball = 1,  //!< Mouse drag = virtual-trackball quaternion (full 3D, no gimbal).
};

/**
 * @brief Screen-to-sphere projection mode for virtual trackball rotation.
 * @see OrbitalCameraManipulator::setTrackballProjectionMode
 */
enum class TrackballProjectionMode : std::uint8_t {
    eHyperbolic = 0,  //!< Spherical cap + hyperbolic continuation (default; isotropic feel)
    eRim = 1,         //!< Hemisphere inside unit disk; equatorial rim beyond it
};

/**
 * @brief Pivot control mode for orbit-style camera behaviors.
 *
 * Determines which point in world space the camera orbits around.
 * Used by OrbitalCameraManipulator and Inspect3DController.
 */
enum class OrbitPivotMode : std::uint8_t {
    eCoi = 0,         //!< Pan moves COI in the view plane; camera looks at COI (default).
    eViewCenter = 1,  //!< Same as eCoi during pan; on pan end, sync COI from camera target.
    eFixed = 2,       //!< Fixed world pivot; pan translates eye+target together; COI unchanged.
};

/** World up axis selection. */
enum class UpAxis : std::uint8_t {
    eY = 0,  //!< Y axis is world up (standard in graphics)
    eZ = 1,  //!< Z axis is world up (common in scientific/CAD)
};

/**
 * @brief Convert an UpAxis enum to a normalized world-space up vector.
 * @param axis The up axis (eY or eZ)
 * @return Normalized vector (0,1,0) for eY or (0,0,1) for eZ
 */
[[nodiscard]] inline vne::math::Vec3f toWorldUp(UpAxis axis) noexcept {
    return (axis == UpAxis::eZ) ? vne::math::Vec3f(0.0f, 0.0f, 1.0f) : vne::math::Vec3f(0.0f, 1.0f, 0.0f);
}

}  // namespace vne::interaction
