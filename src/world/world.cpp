#include <world/world.h>
#include <player/player.h>


// NOTE
// when u call getBlock, if the region of the function call has not locked chunkMap, use a shared_lock to call this function
// if the function call region has already locked chunkMap with a shared_lock, then just call this function without locking again
Block* World::getBlock(glm::ivec3 blockPosition) {
    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);

    // Check if the chunk exists
    auto it = chunkMap.find(chunkCoord);
    if (it == chunkMap.end()) {
        return nullptr; // Chunk not found
    }
    
    // Chunk exists, now get the block's local position
    glm::ivec3 localPos = blockPosition - chunkCoord + glm::ivec3(1); // +1 to account for the padding in the Chunk
    if (localPos.x < 1 || localPos.x > CHUNK_SIZE ||
        localPos.y < 1 || localPos.y > CHUNK_SIZE ||
        localPos.z < 1 || localPos.z > CHUNK_SIZE){ 
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
    std::unique_lock<std::shared_mutex> writeLock(chunkMapMutex); 

    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);

    // Find the chunk in the map. If it exists, modify it.
    auto it = chunkMap.find(chunkCoord);
    if (it != chunkMap.end()) {
        glm::ivec3 localPos = blockPosition - chunkCoord + glm::ivec3(1); // +1 to account for the padding in the Chunk
        it->second.blocks[localPos.x][localPos.y][localPos.z].type = type;
    }
}

void World::generateChunks(glm::vec3 playerPosition) {

    glm::ivec3 playerChunkOrigin = getChunkOrigin(glm::round(playerPosition));

    std::vector<glm::ivec3> chunksToGenerate;  // vector of all chunks around the player that we need to generate data for
    chunksToGenerate.reserve((XZ_LOAD_DIST*2+1) * (XZ_LOAD_DIST*2+1) * (Y_LIMIT*2+1)); // Reserve space for worst case

    for (int cx = -XZ_LOAD_DIST; cx <= XZ_LOAD_DIST; cx++) {
        for (int cz = -XZ_LOAD_DIST; cz <= XZ_LOAD_DIST; cz++) {

            // Use cylindrical distance
            if (cx * cx + cz * cz > XZ_LOAD_DIST * XZ_LOAD_DIST) {
                continue;
            }
            
            for (int y = -Y_LIMIT; y <= Y_LIMIT; y++) {
            //decoupling Y to iterate from Y_LIMIT to -Y_LIMIT cause worlds vertical bounds is fixed and is independent of the players position
                
                glm::ivec3 chunkOrigin = glm::ivec3(
                    playerChunkOrigin.x + (cx * CHUNK_SIZE),
                    y * CHUNK_SIZE, 
                    playerChunkOrigin.z + (cz * CHUNK_SIZE)
                );
                
                if(chunkOrigin.y < -(Y_LIMIT*CHUNK_SIZE) || chunkOrigin.y > Y_LIMIT*CHUNK_SIZE) {
                    continue; // Skip chunks beyond vertical world limits
                }
                
                {
                    std::shared_lock<std::shared_mutex> lock(chunkMapMutex);
                    // If chunk data already exists, skip it
                    if (chunkMap.count(chunkOrigin)) {
                        continue;
                    }
                }

                chunksToGenerate.push_back(chunkOrigin);                
            }
        }
    }

    std::sort(chunksToGenerate.begin(), chunksToGenerate.end(), [playerChunkOrigin](const glm::ivec3& a, const glm::ivec3& b) {
        // Calculate squared distance (cheaper than using sqrt)
        int distA = (a.x - playerChunkOrigin.x)*(a.x - playerChunkOrigin.x) + 
                    (a.y - playerChunkOrigin.y)*(a.y - playerChunkOrigin.y) + 
                    (a.z - playerChunkOrigin.z)*(a.z - playerChunkOrigin.z);
                    
        int distB = (b.x - playerChunkOrigin.x)*(b.x - playerChunkOrigin.x) + 
                    (b.y - playerChunkOrigin.y)*(b.y - playerChunkOrigin.y) + 
                    (b.z - playerChunkOrigin.z)*(b.z - playerChunkOrigin.z);
                    
        return distA < distB; // Sort ascending
    });    

    if (!chunksToGenerate.empty()) {
        for (const auto& chunkOrigin : chunksToGenerate) {
            threadpool->enqueueBackWorkerTask([this, chunkOrigin]{
                generateChunkData(chunkOrigin);
            });
        }
    }    
}

void World::generateChunkData(glm::ivec3 chunkOrigin) {
    Chunk currentChunk;
    for (int x = 1; x < CHUNK_SIZE+1; x++) {
        for (int z = 1; z < CHUNK_SIZE+1; z++) {
            float globalX = (float)(chunkOrigin.x + x);
            float globalZ = (float)(chunkOrigin.z + z);
            float height = noise.GetNoise(globalX, globalZ) * amplitude; // Scale noise to 0-2*amp
            height = glm::round(height);

            for (int y = 1; y < CHUNK_SIZE+1; y++) {
                int globalY = chunkOrigin.y + y;
                if (globalY > height) {
                    currentChunk.blocks[x][y][z].type = 0; // Air
                } else if (globalY == (int)height) {
                    currentChunk.blocks[x][y][z].type = 1; // Grass
                } else if (globalY >= height - 5) {
                    currentChunk.blocks[x][y][z].type = 2; // Dirt
                } else if (globalY >= -(Y_LIMIT*CHUNK_SIZE)) {
                    currentChunk.blocks[x][y][z].type = 3; // Stone
                }
            }
        }
    }

    {
        std::unique_lock<std::shared_mutex> writeLock(chunkMapMutex);
        chunkMap[chunkOrigin] = std::move(currentChunk);
    }    

    
    threadpool->enqueueBackWorkerTask([this, chunkOrigin]{
        calculateChunkMesh(chunkOrigin);
    });
}

void World::calculateChunkAndNeighborsMesh(glm::ivec3 block) {
    glm::ivec3 chunkCoord = getChunkOrigin(block);
    glm::ivec3 blockOffset = block - chunkCoord;

    std::vector<glm::ivec3> chunksToRemesh;
    chunksToRemesh.reserve(4); 

    // Always recalculate the mesh for the chunk the block is in
    chunksToRemesh.push_back(chunkCoord);

    // If the block is on a boundary, the neighbor chunk's mesh is also affected
    if (blockOffset.x == 0) {
        chunksToRemesh.push_back(chunkCoord + glm::ivec3(-CHUNK_SIZE, 0, 0));
    } else if (blockOffset.x == CHUNK_SIZE - 1) {
        chunksToRemesh.push_back(chunkCoord + glm::ivec3(CHUNK_SIZE, 0, 0));
    }
    
    if (blockOffset.y == 0) {
        chunksToRemesh.push_back(chunkCoord + glm::ivec3(0, -CHUNK_SIZE, 0));
    } else if (blockOffset.y == CHUNK_SIZE - 1) {
        chunksToRemesh.push_back(chunkCoord + glm::ivec3(0, CHUNK_SIZE, 0));
    }

    if (blockOffset.z == 0) {
        chunksToRemesh.push_back(chunkCoord + glm::ivec3(0, 0, -CHUNK_SIZE));
    } else if (blockOffset.z == CHUNK_SIZE - 1) {
        chunksToRemesh.push_back(chunkCoord + glm::ivec3(0, 0, CHUNK_SIZE));
    }

    // although it keeps the main thread smooth, it can often causes like hole in the chunk mesh for a split second which is noticable
    for (const auto& coord : chunksToRemesh) {
        threadpool->enqueueFrontWorkerTask([this, coord]{
            calculateChunkMesh(coord);
        });
    }
}

void World::populateChunkPadding(glm::ivec3 chunkCoord){
    Chunk& chunk = chunkMap.at(chunkCoord); 

    const glm::ivec3 neighbourChunks[6] = {
        chunkCoord + glm::ivec3(CHUNK_SIZE, 0, 0),    // Left
        chunkCoord + glm::ivec3(-CHUNK_SIZE, 0, 0),   // Right
        chunkCoord + glm::ivec3(0, CHUNK_SIZE, 0),    // Top
        chunkCoord + glm::ivec3(0, -CHUNK_SIZE, 0),    // Bottom
        chunkCoord + glm::ivec3(0, 0, CHUNK_SIZE),    // Back
        chunkCoord + glm::ivec3(0, 0, -CHUNK_SIZE)   // Front
    };

    auto it = chunkMap.find(neighbourChunks[0]);
    if (it != chunkMap.end()) {
        Chunk& neighbor = it->second;
        for(int y=1; y<CHUNK_SIZE+1; y++) {
            std::memcpy(&chunk.blocks[CHUNK_SIZE+1][y][1], &neighbor.blocks[1][y][1], CHUNK_SIZE * sizeof(Block));                
        }
    }

    auto it = chunkMap.find(neighbourChunks[1]);
    if (it != chunkMap.end()) {
        Chunk& neighbor = it->second;
        for(int y=1; y<CHUNK_SIZE+1; y++) {
            std::memcpy(&chunk.blocks[0][y][1], &neighbor.blocks[CHUNK_SIZE][y][1], CHUNK_SIZE * sizeof(Block));                
        }
    }   

    auto it = chunkMap.find(neighbourChunks[2]);
    if (it != chunkMap.end()) {
        Chunk& neighbor = it->second;
        for(int x=1; x<CHUNK_SIZE+1; x++) {
            std::memcpy(&chunk.blocks[x][CHUNK_SIZE+1][1], &neighbor.blocks[x][1][1], CHUNK_SIZE * sizeof(Block));
        }
    }

    auto it = chunkMap.find(neighbourChunks[3]);
    if (it != chunkMap.end()) {
        Chunk& neighbor = it->second;
        for(int x=1; x<CHUNK_SIZE+1; x++) {
            std::memcpy(&chunk.blocks[x][0][1], &neighbor.blocks[x][CHUNK_SIZE][1], CHUNK_SIZE * sizeof(Block));
        }
    }

    if (chunkMap.find(neighbourChunks[4]) != chunkMap.end()) {
        for(int x=1; x<CHUNK_SIZE+1; x++) {
            for(int y=1; y<CHUNK_SIZE+1; y++) {
                chunk.blocks[x][y][CHUNK_SIZE+1] = chunkMap.at(neighbourChunks[4]).blocks[x][y][1];
            }
        }
    }

    if (chunkMap.find(neighbourChunks[5]) != chunkMap.end()) {
        for(int x=1; x<CHUNK_SIZE+1; x++) {
            for(int y=1; y<CHUNK_SIZE+1; y++) {
                chunk.blocks[x][y][0] = chunkMap.at(neighbourChunks[5]).blocks[x][y][CHUNK_SIZE];
            }
        }
    }
    
}

void World::calculateChunkMesh(glm::ivec3 chunkCoord) {
    
    std::vector<float> meshData;
    meshData.reserve((CHUNK_SIZE+2) * (CHUNK_SIZE+2) * (CHUNK_SIZE+2) * 6 * 10); // Reserve space for worst case (all blocks visible, 6 faces each, 10 floats per vertex)

    {
        std::shared_lock<std::shared_mutex> lock(chunkMapMutex);

        // Ensure the chunk exists in the map
        if (chunkMap.find(chunkCoord) == chunkMap.end()) {
            return; // Cannot mesh a chunk that hasn't had its block data generated
        }

        Chunk& chunk = chunkMap.at(chunkCoord);

        populateChunkPadding(chunkCoord); // Fill the padding layer of the chunks

        const glm::vec3 normals[6] = {
            glm::vec3(0.0f, 1.0f, 0.0f),    // Top (+Y)
            glm::vec3(0.0f, 0.0f, -1.0f),   // Front (-Z)
            glm::vec3(-1.0f, 0.0f, 0.0f),   // Right (-X)
            glm::vec3(0.0f, 0.0f, 1.0f),    // Back (+Z)
            glm::vec3(1.0f, 0.0f, 0.0f),    // Left (+X)
            glm::vec3(0.0f, -1.0f, 0.0f)    // Bottom (-Y)
        };    


        for (int x = 1; x < CHUNK_SIZE+1; x++) {
            for (int y = 1; y < CHUNK_SIZE+1; y++) {
                for (int z = 1; z < CHUNK_SIZE+1; z++) {
                    if (chunk.blocks[x][y][z].type == 0) continue; // Skip air blocks

                    glm::ivec3 blockPosition = chunkCoord + glm::ivec3(x, y, z);
                    uint8_t faces = getVisibleFaces(blockPosition); // Get visible faces bitmask for this block

                    for (int faceID=0; faceID<6; faceID++) {
                        
                        if ((faces & (1 << faceID)) == 0) continue; // This face is not visible, skip it

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

                            meshData.insert(meshData.end(), {
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
        
    }

    threadpool->enqueueMainTask([this, chunkCoord, meshData = std::move(meshData)]() mutable {
        uploadChunkMesh(chunkCoord, meshData);
    });
}

void World::uploadChunkMesh(glm::ivec3 chunkCoord, std::vector<float> meshData) {

    GLuint chunkVAO, chunkVBO;
    // Check if the chunk already has a VAO/VBO.
    if (chunkVaoMap.find(chunkCoord) == chunkVaoMap.end()) {
        renderer.initWorldObjects(chunkVAO, chunkVBO, chunkCoord, *this);
    } else {
        // The chunk already exists, so just get its VBO handle for updating.
        chunkVBO = chunkVboMap.at(chunkCoord);
        glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
    }

    // update vertex count on main thread
    chunkVertexCountMap[chunkCoord] = meshData.size() / 10; // 10 floats per vertex
    
    // Upload the new vertex data to the VBO
    if (chunkVertexCountMap[chunkCoord]) {
        glBufferData(GL_ARRAY_BUFFER, meshData.size() * sizeof(float), meshData.data(), GL_DYNAMIC_DRAW);
    }
}

uint8_t World::getVisibleFaces(glm::ivec3 block) {
    uint8_t visibleFaces = 0;

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

        // never render faces at and beyond the vertical world limits cause the player can't see them
        if (i==5 && neighborPos.y <= -(Y_LIMIT*CHUNK_SIZE)) {
            continue;
        }
     
        else if (!neighborBlock || neighborBlock->type == 0) {// If chunk doesn't exist or block is air
            visibleFaces |= (1 << i); // Mark this bit's face as visible
        }

    }
    return visibleFaces;
}

void World::init(glm::vec3& playerPosition, Threadpool* threadpoolPtr) {
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

    threadpool = threadpoolPtr;

    // Generate the initial terrain around the player
    generateChunks(playerPosition);       
}

void World::cleanup() {
    // Delete all OpenGL objects
    for (auto& pair : chunkVboMap) glDeleteBuffers(1, &pair.second);
    for (auto& pair : chunkVaoMap) glDeleteVertexArrays(1, &pair.second);
}