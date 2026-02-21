#pragma once

#include <shader/shader.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <core/constants.h>
#include <renderer/frustum.h>

// Forward Declarations
struct Player;
struct Camera;
class World;


class Renderer {
    public:
        // Shaders and Render Objects
        std::unique_ptr<Shader> chunkShader;       
        std::unique_ptr<Shader> selectedBlockShader; 

        // frustum culling stats
        int totalVisibleChunks = 0;
        int inFrustumChunks = 0;        

        // Render Functions
        void render(glm::ivec3 selectedBlock, Camera& camera, Player& player, World& world, GLFWwindow* window); 
        
        // Lifecycle
        void init(GLFWwindow* window);
        void cleanup();
        
        void initSelectedBlockObjects();  
        void initWorldObjects(GLuint chunkVAO, GLuint chunkVBO, glm::ivec3 chunkCoord, World& world);

    private:
        // Textures and Buffers
        GLuint textureAtlas;
        GLuint selectedBlockVao, selectedBlockVbo;   

        // Initialization Helpers
        void initShaders();
        void initTextures();
        void initImGui(GLFWwindow* window);       
        
        // ImGui Rendering
        void renderImGui(Player& player, World& world); // maybe move to Renderer class? 
};