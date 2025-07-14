#include "rendering/VertexData.h"

// Define the size constant. This is the only place where sizeof(cubeVertices)
const size_t CUBE_VERTICES_SIZE = CUBE_VERTICES_SIZE;

float topFace[] = {
    // Top face (+Y) → faceID = 0
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
};

float frontFace[] = {
    // Front face (-Z) → faceID = 1
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
};

float rightFace[] = {
    // Right face (–X) → faceID = 2
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 2.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 2.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
};

float backFace[] = {
    // back face (+Z) → faceID = 3
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 3.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 3.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
};

float leftFace[] = {
    // Left face (+X) → faceID = 4
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
    0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 4.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 4.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,
};

float bottomFace[] = {
    // Bottom face (–Y) → faceID = 5
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 5.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 5.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 5.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 5.0f,
};

float* faceVertices[6] = {
    topFace,
    frontFace,
    rightFace,
    backFace,
    leftFace,
    bottomFace
};

float cubeVertices[] = {
    // Top face (+Y) → faceID = 0
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

    // Front face (-Z) → faceID = 1
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,

    // Right face (–X) → faceID = 2
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 2.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 2.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,

    // back face (+Z) → faceID = 3
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 3.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 3.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 3.0f,

    // Left face (+X) → faceID = 4
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
    0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 4.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 4.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,

    // Bottom face (–Y) → faceID = 5
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 5.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 5.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 5.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 5.0f,
};
