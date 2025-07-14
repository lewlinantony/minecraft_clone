#pragma once

#include <glm/glm.hpp>

struct BoundingBox {
    glm::vec3 max;
    glm::vec3 min;

    static BoundingBox box(glm::vec3 position, float width, float height, float depth) {
        BoundingBox b;
        glm::vec3 halfSize = glm::vec3(width / 2.0f, height / 2.0f, depth / 2.0f);
        b.min = position - halfSize;
        b.max = position + halfSize;
        return b;
    }
};