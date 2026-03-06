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
    int type = 0; // 0 for a
};


// CHUNK
struct Chunk {
    Block blocks[CHUNK_SIZE+2][CHUNK_SIZE+2][CHUNK_SIZE+2]; // +2 for easier face visibility checks without needing to worry about chunk boundaries
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
        int XZ_RENDER_DIST = 8;
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
        void calculateChunkAndNeighborsMesh(glm::ivec3 block);
        void calculateChunkMesh(glm::ivec3 chunkCoord);
        void uploadChunkMesh(glm::ivec3 chunkCoord, std::vector<float> meshData);        

        std::shared_mutex chunkMapMutex; // chunkMap shared mutex

        
    private:        
        // World Data
        std::unordered_map<glm::ivec3, Chunk> chunkMap;

        void populateChunkPadding(glm::ivec3 chunkCoord);        
        
        // Noise Parameters
        FastNoiseLite noise;
        int   g_NoiseOctaves    = 4;
        float g_NoiseGain       = 0.3f;
        float g_NoiseLacunarity = 2.1f;
        float g_NoiseFrequency  = 0.01f;
        float amplitude         = 10.0f;
        int   g_NoiseSeed       = 133;

        double processDuration = 10.0;

        // Helpers
        uint8_t getVisibleFaces(glm::ivec3 block);

        Threadpool* threadpool;
    };
    
    


