#include <physics/physics.h>
#include <world/world.h>
#include <player/player.h>


void Physics::updatePhysics(Player& player, Collision& collision, World& world, float deltaTime, bool& playerMovedChunks) {
    // Apply gravity if not on the ground
    if (!player.onGround) {
        player.velocityY -= player.gravity * deltaTime;
        // Terminal velocity
        if (player.velocityY < -50.0f) {
            player.velocityY = -50.0f;
        }
    }

    glm::vec3 y_movement = glm::vec3(0.0f, player.velocityY * deltaTime, 0.0f);
    glm::vec3 nextPlayerPosition = player.position + y_movement;
    
    glm::vec3 oldPos = player.position;
    player.position = collision.resolveYCollision(nextPlayerPosition, y_movement, world, player);

    // Check if player crossed a chunk boundary after physics update
    if (world.getChunkOrigin(player.position) != world.getChunkOrigin(oldPos)) {
        playerMovedChunks = true;
    }
}