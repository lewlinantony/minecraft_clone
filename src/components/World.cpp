#include "components/World.h"


Block* World::getBlock(glm::ivec3 blockPosition) {
    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);


    // Check if the chunk exists
    auto it = chunkMap.find(chunkCoord);
    if (it == chunkMap.end()) {
        return nullptr; // Chunk not found
    }

    glm::ivec3 localPos = blockPosition - chunkCoord;
    // Chunk exists, now get the block's local position
    if (localPos.x < 0 || localPos.x >= CHUNK_SIZE ||
        localPos.y < 0 || localPos.y >= CHUNK_SIZE ||
        localPos.z < 0 || localPos.z >= CHUNK_SIZE){ 
        return nullptr;
    }

    return &it->second.blocks[localPos.x][localPos.y][localPos.z];
}
glm::ivec3 World::getChunkOrigin(glm::ivec3 blockPosition) {
    return glm::ivec3(
        floor(blockPosition.x / (float)CHUNK_SIZE) * CHUNK_SIZE,
        floor(blockPosition.y / (float)CHUNK_SIZE) * CHUNK_SIZE,
        floor(blockPosition.z / (float)CHUNK_SIZE) * CHUNK_SIZE
    );
}
void World::setBlock(glm::ivec3 blockPosition, int type) {
    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);
    // Find the chunk in the map. If it exists, modify it.
    auto it = chunkMap.find(chunkCoord);
    if (it != chunkMap.end()) {
        glm::ivec3 localPos = blockPosition - chunkCoord;
        it->second.blocks[localPos.x][localPos.y][localPos.z].type = type;
    }
}
