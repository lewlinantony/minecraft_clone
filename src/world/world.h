#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <unordered_map>
#include <vector>
#include <core/constants.h>
#include <FastNoiseLite/FastNoiseLite.h>


// Bounding box
struct BoundingBox {
    glm::vec3 max;
    glm::vec3 min;

    static BoundingBox box(glm::vec3 position, float width, float height, float depth) {
        BoundingBox b;
        glm::vec3 halfSize = glm::vec3(width / 2.0f, height / 2.0f, depth / 2.0f);
        b.min = position - halfSize;
        b.max = position + halfSize;
        return b;
    }
};

// Hash function for glm::ivec3
namespace std {
    template<>
    struct hash<glm::ivec3> {
        std::size_t operator()(const glm::ivec3& v) const noexcept {
            std::size_t hx = std::hash<int>()(v.x);
            std::size_t hy = std::hash<int>()(v.y);
            std::size_t hz = std::hash<int>()(v.z);
            return hx ^ (hy << 1) ^ (hz << 2);
        }
    };
}


// BLOCK
struct Block{
    int type = 0; // 0 for air
};


// CHUNK
struct Chunk {
    Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};


// WORLD GEN AND STORING
class World {
    public:
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


