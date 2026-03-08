#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <unordered_map>
#include <vector>
#include <FastNoiseLite/FastNoiseLite.h>
#include <core/constants.h>
#include <core/utils.h>
#include <renderer/renderer.h>
#include <threadpool/threadpool.h>
#include <chrono>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>


// Forward declaration
class Player; 

// BLOCK
struct Block{
    int type = 0; // 0 for air
};

// CHUNK STATE
enum class CHUNK_STATE: u_int8_t{
    EMPTY       = 0,    // No block data, not generated
    GENERATED   = 1,    // Block data generated, no mesh
    MESHED      = 2,    // Mesh generated and uploaded to GPU
};

// CHUNK
struct Chunk {
    Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    CHUNK_STATE state = CHUNK_STATE::EMPTY; 
};


// WORLD GEN AND STORING
class World {
    public:    
    
        Renderer renderer;

        // Mesh Data
        std::unordered_map<glm::ivec3, int> chunkVertexCountMap;
        std::unordered_map<glm::ivec3, GLuint> chunkVboMap;
        std::unordered_map<glm::ivec3, GLuint> chunkVaoMap;   

        // Render and Load Distances
        int Y_LIMIT = 4; // Vertical world limit in chunks (total height in blocks = Y_LIMIT*CHUNK_SIZE)
        int XZ_RENDER_DIST = 25;
        int XZ_LOAD_DIST = XZ_RENDER_DIST+1;     
        
        // Lifecycle
        void init(glm::vec3& playerPosition, Threadpool* threadpoolPtr);
        void cleanup();
        
        // Accessors
        Block* getBlock(glm::ivec3 blockPosition);
        void setBlock(glm::ivec3 blockPosition, int type);
        glm::ivec3 getChunkOrigin(glm::ivec3 blockPosition);

        // Terrain Generation
        void generateChunks(glm::vec3 playerPosition);  
        void generateChunkData(glm::ivec3 chunkOrigin); 
        void updateChunkAndNeighboursMesh(glm::ivec3 block);
        void tryCalculateChunkMesh(glm::ivec3 chunkCoord); // only calculates mesh if chunk state is GENERATED, otherwise does nothing
        void calculateChunkMesh(glm::ivec3 chunkCoord);
        void uploadChunkMesh(glm::ivec3 chunkCoord, std::vector<float>& meshData);        

        std::shared_mutex chunkMapMutex; // chunkMap shared mutex

        
    private:        

        Threadpool* threadpool;

        // World Data
        std::unordered_map<glm::ivec3, Chunk> chunkMap;

        // Bitmasking helpers for Face Culling
        void populateChunkBitMask(Chunk& chunk, glm::ivec3 chunkCoord, u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2]);
        void populateChunkBitMaskPadding(Chunk& chunk, glm::ivec3 chunkCoord, u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2]);
        void bitMaskFaceCulling(Chunk& chunk, glm::ivec3 chunkCoord, u_int64_t x_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t y_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], u_int64_t z_solid_mask[CHUNK_SIZE+2][CHUNK_SIZE+2], std::vector<float>& meshData);

        // Neighbor chunk offsets 
        const glm::ivec3 neighbourChunks[6] = {
            glm::ivec3(CHUNK_SIZE, 0, 0),    // Right
            glm::ivec3(-CHUNK_SIZE, 0, 0),   // Left
            glm::ivec3(0, CHUNK_SIZE, 0),    // Top
            glm::ivec3(0, -CHUNK_SIZE, 0),    // Bottom
            glm::ivec3(0, 0, CHUNK_SIZE),    // Back
            glm::ivec3(0, 0, -CHUNK_SIZE)   // Front
        };        

        // normals for each face direction 
        const glm::vec3 normals[6] = {
            glm::vec3(1.0f, 0.0f, 0.0f),    // Right (+X)
            glm::vec3(-1.0f, 0.0f, 0.0f),   // Left (-X)
            glm::vec3(0.0f, 1.0f, 0.0f),    // Top (+Y)
            glm::vec3(0.0f, -1.0f, 0.0f),   // Bottom (-Y)
            glm::vec3(0.0f, 0.0f, 1.0f),    // Back (+Z)
            glm::vec3(0.0f, 0.0f, -1.0f)    // Front (-Z)
        };            


        // Noise Parameters
        FastNoiseLite noise;
        int   g_NoiseOctaves    = 4;
        float g_NoiseGain       = 0.3f;
        float g_NoiseLacunarity = 2.1f;
        float g_NoiseFrequency  = 0.01f;
        float amplitude         = 10.0f;
        int   g_NoiseSeed       = 133;        
    };
    
    


