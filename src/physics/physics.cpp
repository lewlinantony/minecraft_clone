#include <physics/physics.h>
#include <world/world.h>
#include <player/player.h>


void Physics::updatePhysics(Player& player, Collision& collision, World& world, float deltaTime, bool& playerMovedChunks) {
    // Check if player is in water
    player.inWater = false;
    {
        std::shared_lock<std::shared_mutex> lock(world.chunkMapMutex);
        glm::ivec3 playerCenter(glm::floor(player.position.x), glm::floor(player.position.y), glm::floor(player.position.z));
        Block* bCenter = world.getBlock(playerCenter);
        glm::ivec3 playerHead(glm::floor(player.position.x), glm::floor(player.position.y + player.eyeHeight), glm::floor(player.position.z));
        Block* bHead = world.getBlock(playerHead);
        
        if ((bCenter && bCenter->type == 6) || (bHead && bHead->type == 6)) {
            player.inWater = true;
        }
    }

    if (player.inWater) {
        // Reduced gravity and damping in water
        player.velocityY -= player.gravity * 0.5f * deltaTime;
        player.velocityY *= 0.9f; 
        if (player.velocityY < -5.0f) {
            player.velocityY = -5.0f;
        }
    } else if (!player.onGround) {
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