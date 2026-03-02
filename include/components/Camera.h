#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// CAMERA
struct Camera {
    // State
    glm::vec3 position; // Will be updated from Player's position
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;

    // Properties
    const float speed = 10.0f;
    const float sensitivity = 0.1f;
    const float maxPitch = 89.0f;

    // Method to be moved from global scope
    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }
};