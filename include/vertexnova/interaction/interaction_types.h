#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file interaction_types.h
 * @brief Shared types: commands (@ref CameraActionType), input rules (@ref InputRule), bindings, and state blobs.
 *
 * @par Command layer
 * @ref CameraActionType and @ref CameraCommandPayload are the **intent** between @ref InputMapper
 * (and controllers) and @ref CameraRig / @ref ICameraManipulator. Manipulators consume the subset
 * of actions they implement and ignore the rest.
 *
 * @par Rules and gestures
 * @ref InputRule describes one trigger → action mapping. High-level remapping uses @ref GestureAction,
 * @ref MouseBinding, and @ref KeyBinding together with @ref InputMapper::bindGesture and related APIs.
 *
 * @par Math helpers
 * Orbit/trackball **geometry** lives in @ref OrbitBehavior and @ref TrackballBehavior; this header
 * holds interaction enums, payloads, and grouped state used across manipulators.
 */

#include "vertexnova/interaction/export.h"

#include <vertexnova/events/types.h>
#include <vertexnova/math/core/core.h>

#include <cstdint>

namespace vne::interaction {

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

/** Rotation algorithm for orbit-style camera (OrbitalCameraManipulator, Inspect3DController). */
enum class OrbitRotationMode : std::uint8_t {
    eOrbit = 0,      //!< Classic orbit (Euler yaw/pitch), pitch clamped [-89°, 89°]
    eTrackball = 1,  //!< Quaternion / virtual-trackball rotation (smooth, no gimbal lock)
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

/** Mouse button: use vne::events::MouseButton (eLeft, eRight, eMiddle) for compatibility. */
using MouseButton = vne::events::MouseButton;

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

/** Touch pinch (zoom) gesture data with scale and center position. */
struct VNE_INTERACTION_API TouchPinch final {
    float scale = 1.0f;        //!< Zoom scale factor (>1 zooms in, <1 zooms out)
    float center_x_px = 0.0f;  //!< Pinch center X position in screen pixels
    float center_y_px = 0.0f;  //!< Pinch center Y position in screen pixels
};

// -----------------------------------------------------------------------------
// Camera action / command layer (intent between input and manipulator)
// -----------------------------------------------------------------------------
enum class CameraActionType : std::uint8_t {
    eNone = 0,  //!< Sentinel: no action for this event phase (used in InputRule)
    eBeginRotate = 1,
    eRotateDelta = 2,
    eEndRotate = 3,
    eBeginPan = 4,
    ePanDelta = 5,
    eEndPan = 6,
    eZoomAtCursor = 7,
    eLookDelta = 8,
    eBeginLook = 9,
    eEndLook = 10,
    eMoveForward = 11,
    eMoveBackward = 12,
    eMoveLeft = 13,
    eMoveRight = 14,
    eMoveUp = 15,
    eMoveDown = 16,
    eSprintModifier = 17,
    eSlowModifier = 18,
    eOrbitPanModifier = 19,  //!< Orbit: Shift+LMB pan alias; navigation: sprint-like where mapped.
    eResetView = 20,
    eSetPivotAtCursor =
        21,  //!< Double-click: set COI along view direction (camera + front * orbit distance); payload x/y ignored
    eIncreaseMoveSpeed = 22,         //!< Discrete: increase FPS move speed (Navigation3DController)
    eDecreaseMoveSpeed = 23,         //!< Discrete: decrease FPS move speed
    eIncreaseInteractionSpeed = 24,  //!< Discrete: scale orbit pan/rotate sensitivity (Inspect3DController)
    eDecreaseInteractionSpeed = 25,  //!< Discrete: scale orbit pan/rotate sensitivity down
};

// -----------------------------------------------------------------------------
// InputRule — data-driven input ->action mapping (replaces hardcoded adapter logic)
// -----------------------------------------------------------------------------

/** Modifier key bitmask constants for InputRule::modifier_mask. */
static constexpr int kModNone = 0;
static constexpr int kModShift = 1 << 0;  //!< Shift key held
static constexpr int kModCtrl = 1 << 1;   //!< Ctrl key held
static constexpr int kModAlt = 1 << 2;    //!< Alt key held

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
        eMouseButton,    //!< Mouse button; code = MouseButton int (0=left, 1=right, 2=middle)
        eKey,            //!< Keyboard key; code = vne::events::KeyCode:: value (e.g. KeyCode::eW)
        eScroll,         //!< Mouse scroll wheel; code = 0
        eTouchPan,       //!< Touch pan gesture; code = 0
        eTouchPinch,     //!< Touch pinch gesture; code = 0
        eMouseDblClick,  //!< Mouse button double-click; code = MouseButton int
    };

    Trigger trigger = Trigger::eMouseButton;
    int code = 0;                  //!< Button index or key code; 0 for scroll/touch
    int modifier_mask = kModNone;  //!< Required modifier bitmask; 0 = no modifier required

    CameraActionType on_press = CameraActionType::eNone;    //!< Emitted on press / double-click
    CameraActionType on_release = CameraActionType::eNone;  //!< Emitted on release
    CameraActionType on_delta = CameraActionType::eNone;    //!< Emitted per delta/scroll/touch event
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

// -----------------------------------------------------------------------------
// Grouped input / interaction state (replaces scattered booleans)
// -----------------------------------------------------------------------------
struct VNE_INTERACTION_API FreeLookInputState {
    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;
    bool sprint = false;
    bool slow = false;
    bool looking = false;
};

struct VNE_INTERACTION_API OrbitInteractionState {
    bool rotating = false;
    bool panning = false;
    bool modifier_shift = false;
    float last_x_px = 0.0f;
    float last_y_px = 0.0f;
};

// -----------------------------------------------------------------------------
// Internal camera state (manipulators operate on these, then apply to ICamera)
// -----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)   // dll-interface for member type from another lib
#pragma warning(disable : 26495)  // uninitialized member (ctor initializes all)
#endif

struct VNE_INTERACTION_API OrbitCameraState {
    vne::math::Vec3f coi_world;
    float distance = 5.0f;
    vne::math::Vec3f world_up;
    float yaw_deg = 0.0f;
    float pitch_deg = 0.0f;

    OrbitCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

struct VNE_INTERACTION_API TrackballCameraState {
    vne::math::Vec3f coi_world;
    float distance = 5.0f;
    vne::math::Quatf rotation;
    vne::math::Vec3f world_up;

    TrackballCameraState() noexcept
        : coi_world(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f, 1.0f)
        , world_up(0.0f, 1.0f, 0.0f) {}
};

struct VNE_INTERACTION_API FreeCameraState {
    vne::math::Vec3f position;
    float yaw_deg = 0.0f;
    float pitch_deg = 0.0f;
    vne::math::Vec3f up_hint;

    FreeCameraState() noexcept
        : position(0.0f, 0.0f, 0.0f)
        , up_hint(0.0f, 1.0f, 0.0f) {}
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace vne::interaction
