#pragma once

#include <glm/glm.hpp>
#include <core/constants.h>

// Forward Declarations
class World;
struct Player;

// Bounding box
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

class Collision {
    public:        
        bool boxBoxOverlap(const BoundingBox& playerBox, const BoundingBox& blockBox) const;
        glm::vec3 resolveYCollision(glm::vec3 nextPlayerPos, glm::vec3 yMovement, World& world, Player& player);
        glm::vec3 resolveXZCollision(glm::vec3 nextPlayerPos, glm::vec3 xzMovement, World& world, Player& player);
        
    private:
        // Collision constants 
        const float collisionGap = 0.01f; 
        const float collisionMargin = 0.5f; 
};