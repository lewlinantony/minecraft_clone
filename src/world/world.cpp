#include <world/world.h>
#include <player/player.h>


Block* World::getBlock(glm::ivec3 blockPosition) {
    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);

    // Check if the chunk exists
    auto it = chunkMap.find(chunkCoord);
    if (it == chunkMap.end()) {
        return nullptr; // Chunk not found
    }
    
    // Chunk exists, now get the block's local position
    glm::ivec3 localPos = blockPosition - chunkCoord;
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

void World::generateTerrain(glm::vec3 playerPosition) {
    std::vector<glm::ivec3> newChunksToMesh;
    glm::ivec3 playerChunkOrigin = getChunkOrigin(glm::round(playerPosition));

    for (int cx = -XZ_LOAD_DIST; cx <= XZ_LOAD_DIST; cx++) {
        for (int cy = -Y_LOAD_DIST; cy <= Y_LOAD_DIST; cy++) {
            for (int cz = -XZ_LOAD_DIST; cz <= XZ_LOAD_DIST; cz++) {
                // Use cylindrical distance
                if (cx * cx + cz * cz > XZ_LOAD_DIST * XZ_LOAD_DIST) {
                    continue;
                }
                
                glm::ivec3 chunkOrigin = playerChunkOrigin + glm::ivec3(cx, cy, cz) * CHUNK_SIZE;
                // If chunk data already exists, skip it
                if (chunkMap.count(chunkOrigin)) {
                    continue;
                }
                
                // Chunk does not exist, so generate its block data
                Chunk& currentChunk = chunkMap[chunkOrigin];
                newChunksToMesh.push_back(chunkOrigin);

                for (int x = 0; x < CHUNK_SIZE; x++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        float globalX = (float)(chunkOrigin.x + x);
                        float globalZ = (float)(chunkOrigin.z + z);
                        float height = noise.GetNoise(globalX, globalZ) * amplitude; // Scale noise to 0-2*amp
                        height = glm::round(height);

                        for (int y = 0; y < CHUNK_SIZE; y++) {
                            int globalY = chunkOrigin.y + y;
                            if (globalY > height) {
                                currentChunk.blocks[x][y][z].type = 0; // Air
                            } else if (globalY == (int)height) {
                                currentChunk.blocks[x][y][z].type = 1; // Grass
                            } else if (globalY >= height - 5) {
                                currentChunk.blocks[x][y][z].type = 2; // Dirt
                            } else if (globalY >= -40) {
                                currentChunk.blocks[x][y][z].type = 3; // Stone
                            }
                        }
                    }
                }
            }
        }
    }

    // After generating block data, create the meshes for the new chunks
    for (const auto& chunkCoord : newChunksToMesh) {
        calculateChunk(chunkCoord, playerPosition);
    }
}

void World::calculateChunkAndNeighbors(glm::ivec3 block, glm::vec3 playerPosition) {
    glm::ivec3 chunkCoord = getChunkOrigin(block);
    glm::ivec3 blockOffset = block - chunkCoord;

    // Always recalculate the mesh for the chunk the block is in
    calculateChunk(chunkCoord, playerPosition);

    // If the block is on a boundary, the neighbor chunk's mesh is also affected
    if (blockOffset.x == 0) {
        calculateChunk(chunkCoord + glm::ivec3(-CHUNK_SIZE, 0, 0), playerPosition);
    } else if (blockOffset.x == CHUNK_SIZE - 1) {
        calculateChunk(chunkCoord + glm::ivec3(CHUNK_SIZE, 0, 0), playerPosition);
    }
    
    if (blockOffset.y == 0) {
        calculateChunk(chunkCoord + glm::ivec3(0, -CHUNK_SIZE, 0), playerPosition);
    } else if (blockOffset.y == CHUNK_SIZE - 1) {
        calculateChunk(chunkCoord + glm::ivec3(0, CHUNK_SIZE, 0), playerPosition);
    }

    if (blockOffset.z == 0) {
        calculateChunk(chunkCoord + glm::ivec3(0, 0, -CHUNK_SIZE), playerPosition);
    } else if (blockOffset.z == CHUNK_SIZE - 1) {
        calculateChunk(chunkCoord + glm::ivec3(0, 0, CHUNK_SIZE), playerPosition);
    }
}

void World::calculateChunk(glm::ivec3 chunkCoord, glm::vec3 playerPosition) {
    // Clear any previous mesh data for this chunk
    chunkMeshData[chunkCoord].clear();
    
    // Ensure the chunk exists in the map
    if (chunkMap.find(chunkCoord) == chunkMap.end()) {
        return; // Cannot mesh a chunk that hasn't had its block data generated
    }
    Chunk& chunk = chunkMap.at(chunkCoord);

    const glm::vec3 normals[6] = {
        glm::vec3(0.0f, 1.0f, 0.0f),    // Top (+Y)
        glm::vec3(0.0f, 0.0f, -1.0f),   // Front (-Z)
        glm::vec3(-1.0f, 0.0f, 0.0f),   // Right (-X)
        glm::vec3(0.0f, 0.0f, 1.0f),    // Back (+Z)
        glm::vec3(1.0f, 0.0f, 0.0f),    // Left (+X)
        glm::vec3(0.0f, -1.0f, 0.0f)    // Bottom (-Y)
    };    

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (chunk.blocks[x][y][z].type == 0) continue; // Skip air blocks

                glm::ivec3 blockPosition = chunkCoord + glm::ivec3(x, y, z);
                std::vector<int> faces = getVisibleFaces(blockPosition, playerPosition);
                for (int faceID : faces) {
                    const float* curFace = faceVertices[faceID];
                    if (!curFace) continue;

                    const glm::vec3 normal = normals[faceID];

                    for (int i = 0; i < 6; ++i) { // 6 vertices per face
                        int idx = i * 6; // 6 attributes per vertex in face data

                        // Vertex position 
                        float vx = curFace[idx + 0] + blockPosition.x;
                        float vy = curFace[idx + 1] + blockPosition.y;
                        float vz = curFace[idx + 2] + blockPosition.z;

                        // Texture coordinates
                        float ux = curFace[idx + 3];
                        float uy = curFace[idx + 4];
                        
                        // Face ID and Block Type
                        float fid = curFace[idx + 5];
                        float blockType = static_cast<float>(chunk.blocks[x][y][z].type);

                        chunkMeshData[chunkCoord].insert(chunkMeshData[chunkCoord].end(), {
                            vx, vy, vz,           // Position
                            ux, uy,               // UV Coords
                            fid,                  // Face ID
                            blockType,            // Block Type
                            normal.x, normal.y, normal.z // Normal
                        });
                    }
                }
            }
        }
    }

    GLuint chunkVAO, chunkVBO;

    // Check if the chunk already has a VAO/VBO.
    if (chunkVaoMap.find(chunkCoord) == chunkVaoMap.end()) {
        renderer.initWorldObjects(chunkVAO, chunkVBO, chunkCoord, *this);
    } else {
        // The chunk already exists, so just get its VBO handle for updating.
        chunkVBO = chunkVboMap.at(chunkCoord);
        glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
    }
    
    // Upload the new vertex data to the VBO
    if (!chunkMeshData[chunkCoord].empty()) {
        glBufferData(GL_ARRAY_BUFFER, chunkMeshData[chunkCoord].size() * sizeof(float), chunkMeshData[chunkCoord].data(), GL_DYNAMIC_DRAW);
    }
}

std::vector<int> World::getVisibleFaces(glm::ivec3 block, glm::vec3 playerPosition) {
    std::vector<int> visibleFaces;
    // Directions corresponding to face IDs 0 through 5
    const glm::ivec3 directions[6] = {
        glm::ivec3(0, 1, 0),    // Top (+Y)
        glm::ivec3(0, 0, -1),   // Front (-Z)
        glm::ivec3(-1, 0, 0),   // Right (-X)
        glm::ivec3(0, 0, 1),    // Back (+Z)
        glm::ivec3(1, 0, 0),    // Left (+X)
        glm::ivec3(0, -1, 0)    // Bottom (-Y)
    };

    for (int i = 0; i < 6; ++i) {
        glm::ivec3 neighborPos = block + directions[i];
        Block* neighborBlock = getBlock(neighborPos);

        // to be reviewed
        // if ((i == 5 && neighborPos.y < playerPosition.y)) {     // Bottom
        //     continue;
        // }

        if (neighborBlock == nullptr || neighborBlock->type == 0) {// If chunk doesn't exist or block is air
            visibleFaces.push_back(i);
        }
    }
    return visibleFaces;
}

void World::init(glm::vec3& playerPosition) {
    // Configure the noise generator
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(g_NoiseSeed);
    noise.SetFractalOctaves(g_NoiseOctaves);
    noise.SetFractalGain(g_NoiseGain);
    noise.SetFractalLacunarity(g_NoiseLacunarity);
    noise.SetFrequency(g_NoiseFrequency);

    // Position the player above the terrain at (0,0)
    playerPosition = glm::vec3(0.0f, (noise.GetNoise(0.0f, 0.0f) + 1. * amplitude) + 3.0f, 0.0f);

    // Generate the initial terrain around the player
    generateTerrain(playerPosition);       
}

void World::cleanup() {
    // Delete all OpenGL objects
    for (auto& pair : chunkVboMap) glDeleteBuffers(1, &pair.second);
    for (auto& pair : chunkVaoMap) glDeleteVertexArrays(1, &pair.second);
}