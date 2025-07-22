#pragma once

#include <iostream>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include "rendering/VertexData.h"
#include "components/Camera.h"
#include "components/InputManager.h"
#include "components/Player.h"
#include "components/World.h"
#include "utils/BoundingBox.h"
#include "utils/ThreadPool.h"
#include <shader/shader.h>


struct ChunkMeshResult {
    glm::ivec3 chunkCoord;
    std::vector<float> meshData;
};


class Game {
public:
    Game();
    void init();
    void run();
    void cleanup();

private:
    // Game Components
    Player m_player;
    Camera m_camera;
    InputManager m_input;
    World m_world;

    // Window and Timing
    GLFWwindow* m_window;
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;
    bool m_chunkChange = false;

    // Shaders and Render Objects
    std::unique_ptr<Shader> m_chunkShader;       
    std::unique_ptr<Shader> m_selectedBlockShader; 
    std::unique_ptr<Shader> m_skyboxShader;
    GLuint m_skyboxVao, m_skyboxVbo;
    GLuint m_cubemapTexture;    
    GLuint m_textureAtlas;
    GLuint m_selectedBlockVao, m_selectedBlockVbo;

    // Raycasting State (as per your request)
    glm::ivec3 m_selectedBlock;
    glm::ivec3 m_previousBlock;
    int curBlockType;

    // Raycasting Configuration
    const float m_rayStart = 0.1f;
    const float m_rayEnd = 4.0f;
    const float m_rayStep = 0.1f;
    const std::vector<glm::vec3> m_rayStarts = {
        glm::vec3(0), glm::vec3(0.05f, 0, 0), glm::vec3(-0.05f, 0, 0),
        glm::vec3(0, 0.05f, 0), glm::vec3(0, -0.05f, 0), glm::vec3(0, 0, 0.05f),
        glm::vec3(0, 0, -0.05f)
    };

    //Camera 
    const float FOV = 45.0f;    

    // Collision constants
    const float m_collisionGap = 0.01f;
    const float m_collisionMargin = 0.5f;

    // thread pool 
    std::unique_ptr<ThreadPool> m_threadPool; 

    // Thread-safe queue for chunk mesh results
    std::queue<ChunkMeshResult> m_readyMeshes;
    std::mutex m_readyMeshesMutex; // Mutex to protect the queue

    //first load state
    std::atomic<int> m_initialChunksToLoad = 0;
    bool firstLoad = true;


    // Core Methods
    void processInput();
    void update();
    void render();
    void renderImGui();

    //debug prints
    void debug();


    // Logic Methods
    void updatePhysics();
    void performRaycasting();
    void generateTerrain();
    void calculateChunkAndNeighbors(glm::ivec3 block);
    std::vector<int> getVisibleFaces(glm::ivec3 block);

    void generateChunkData(glm::ivec3 chunkCoord);
    void createChunkMesh(glm::ivec3 chunkCoord);
    void uploadReadyMeshes();    

    // Collision
    bool boxBoxOverlap(const BoundingBox& playerBox, const BoundingBox& blockBox) const;
    glm::vec3 resolveYCollision(glm::vec3 nextPlayerPos, glm::vec3 yMovement);
    glm::vec3 resolveXZCollision(glm::vec3 nextPlayerPos, glm::vec3 xzMovement);

    // Initialization Helpers
    void initGlfw();
    void initGlad();
    void initImGui();
    void initShaders();
    void initTextures();
    void initRenderObjects();

    // Callback Methods
    void onMouseMovement(double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback_router(GLFWwindow* window, double xpos, double ypos);

    // Loading Screen
    void renderLoadingScreen();    

    // skybox Loading
    void loadCubemap(std::vector<std::string> faces);
};