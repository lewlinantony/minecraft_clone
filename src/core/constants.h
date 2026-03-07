#pragma once


// Constants
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
constexpr float BLOCK_SIZE = 1.0f; 
constexpr int CHUNK_SIZE = 32;

// face coords
inline constexpr float rightFace[] = {
    // Right face (+X) → faceID = 0
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
};

inline constexpr float leftFace[] = {
    // Left face (–X) → faceID = 1
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,
};

inline constexpr float topFace[] = {
    // Top face (+Y) → faceID = 2
      0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
     -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 2.0f,
     -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
      0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
      0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 2.0f,
     -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
};
    
inline constexpr float bottomFace[] = {
    // Bottom face (–Y) → faceID = 3
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 3.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 3.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 3.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 3.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
};

inline constexpr float backFace[] = {
    // back face (+Z) → faceID = 4
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 4.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 4.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 4.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 4.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,
};

inline constexpr float frontFace[] = {
    // Front face (-Z) → faceID = 5
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 5.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 5.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 5.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 5.0f,
};




inline constexpr const float* faceVertices[6] = { // since we used constexpr for faces, the type of the pointer becomes const float*
    rightFace,      // faceID = 0
    leftFace,       // faceID = 1
    topFace,        // faceID = 2
    bottomFace,     // faceID = 3
    backFace,       // faceID = 4
    frontFace       // faceID = 5
};

inline constexpr float cubeVertices[] = {
    // Right face (+X) → faceID = 4
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

    // Left face (–X) → faceID = 2
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,    

    // Top face (+Y) → faceID = 0
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 2.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 2.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,

    // Bottom face (–Y) → faceID = 5
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 3.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 3.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 3.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 3.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 3.0f,    

    // Front face (-Z) → faceID = 1
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 4.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 4.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 4.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 4.0f,

    // back face (+Z) → faceID = 3
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 5.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 5.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 5.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 5.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 5.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 5.0f,

};
