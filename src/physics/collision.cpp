#include <physics/collision.h>
#include <world/world.h>
#include <player/player.h>


bool Collision::boxBoxOverlap(const BoundingBox& box1, const BoundingBox& box2) const {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
           (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
           (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

glm::vec3 Collision::resolveYCollision(glm::vec3 nextPlayerPos, glm::vec3 yMovement, World& world, Player& player) {
    BoundingBox playerBox = BoundingBox::box(
        nextPlayerPos + glm::vec3(0.0f, player.height / 2.0f, 0.0f),
        player.width,
        player.height,
        player.depth
    );
    
    int max_x = static_cast<int>(glm::ceil(playerBox.max.x + collisionMargin));
    int max_y = static_cast<int>(glm::ceil(playerBox.max.y + collisionMargin));
    int max_z = static_cast<int>(glm::ceil(playerBox.max.z + collisionMargin));
    int min_x = static_cast<int>(glm::floor(playerBox.min.x - collisionMargin));
    int min_y = static_cast<int>(glm::floor(playerBox.min.y - collisionMargin));
    int min_z = static_cast<int>(glm::floor(playerBox.min.z - collisionMargin));

    player.onGround = false;

    for (int x = min_x; x < max_x; x++) {
        for (int y = min_y; y < max_y; y++) {
            for (int z = min_z; z < max_z; z++) {
                Block* block = world.getBlock(glm::ivec3(x, y, z));
                if (block && block->type != 0 && block->type != 6) {
                    BoundingBox blockBox = BoundingBox::box(
                        glm::vec3(x, y, z),
                        BLOCK_SIZE,
                        BLOCK_SIZE + 2.0f * collisionGap,
                        BLOCK_SIZE
                    );

                    if (boxBoxOverlap(playerBox, blockBox)) {
                        player.velocityY = 0.0f;
                        if (yMovement.y > 0) { // Moving up
                            nextPlayerPos.y = blockBox.min.y - player.height;
                        } else { // Moving down
                            nextPlayerPos.y = blockBox.max.y;
                            player.onGround = true;
                        }
                        return nextPlayerPos; // Collision handled
                    }
                }
            }
        }
    }
    return nextPlayerPos; // No collision
}

glm::vec3 Collision::resolveXZCollision(glm::vec3 nextPlayerPos, glm::vec3 xzMovement, World& world, Player& player) {

    glm::vec3 resolvedPos = nextPlayerPos;

    // Resolve X-axis collision
    if (xzMovement.x != 0) {
        BoundingBox playerBoxX = BoundingBox::box(
            glm::vec3(resolvedPos.x, player.position.y + player.height / 2.0f, player.position.z),
            player.width,
            player.height,
            player.depth
        );

        int max_x = static_cast<int>(glm::ceil(playerBoxX.max.x + collisionMargin));
        int max_y = static_cast<int>(glm::ceil(playerBoxX.max.y + collisionMargin));
        int max_z = static_cast<int>(glm::ceil(playerBoxX.max.z + collisionMargin));
        int min_x = static_cast<int>(glm::floor(playerBoxX.min.x - collisionMargin));
        int min_y = static_cast<int>(glm::floor(playerBoxX.min.y - collisionMargin));
        int min_z = static_cast<int>(glm::floor(playerBoxX.min.z - collisionMargin));

        for (int x = min_x; x < max_x; x++) {
            for (int y = min_y; y < max_y; y++) {
                for (int z = min_z; z < max_z; z++) {
                    if (world.getBlock({x, y, z}) && world.getBlock({x, y, z})->type != 0 && world.getBlock({x, y, z})->type != 6) {
                        BoundingBox blockBox = BoundingBox::box(
                            glm::vec3(x, y, z),
                            BLOCK_SIZE + 2.0f * collisionGap,
                            BLOCK_SIZE,
                            BLOCK_SIZE
                        );
                        if (boxBoxOverlap(playerBoxX, blockBox)) {
                            if (xzMovement.x > 0) { // Moving right
                                resolvedPos.x = blockBox.min.x - player.width / 2;
                            } else { // Moving left
                                resolvedPos.x = blockBox.max.x + player.width / 2;
                            }
                            goto resolve_z; // Move to Z resolution, cause at a time we can only have one collisoin along an axis, which is the closest to the player same for z
                        }
                    }
                }
            }
        }
    }

resolve_z:
    // Resolve Z-axis collision
    if (xzMovement.z != 0) {
        BoundingBox playerBoxZ = BoundingBox::box(
            glm::vec3(resolvedPos.x, player.position.y + player.height / 2.0f, resolvedPos.z),
            player.width,
            player.height,
            player.depth
        );

        int max_x = static_cast<int>(glm::ceil(playerBoxZ.max.x + collisionMargin));
        int max_y = static_cast<int>(glm::ceil(playerBoxZ.max.y + collisionMargin));
        int max_z = static_cast<int>(glm::ceil(playerBoxZ.max.z + collisionMargin));
        int min_x = static_cast<int>(glm::floor(playerBoxZ.min.x - collisionMargin));
        int min_y = static_cast<int>(glm::floor(playerBoxZ.min.y - collisionMargin));
        int min_z = static_cast<int>(glm::floor(playerBoxZ.min.z - collisionMargin));

        for (int x = min_x; x < max_x; x++) {
            for (int y = min_y; y < max_y; y++) {
                for (int z = min_z; z < max_z; z++) {
                    if (world.getBlock({x, y, z}) && world.getBlock({x, y, z})->type != 0 && world.getBlock({x, y, z})->type != 6) {
                        BoundingBox blockBox = BoundingBox::box(
                            glm::vec3(x, y, z),
                            BLOCK_SIZE,
                            BLOCK_SIZE,
                            BLOCK_SIZE + 2.0f * collisionGap
                        );
                        if (boxBoxOverlap(playerBoxZ, blockBox)) {
                            if (xzMovement.z > 0) { // Moving forward
                                resolvedPos.z = blockBox.min.z - player.depth / 2;
                            } else { // Moving backward
                                resolvedPos.z = blockBox.max.z + player.depth / 2;
                            }
                            goto end_resolve;
                        }
                    }
                }
            }
        }
    }

end_resolve:
    return resolvedPos;
}

