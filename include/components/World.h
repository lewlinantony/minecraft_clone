#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <FastNoiseLite/FastNoiseLite.h> // noise  
#include "Chunk.h"
#include "Block.h"
#include "utils/GlmHash.h"
#include <glad/glad.h>


// WORLD GEN AND STORING
struct World {
    // Data
    std::unordered_map<glm::ivec3, Chunk> chunkMap;
    std::unordered_map<glm::ivec3, std::vector<float>> chunkMeshData;
    std::unordered_map<glm::ivec3, GLuint> chunkVboMap;
    std::unordered_map<glm::ivec3, GLuint> chunkVaoMap;

    // Configuration
    int Y_RENDER_DIST = 3;
    int XZ_RENDER_DIST = 10;
    int Y_LOAD_DIST = Y_RENDER_DIST+1;
    int XZ_LOAD_DIST = XZ_RENDER_DIST+1;

    // Noise Parameters
    FastNoiseLite noise;
    int   g_NoiseOctaves    = 4;
    float g_NoiseGain       = 0.3f;
    float g_NoiseLacunarity = 2.1f;
    float g_NoiseFrequency  = 0.01f;
    float amplitude         = 10.0f;
    int   g_NoiseSeed       = 133;

    // Methods
    Block* getBlock(glm::ivec3 blockPosition);
    void setBlock(glm::ivec3 blockPosition, int type);
    glm::ivec3 getChunkOrigin(glm::ivec3 blockPosition);
};