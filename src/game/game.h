#pragma once

#include <shader/shader.h> // never import before glfw
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <stb/stb_image.h>
#include <core/camera.h>
#include <core/input_manager.h>
#include <player/player.h>
#include <physics/collision.h>
#include <physics/physics.h>
#include <world/world.h>
#include <renderer/renderer.h>

//================================================================================
// The Main Game Class
//================================================================================

class Game {
public:
    void init();
    void run();
    void cleanup();

private:
    // Game Components
    Player m_player;
    Camera m_camera;
    InputManager m_input;
    World m_world;
    Collision m_collision;
    Physics m_physics;
    Renderer m_renderer;

    // Window and Timing
    GLFWwindow* m_window = nullptr;
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;
    bool m_playerMovedChunks = false; 

    // Raycasting State (as per your request)
    glm::ivec3 m_selectedBlock = glm::ivec3(INT_MAX);
    glm::ivec3 m_previousBlock = glm::ivec3(INT_MAX);
    int m_curBlockType = 1; // Default block type dirt

    // Raycasting Configuration
    const float m_rayStart = 0.1f;
    const float m_rayEnd = 4.0f;
    const float m_rayStep = 0.1f;
    const std::vector<glm::vec3> m_rayStarts = {
        glm::vec3(0), glm::vec3(0.05f, 0, 0), glm::vec3(-0.05f, 0, 0),
        glm::vec3(0, 0.05f, 0), glm::vec3(0, -0.05f, 0), glm::vec3(0, 0, 0.05f),
        glm::vec3(0, 0, -0.05f)
    };

    // Core Methods
    void processInput(); // maybe move to InputManager?
    void update();


    // Logic Methods
    void performRaycasting(); // idk move maybe?

    // Initialization Helpers
    void initGlfw();
    void initGlad();
    void initWorld(); 
    void initRenderer(); 

    // Callback Methods
    void onMouseMovement(double xpos, double ypos);
    static void mouse_callback_router(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};

