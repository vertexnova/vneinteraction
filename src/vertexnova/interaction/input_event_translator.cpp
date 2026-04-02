/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   April 2026
 * ----------------------------------------------------------------------
 */

#include "input_event_translator.h"

#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/touch_event.h"
#include "vertexnova/interaction/interaction_types.h"

namespace vne::interaction {

void dispatchMouseEvents(InputMapper& mapper,
                         CursorState& cursor,
                         const vne::events::Event& event,
                         double dt) noexcept {
    switch (event.type()) {
        case events::EventType::eMouseMoved: {
            const auto& e = static_cast<const events::MouseMovedEvent&>(event);
            const float x = static_cast<float>(e.x());
            const float y = static_cast<float>(e.y());
            const float dx = cursor.first ? 0.0f : static_cast<float>(e.x() - cursor.last_x);
            const float dy = cursor.first ? 0.0f : static_cast<float>(e.y() - cursor.last_y);
            cursor.last_x = e.x();
            cursor.last_y = e.y();
            cursor.first = false;
            mapper.onMouseMove(x, y, dx, dy, dt);
            break;
        }
        case events::EventType::eMouseButtonPressed: {
            const auto& e = static_cast<const events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                cursor.last_x = e.x();
                cursor.last_y = e.y();
            }
            cursor.first = false;
            mapper.onMouseButton(static_cast<int>(e.button()),
                                 true,
                                 static_cast<float>(cursor.last_x),
                                 static_cast<float>(cursor.last_y),
                                 dt);
            break;
        }
        case events::EventType::eMouseButtonReleased: {
            const auto& e = static_cast<const events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                cursor.last_x = e.x();
                cursor.last_y = e.y();
            }
            mapper.onMouseButton(static_cast<int>(e.button()),
                                 false,
                                 static_cast<float>(cursor.last_x),
                                 static_cast<float>(cursor.last_y),
                                 dt);
            break;
        }
        case events::EventType::eMouseButtonDoubleClicked: {
            const auto& e = static_cast<const events::MouseButtonEvent&>(event);
            if (e.hasPosition()) {
                cursor.last_x = e.x();
                cursor.last_y = e.y();
            }
            cursor.first = false;
            mapper.onMouseDoubleClick(static_cast<int>(e.button()),
                                      static_cast<float>(cursor.last_x),
                                      static_cast<float>(cursor.last_y),
                                      dt);
            break;
        }
        case events::EventType::eMouseScrolled: {
            const auto& e = static_cast<const events::MouseScrolledEvent&>(event);
            mapper.onMouseScroll(static_cast<float>(e.xOffset()),
                                 static_cast<float>(e.yOffset()),
                                 static_cast<float>(cursor.last_x),
                                 static_cast<float>(cursor.last_y),
                                 dt);
            break;
        }
        case events::EventType::eTouchPress: {
            const auto& e = static_cast<const events::TouchPressEvent&>(event);
            cursor.last_x = e.x();
            cursor.last_y = e.y();
            cursor.first = false;
            break;
        }
        case events::EventType::eTouchMove: {
            const auto& e = static_cast<const events::TouchMoveEvent&>(event);
            const float dx = cursor.first ? 0.0f : static_cast<float>(e.x() - cursor.last_x);
            const float dy = cursor.first ? 0.0f : static_cast<float>(e.y() - cursor.last_y);
            cursor.last_x = e.x();
            cursor.last_y = e.y();
            cursor.first = false;
            mapper.onTouchPan(TouchPan{dx, dy}, dt);
            break;
        }
        case events::EventType::eTouchRelease:
            cursor.first = true;
            break;
        default:
            break;
    }
}

}  // namespace vne::interaction
