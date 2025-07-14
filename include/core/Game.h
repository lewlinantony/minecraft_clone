#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "components/Player.h"
#include "components/Camera.h"
#include "components/InputManager.h"
#include "components/World.h"
#include "shader/shader.h"
#include "utils/BoundingBox.h"




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
    GLuint m_textureAtlas;
    GLuint m_selectedBlockVao, m_selectedBlockVbo;

    // Raycasting State
    glm::ivec3 m_selectedBlock;
    glm::ivec3 m_previousBlock;
    int m_curBlockType;

    // Raycasting Configuration
    const float m_rayStart = 0.1f;
    const float m_rayEnd = 4.0f;
    const float m_rayStep = 0.1f;
    const std::vector<glm::vec3> m_rayStarts;

    // Collision constants
    const float m_collisionGap = 0.01f;
    const float m_collisionMargin = 0.5f;

    // Core Methods
    void processInput();
    void update();
    void render();
    void renderImGui();

    // Logic Methods
    void updatePhysics();
    void performRaycasting();
    void generateTerrain();
    void calculateChunk(glm::ivec3 chunkCoord);
    void calculateChunkAndNeighbors(glm::ivec3 block);
    std::vector<int> getVisibleFaces(glm::ivec3 block);

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
};