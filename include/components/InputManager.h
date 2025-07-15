#pragma once

#include "config.h"


// INPUT MANAGERS
struct InputManager {
    // Mouse State
    bool firstMouse = true;
    float lastX = SCREEN_WIDTH / 2.0f;
    float lastY = SCREEN_HEIGHT / 2.0f;

    // Keyboard/Mouse Button State
    bool spaceWasPressed = false;
    bool tabWasPressed = false;
    bool mouseLeftWasPressed = false;
    bool mouseRightWasPressed = false;
};