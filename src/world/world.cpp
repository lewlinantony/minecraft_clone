#include <world/world.h>
#include <player/player.h>
#include <iostream>

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
    std::unique_lock<std::shared_mutex> writeLock(chunkMapMutex); 

    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);

    // Find the chunk in the map. If it exists, modify it.
    auto it = chunkMap.find(chunkCoord);
    if (it != chunkMap.end()) {
        glm::ivec3 localPos = blockPosition - chunkCoord;
        it->second.blocks[localPos.x][localPos.y][localPos.z].type = type;
    }
}

void World::generateChunks(glm::vec3 playerPosition) {

    glm::ivec3 playerChunkOrigin = getChunkOrigin(glm::round(playerPosition));

    std::vector<glm::ivec3> chunksToGenerate;  // vector of all chunks around the player that we need to generate data for
    chunksToGenerate.reserve((XZ_LOAD_DIST*2+1) * (XZ_LOAD_DIST*2+1) * (Y_LIMIT*2+1)); // Reserve space for worst case

    {   
        std::shared_lock<std::shared_mutex> lock(chunkMapMutex); // put it outside the loop to avoid locking a shit ton of times which did cause lag in the main thread

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
                    
                    // If chunk data already exists, skip it
                    if (chunkMap.count(chunkOrigin)) {
                        continue;
                    }
    
                    chunksToGenerate.push_back(chunkOrigin);                
                }
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

    // precompute the height values for each x,z column in the chunk to save some redundant noise calculations in the inner loop
    float localHeights[CHUNK_SIZE][CHUNK_SIZE]; 
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float globalX = (float)(chunkOrigin.x + x);
            float globalZ = (float)(chunkOrigin.z + z);

            warpNoise.DomainWarp(globalX, globalZ);
            float noiseVal = baseNoise.GetNoise(globalX, globalZ);
            localHeights[x][z] = 64 + static_cast<int>(noiseVal * 30.0f);
        }
    }    
    

    // maintain x y z order in the loops to make sure the memory access pattern is cache friendly
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            int globalY = chunkOrigin.y + y;
            for (int z = 0; z < CHUNK_SIZE; z++) {

                int height = localHeights[x][z]; // use precomputed height value

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

    currentChunk.state = CHUNK_STATE::GENERATED; // mark chunk as generated

    {
        std::unique_lock<std::shared_mutex> writeLock(chunkMapMutex);
        chunkMap[chunkOrigin] = std::move(currentChunk);
    }    

    // try to calculate the mesh for current chunk(mostly fails cause the neighbours ususally arent generated yet)
    tryCalculateChunkMesh(chunkOrigin);   
    
    for(auto neighbourOffset : neighbourChunks) {
        glm::ivec3 neighbourCoord = chunkOrigin + neighbourOffset;
        tryCalculateChunkMesh(neighbourCoord); // try to calculate the mesh for the neighbours (this is likely where most generation happens)
    }
}

void World::tryCalculateChunkMesh(glm::ivec3 chunkCoord) {

    bool canMesh = false;
    {
        std::shared_lock<std::shared_mutex> lock(chunkMapMutex);

        auto it = chunkMap.find(chunkCoord);
        if (it != chunkMap.end() && it->second.state == CHUNK_STATE::GENERATED) {            
            canMesh = true;

            for(auto neighbourOffset : neighbourChunks) {
                glm::ivec3 neighbourCoord = chunkCoord + neighbourOffset;
                
                if (neighbourCoord.y < -(Y_LIMIT*CHUNK_SIZE) || neighbourCoord.y > Y_LIMIT*CHUNK_SIZE) {
                        continue; // Skip neighbor chunks beyond vertical world limits
                }
                
                it = chunkMap.find(neighbourCoord);
                if (it == chunkMap.end() || it->second.state == CHUNK_STATE::EMPTY) {
                    canMesh = false; // if any neighbour is not generated, we cannot mesh this chunk yet
                    break;
                }
            }

        }

    }
    if (canMesh) {
        std::unique_lock<std::shared_mutex> writeLock(chunkMapMutex);

        if (chunkMap[chunkCoord].state != CHUNK_STATE::GENERATED) {
            return; // another thread couldve meshed while we unlocked
        }
            
        chunkMap[chunkCoord].state = CHUNK_STATE::MESHED; 

        threadpool->enqueueFrontWorkerTask([this, chunkCoord]{
            calculateChunkMesh(chunkCoord);
        });   
    }

}

void World::updateChunkAndNeighboursMesh(glm::ivec3 block) {
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

    for (const auto& coord : chunksToRemesh) {
        threadpool->enqueueFrontWorkerTask([this, coord]{
            calculateChunkMesh(coord);
        });
    }
}

void World::populateChunkBitMask(Chunk& chunk, glm::ivec3 chunkCoord, u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2]){
    // create bitmask representation of chunk, where 1 represents a solid block, 0 represents air
    for(int x=0; x<CHUNK_SIZE; x++){
        for(int y=0; y<CHUNK_SIZE; y++){
            for(int z=0; z<CHUNK_SIZE; z++){
                if (chunk.blocks[x][y][z].type != 0) {
                    x_solid_mask[y+1][z+1] |= (1ULL << (x+1)); // +1 for padding offset(this is what we subtract when calculating actual blockPos in mesh generation)
                    y_solid_mask[x+1][z+1] |= (1ULL << (y+1)); 
                    z_solid_mask[x+1][y+1] |= (1ULL << (z+1)); 
                }
            }
        }
    }
}

void World::populateChunkBitMaskPadding(Chunk& chunk, glm::ivec3 chunkCoord, u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2]){
    
    glm::ivec3 rightNeightbour = chunkCoord + neighbourChunks[0];
    auto it = chunkMap.find(rightNeightbour);
    if (it != chunkMap.end()) {
        for(int y=0; y<CHUNK_SIZE; y++){
            for(int z=0; z<CHUNK_SIZE; z++){
                if (it->second.blocks[0][y][z].type != 0) {
                    x_solid_mask[y+1][z+1] |= (1ULL << (CHUNK_SIZE+1)); // 0th index of neighbour goes in CHUNK_SIZE+1 index of current chunk's mask padding
                }
            }
        }
    }   

    glm::ivec3 leftNeightbour = chunkCoord + neighbourChunks[1];
    it = chunkMap.find(leftNeightbour);
    if (it != chunkMap.end()) {
        for(int y=0; y<CHUNK_SIZE; y++){
            for(int z=0; z<CHUNK_SIZE; z++){
                if (it->second.blocks[CHUNK_SIZE-1][y][z].type != 0) {
                    x_solid_mask[y+1][z+1] |= (1ULL << 0); // CHUNK_SIZE-1 index of neighbour goes in 0th index of current chunk's mask padding
                }
            }
        }
    }

    glm::ivec3 topNeightbour = chunkCoord + neighbourChunks[2];
    it = chunkMap.find(topNeightbour);
    if (it != chunkMap.end()) {
        for(int x=0; x<CHUNK_SIZE; x++){
            for(int z=0; z<CHUNK_SIZE; z++){
                if (it->second.blocks[x][0][z].type != 0) {
                    y_solid_mask[x+1][z+1] |= (1ULL << (CHUNK_SIZE+1)); 
                }
            }
        }
    }

    glm::ivec3 bottomNeightbour = chunkCoord + neighbourChunks[3];
    it = chunkMap.find(bottomNeightbour);
    if (it != chunkMap.end()) {
        for(int x=0; x<CHUNK_SIZE; x++){
            for(int z=0; z<CHUNK_SIZE; z++){
                if (it->second.blocks[x][CHUNK_SIZE-1][z].type != 0) {
                    y_solid_mask[x+1][z+1] |= (1ULL << 0); 
                }
            }
        }
    }

    glm::ivec3 backNeightbour = chunkCoord + neighbourChunks[4];
    it = chunkMap.find(backNeightbour);
    if (it != chunkMap.end()) {
        for(int x=0; x<CHUNK_SIZE; x++){
            for(int y=0; y<CHUNK_SIZE; y++){
                if (it->second.blocks[x][y][0].type != 0) {
                    z_solid_mask[x+1][y+1] |= (1ULL << (CHUNK_SIZE+1)); 
                }
            }
        }
    }

    glm::ivec3 frontNeightbour = chunkCoord + neighbourChunks[5];
    it = chunkMap.find(frontNeightbour);
    if (it != chunkMap.end()) {
        for(int x=0; x<CHUNK_SIZE; x++){
            for(int y=0; y<CHUNK_SIZE; y++){
                if (it->second.blocks[x][y][CHUNK_SIZE-1].type != 0) {
                    z_solid_mask[x+1][y+1] |= (1ULL << 0); 
                }
            }
        }
    }
}

void World::bitMaskFaceCulling(Chunk& chunk, glm::ivec3 chunkCoord, u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], std::vector<float>& meshData){
    
    u_int64_t FILTER = ((1ULL << CHUNK_SIZE) - 1) << 1; // Mask to ignore the padding bits (0 and CHUNK_SIZE+1)
    
    // +X and -X faces
    for(int y=1; y<CHUNK_SIZE+1; y++){
        for(int z=1; z<CHUNK_SIZE+1; z++){
            
            // take a 64-bit row from the bitmask
            uint64_t row = x_solid_mask[y][z];

            // use bitwise operations to find which blocks in the row have visible +X and -X faces
            uint64_t rightVisible = (row & ~(row >> 1)) & FILTER;
            uint64_t leftVisible  = (row & ~(row << 1)) & FILTER;

            // iterate and add the visible faces to the mesh data
            while(rightVisible){
                int bit_index = __builtin_ctzll(rightVisible); // Count trailing zeros which gives the index of the least significant set bit
                int x = bit_index; 

                int faceID = 0;
                const float* curFace = faceVertices[faceID];
                const glm::vec3 normal = normals[faceID];

                glm::ivec3 blockPos = chunkCoord + glm::ivec3(x, y, z) - glm::ivec3(1); // -1 for the padding we added while populating the bitmask to make room for neighboring blocks
                float blockType = static_cast<float>(chunk.blocks[x-1][y-1][z-1].type);

                for (int i = 0; i < 6; ++i) { // 6 vertices per face
                    int idx = i * 6; // 6 attributes per vertex in face data                    

                    // Vertex position 
                    float vx = curFace[idx + 0] + blockPos.x;
                    float vy = curFace[idx + 1] + blockPos.y;
                    float vz = curFace[idx + 2] + blockPos.z;

                    // Texture coordinates
                    float ux = curFace[idx + 3];
                    float uy = curFace[idx + 4];
                    
                    // Face ID 
                    float fid = curFace[idx + 5];
                    

                    meshData.insert(meshData.end(), {
                        vx, vy, vz,           // Position
                        ux, uy,               // UV Coords
                        fid,                  // Face ID
                        blockType,            // Block Type
                        normal.x, normal.y, normal.z // Normal
                    });
                }                   
                rightVisible &= ~(1ULL << bit_index); // Clear the least significant set bit
            }
            
            while(leftVisible){
                int bit_index = __builtin_ctzll(leftVisible); // Count trailing zeros to find the index of the least significant set bit
                int x = bit_index; 

                int faceID = 1;
                const float* curFace = faceVertices[faceID];
                const glm::vec3 normal = normals[faceID];

                glm::ivec3 blockPos = chunkCoord + glm::ivec3(x, y, z) - glm::ivec3(1); 
                float blockType = static_cast<float>(chunk.blocks[x-1][y-1][z-1].type);

                for (int i = 0; i < 6; ++i) { // 6 vertices per face
                    int idx = i * 6; // 6 attributes per vertex in face data                   

                    // Vertex position 
                    float vx = curFace[idx + 0] + blockPos.x;
                    float vy = curFace[idx + 1] + blockPos.y;
                    float vz = curFace[idx + 2] + blockPos.z;

                    // Texture coordinates
                    float ux = curFace[idx + 3];
                    float uy = curFace[idx + 4];
                    
                    // Face ID 
                    float fid = curFace[idx + 5];

                    meshData.insert(meshData.end(), {
                        vx, vy, vz,           // Position
                        ux, uy,               // UV Coords
                        fid,                  // Face ID
                        blockType,            // Block Type
                        normal.x, normal.y, normal.z // Normal
                    });
                }                   
                leftVisible &= ~(1ULL << bit_index); // Clear the least significant set bit
            }
        }
    }
    
    // +Y and -Y faces
    for(int x=1; x<CHUNK_SIZE+1; x++){
        for(int z=1; z<CHUNK_SIZE+1; z++){
            uint64_t row = y_solid_mask[x][z];

            uint64_t topVisible = (row & ~(row >> 1)) & FILTER;
            uint64_t bottomVisible = (row & ~(row << 1)) & FILTER;


            while(topVisible){
                int bit_index = __builtin_ctzll(topVisible);
                int y = bit_index;

                int faceID = 2;
                const float* curFace = faceVertices[faceID];
                const glm::vec3 normal = normals[faceID];

                float blockType = static_cast<float>(chunk.blocks[x-1][y-1][z-1].type);
                glm::ivec3 blockPos = chunkCoord + glm::ivec3(x, y, z) - glm::ivec3(1); 

                for(int i=0; i<6; i++){
                    int idx = i * 6; // 6 attributes per vertex in face data

                    // Vertex position 
                    float vx = curFace[idx + 0] + blockPos.x;
                    float vy = curFace[idx + 1] + blockPos.y;
                    float vz = curFace[idx + 2] + blockPos.z;

                    // Texture coordinates
                    float ux = curFace[idx + 3];
                    float uy = curFace[idx + 4];
                    
                    // Face ID 
                    float fid = curFace[idx + 5];

                    meshData.insert(meshData.end(), {
                        vx, vy, vz,           // Position
                        ux, uy,               // UV Coords
                        fid,                  // Face ID
                        blockType,            // Block Type
                        normal.x, normal.y, normal.z // Normal
                    });                        
                }
                topVisible &= ~(1ULL << bit_index);
            }

            while(bottomVisible){
                int bit_index = __builtin_ctzll(bottomVisible);
                int y = bit_index;

                glm::ivec3 blockPos = chunkCoord + glm::ivec3(x, y, z) - glm::ivec3(1); 

                // Skip the -y faces of blocks at and beyond vertical world limits
                if (blockPos.y <= -(Y_LIMIT*CHUNK_SIZE)){
                    bottomVisible &= ~(1ULL << bit_index);
                    continue; 
                }

                float blockType = static_cast<float>(chunk.blocks[x-1][y-1][z-1].type);

                int faceID = 3;
                const float* curFace = faceVertices[faceID];
                const glm::vec3 normal = normals[faceID];                 

                for (int i = 0; i < 6; ++i) { // 6 vertices per face
                    int idx = i * 6; // 6 attributes per vertex in face data                   

                    // Vertex position 
                    float vx = curFace[idx + 0] + blockPos.x;
                    float vy = curFace[idx + 1] + blockPos.y;
                    float vz = curFace[idx + 2] + blockPos.z;

                    // Texture coordinates
                    float ux = curFace[idx + 3];
                    float uy = curFace[idx + 4];
                    
                    // Face ID 
                    float fid = curFace[idx + 5];

                    meshData.insert(meshData.end(), {
                        vx, vy, vz,           // Position
                        ux, uy,               // UV Coords
                        fid,                  // Face ID
                        blockType,            // Block Type
                        normal.x, normal.y, normal.z // Normal
                    });
                }   

                bottomVisible &= ~(1ULL << bit_index);
            }
        }
    }

    // +Z and -Z faces
    for(int x=1; x<CHUNK_SIZE+1; x++){
        for(int y=1; y<CHUNK_SIZE+1; y++){
            uint64_t row = z_solid_mask[x][y];

            uint64_t backVisible = (row & ~(row >> 1)) & FILTER;
            uint64_t frontVisible = (row & ~(row << 1)) & FILTER;

            while(backVisible){
                int bit_index = __builtin_ctzll(backVisible);
                int z = bit_index;

                int faceID = 4;
                const float* curFace = faceVertices[faceID];
                const glm::vec3 normal = normals[faceID]; 

                glm::ivec3 blockPos = chunkCoord + glm::ivec3(x, y, z) - glm::ivec3(1); 
                float blockType = static_cast<float>(chunk.blocks[x-1][y-1][z-1].type);

                for (int i = 0; i < 6; ++i) { // 6 vertices per face
                    int idx = i * 6; // 6 attributes per vertex in face data                    

                    // Vertex position 
                    float vx = curFace[idx + 0] + blockPos.x;
                    float vy = curFace[idx + 1] + blockPos.y;
                    float vz = curFace[idx + 2] + blockPos.z;

                    // Texture coordinates
                    float ux = curFace[idx + 3];
                    float uy = curFace[idx + 4];
                    
                    // Face ID 
                    float fid = curFace[idx + 5];
                    

                    meshData.insert(meshData.end(), {
                        vx, vy, vz,           // Position
                        ux, uy,               // UV Coords
                        fid,                  // Face ID
                        blockType,            // Block Type
                        normal.x, normal.y, normal.z // Normal
                    });
                }   

                backVisible &= ~(1ULL << bit_index);
            }

            while(frontVisible){
                int bit_index = __builtin_ctzll(frontVisible);
                int z = bit_index;
                
                int faceID = 5;
                const float* curFace = faceVertices[faceID];
                const glm::vec3 normal = normals[faceID]; 

                glm::ivec3 blockPos = chunkCoord + glm::ivec3(x, y, z) - glm::ivec3(1); 
                float blockType = static_cast<float>(chunk.blocks[x-1][y-1][z-1].type);

                for (int i = 0; i < 6; ++i) { // 6 vertices per face
                    int idx = i * 6; // 6 attributes per vertex in face data

                    // Vertex position 
                    float vx = curFace[idx + 0] + blockPos.x;
                    float vy = curFace[idx + 1] + blockPos.y;
                    float vz = curFace[idx + 2] + blockPos.z;

                    // Texture coordinates
                    float ux = curFace[idx + 3];
                    float uy = curFace[idx + 4];
                    
                    // Face ID and Block Type
                    float fid = curFace[idx + 5];
                    

                    meshData.insert(meshData.end(), {
                        vx, vy, vz,           // Position
                        ux, uy,               // UV Coords
                        fid,                  // Face ID
                        blockType,            // Block Type
                        normal.x, normal.y, normal.z // Normal
                    });
                }   

                frontVisible &= ~(1ULL << bit_index);
            }
        
        }
    }             

}

void World::calculateChunkMesh(glm::ivec3 chunkCoord) {

    std::vector<float> meshData;
    constexpr int FLOATS_PER_FACE = 6 * 10; // 6 verts, 10 floats each
    const int FACES_PER_XZ_CELL_EST = 2; // calculated guess
    meshData.reserve(FLOATS_PER_FACE * FACES_PER_XZ_CELL_EST * CHUNK_SIZE * CHUNK_SIZE);

    {
        std::unique_lock<std::shared_mutex> lock(chunkMapMutex);

        // Ensure the chunk exists in the map
        if (chunkMap.find(chunkCoord) == chunkMap.end()) {
            return; // Cannot mesh a chunk that hasn't had its block data generated
        }

        Chunk& chunk = chunkMap.at(chunkCoord);

        // bitmask arrays where a bit represents a solid block 0 represents air
        // we define the chunk in 3 different orientations(x, y, z) to make it easier to make it easier to 
        // iterate and cull faces accross all three axises
        u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2] = {0}; // +2 for the padding
        u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2] = {0};
        u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2] = {0};

        // populate the bitmask arrays with current chunk data
        populateChunkBitMask(chunk, chunkCoord, x_solid_mask, y_solid_mask, z_solid_mask);

        // populate the bitmask padding with neighbor chunk data to allow proper face culling at chunk borders
        populateChunkBitMaskPadding(chunk, chunkCoord, x_solid_mask, y_solid_mask, z_solid_mask);

        // use the bitmask arrays to determine which faces of each block are visible and should be included in the mesh
        bitMaskFaceCulling(chunk, chunkCoord, x_solid_mask, y_solid_mask, z_solid_mask, meshData);

        chunk.state = CHUNK_STATE::MESHED; // mark chunk as meshed
    }



    if (!meshData.empty()) {
        meshData.shrink_to_fit(); 
        threadpool->enqueueMainTask([this, chunkCoord, meshData = std::move(meshData)]() mutable {
            uploadChunkMesh(chunkCoord, meshData);
        });
    }
}

void World::uploadChunkMesh(glm::ivec3 chunkCoord, std::vector<float>& meshData) {

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

void World::init(glm::vec3& playerPosition, Threadpool* threadpoolPtr) {
    
    // Configure the noise generator
    warpNoise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    warpNoise.SetDomainWarpAmp(25.0f); 
    warpNoise.SetFrequency(0.005f); 

    // The Base Noise
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    baseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    baseNoise.SetFractalOctaves(4);
    baseNoise.SetFrequency(0.003f); 

    // Position the player above the terrain at (0,0)
    playerPosition = glm::vec3(0.0f, (baseNoise.GetNoise(0.0f, 0.0f) + 1. * amplitude) + 3.0f, 0.0f);

    threadpool = threadpoolPtr;

    // Generate the initial terrain around the player
    generateChunks(playerPosition);       
}

void World::cleanup() {
    // Delete all OpenGL objects
    for (auto& pair : chunkVboMap) glDeleteBuffers(1, &pair.second);
    for (auto& pair : chunkVaoMap) glDeleteVertexArrays(1, &pair.second);
}