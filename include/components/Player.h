#pragma once

struct Player {
    // State
    glm::vec3 position = glm::vec3(0.0f, 30.0f, 0.0f);
    float velocityY = 0.0f;
    bool onGround = false;
    bool creativeMode = false;

    // Properties
    const float width = 0.6f;
    const float depth = 0.6f;
    const float height = 1.8f;
    const float eyeHeight = 1.6f;

    // Physics Constants
    const float gravity = 30.0f;
    const float jumpVelocity = 9.0f;
};