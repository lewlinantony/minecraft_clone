#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <unordered_map>
#include <functional> // For std::hash
#include <FastNoiseLite/FastNoiseLite.h> // noise  
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <cmath>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float BLOCK_SIZE = 1.0f; 
const float COLLISION_THRESHOLD = 0.2f;

// Physics constants
const float gravity = 30.0f;
const float jumpVelocity = 9.0f;
const float cameraSpeed = 10.0f;
const float mouseSensitivity = 0.1f;

// Player dimensions
const float playerWidth = 0.6f;
const float playerDepth = 0.6f;
const float playerHeight = 1.8f;
const float eyeHeight = 1.6f;

// Collision constants
const float gap = 0.01f; // Gap to avoid collision issues
const float margin = 0.5f; // a margin to increase the number of blocks that are checked for collision

// Player state
float velocity = 0.0f;
glm::vec3 playerPosition = glm::vec3(0.0f);
bool onGround = false;
bool CreativeMode = false; 

// Camera state
float yaw = -90.0f;
float pitch = 0.0f;
const float MAX_PITCH = 89.0f;
glm::vec3 cameraPos = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight;

// Mouse state
bool firstMouse = true;
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;

// Input state
bool spaceWasPressed = false;
bool spaceIsPressed = false;
bool tabWasPressed = false;
bool tabIsPressed = false;
bool mouseLeftIsPressed = false;
bool mouseLeftWasPressed = false;
bool mouseRightIsPressed = false;
bool mouseRightWasPressed = false;

// Movement scaling
float MOVE_SCALE = 0.1f;

// Frame timing
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// Uniforms
int blockType;

// Player bounding box
struct BoundingBox {
    glm::vec3 max;
    glm::vec3 min;
};
BoundingBox nextPlayerBox;

// Raycasting constants
const float rayStart = 0.1f;
const float rayEnd = 4.0f;
const float rayStep = 0.1f;
glm::ivec3 selectedBlock; // Currently selected block 
glm::ivec3 previousBlock; // Previously selected block
std::vector<glm::vec3> rayStarts = { //effectively to thicken the ray so it dosent pass through block edges
    glm::vec3(0), // center
    glm::vec3(0.05f, 0, 0),
    glm::vec3(-0.05f, 0, 0),
    glm::vec3(0, 0.05f, 0),
    glm::vec3(0, -0.05f, 0),
    glm::vec3(0, 0, 0.05f),
    glm::vec3(0, 0, -0.05f)
};   

//Buffer Objects
unsigned int selectedBlockVBO, selectedBlockVAO;


//noise values
int   g_NoiseOctaves    = 4;
float g_NoiseGain       = 0.3f;  // Same as persistence
float g_NoiseLacunarity = 2.1f;
float g_NoiseFrequency  = 0.01f;
float amplitude         = 10.0f;
int   g_NoiseSeed       = 133;


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

const int CHUNK_SIZE = 8;
int RENDER_DIST = 2; // This gives you 3x3x3 chunks centered at 0
bool chunkChange = false;

struct BlockInfo{
    int type;
    glm::ivec3 chunkCoord;
};

struct Block{
    int type;
    glm::ivec3 coord;
};

struct Chunk {
    Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};


// Chunk map
std::unordered_map<glm::ivec3, Chunk> chunkMap;

// chunk data
std::unordered_map<glm::ivec3, std::vector<float>> chunks;

std::unordered_map<glm::ivec3, GLuint> chunkVBOMap;
std::unordered_map<glm::ivec3, GLuint> chunkVAOMap;

// face coords
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
    topFace,     // faceID = 0
    frontFace,   // faceID = 1
    rightFace,   // faceID = 2
    backFace,    // faceID = 3
    leftFace,    // faceID = 4
    bottomFace   // faceID = 5
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


// Function declarations
bool boxBoxOverlap(BoundingBox playerBox, BoundingBox blockBox);
bool pointBoxOverlap(glm::vec3 point, BoundingBox box);
glm::vec3 resolveYCollision(glm::vec3& p_nextPlayerPosition, glm::vec3 y_movement);
glm::vec3 resolveXZCollision(glm::vec3 p_nextPlayerPosition, glm::vec3 p_velocity);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void renderImGui();
void rayCast(glm::vec3 cameraFront);
std::vector<int> validFaces(glm::ivec3 block);
void initializeSelectedBlockBuffer();
void calculateChunk(glm::ivec3 block);
void generateTerrain(glm::vec3 playerPosition);
void calculateChunkAndNeighbors(glm::ivec3 block);
glm::ivec3 getChunkOrigin(glm::ivec3 blockPosition);
glm::ivec3 getBlockLocalPosition(glm::ivec3 blockPosition, glm::ivec3 chunkOrigin);
Block* getBlock(glm::ivec3 blockPosition);




glm::ivec3 getBlockLocalPosition(glm::ivec3 blockPosition, glm::ivec3 chunkOrigin) {
    return blockPosition - chunkOrigin;
}

Block* getBlock(glm::ivec3 blockPosition) {
    glm::ivec3 chunkCoord = getChunkOrigin(blockPosition);

    // Check if the chunk exists
    auto it = chunkMap.find(chunkCoord);
    if (it == chunkMap.end()) {
        return nullptr; // Chunk not found
    }

    // Chunk exists, now get the block's local position
    glm::ivec3 localPos = getBlockLocalPosition(blockPosition, chunkCoord);

    return &it->second.blocks[localPos.x][localPos.y][localPos.z];
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if(firstMouse){
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > MAX_PITCH){
        pitch = MAX_PITCH;
    }
    if(pitch < -MAX_PITCH){
        pitch = -MAX_PITCH;
    }
        
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(direction);
}

void processInput(GLFWwindow* window){

    float speed = cameraSpeed * deltaTime;

    //close window
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }

    //camera controls    
    glm::vec3 front = cameraFront;
    front.y = 0.0f;
    front = glm::normalize(front);
    glm::vec3 right = glm::normalize(glm::cross(front, cameraUp));

    //nerf movement in air
    if (onGround){
        MOVE_SCALE = 1.0f;
    }
    else{
        MOVE_SCALE = 0.5f;
    }
    
    // toggle mouse lock
    tabIsPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabIsPressed && !tabWasPressed) {
        int mode = glfwGetInputMode(window, GLFW_CURSOR);
        if (mode == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true; // reset mouse for next lock
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // reset mouse for next lock
        }
    }
    tabWasPressed = tabIsPressed;

    // WSAD movement
    glm::vec3 nextPlayerPosition = playerPosition;
    glm::vec3 xz_movement = glm::vec3(0.0f); // Store intended movement
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        xz_movement += front * speed * MOVE_SCALE;
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        xz_movement -= front * speed * MOVE_SCALE;
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        xz_movement += right * speed * MOVE_SCALE;
    }    
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        xz_movement -= right * speed * MOVE_SCALE;
    }     

    nextPlayerPosition = playerPosition + xz_movement;
    if(!CreativeMode){
        auto nextpos = resolveXZCollision(nextPlayerPosition, xz_movement);
        if (getChunkOrigin(nextpos)!=getChunkOrigin(playerPosition)){
            chunkChange = true;
        }
        playerPosition = nextpos;
    }
    else{
        playerPosition = nextPlayerPosition;
    }

    if(!CreativeMode){
        // jump
        spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if( spaceIsPressed && !spaceWasPressed && onGround){
            velocity += jumpVelocity;
            onGround = false;
        }
        spaceWasPressed = spaceIsPressed;        
    }
    else{
        //creative mode movement
        glm::vec3 nextPlayerPosition = playerPosition;
        glm::vec3 y_movement = glm::vec3(0.0f); 
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            y_movement.y += speed;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            y_movement.y -= speed;
        }
        nextPlayerPosition = playerPosition + y_movement;
        playerPosition = nextPlayerPosition;      
    }


    // block removal
    mouseLeftIsPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if ( mouseLeftIsPressed && !mouseLeftWasPressed && selectedBlock != glm::ivec3(INT_MAX, INT_MAX, INT_MAX) ) {
        Block* block_to_remove = getBlock(selectedBlock);
        if (block_to_remove != nullptr && block_to_remove->type != 0) {
            block_to_remove->type = 0; // Set to air
            calculateChunkAndNeighbors(selectedBlock);
        }
    }
    mouseLeftWasPressed = mouseLeftIsPressed;

    // block placement
    mouseRightIsPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if ( mouseRightIsPressed && !mouseRightWasPressed && previousBlock != glm::ivec3(INT_MAX, INT_MAX, INT_MAX) && selectedBlock != glm::ivec3(INT_MAX, INT_MAX, INT_MAX) ) {
        glm::ivec3 chunkCoord = getChunkOrigin(previousBlock);
        glm::ivec3 localPos = getBlockLocalPosition(previousBlock, chunkCoord);
        
        chunkMap[chunkCoord].blocks[localPos.x][localPos.y][localPos.z].type = 1; // Or whatever type you want to place
        calculateChunkAndNeighbors(previousBlock);
    }
    mouseRightWasPressed = mouseRightIsPressed;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
} 

void renderImGui() {
    // ImGui setup
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Player Info Window
    ImGui::Begin("Player Info");
    ImGui::Text("Position: %.2f, %.2f, %.2f", playerPosition.x, playerPosition.y, playerPosition.z);
    ImGui::Text("On Ground: %s", onGround ? "Yes" : "No");
    ImGui::Text("Velocity: %.2f", velocity);
    
    
    // Creative Mode Toggle
    ImGui::Checkbox("Creative Mode", &CreativeMode);
    
    // FPS Meter
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool boxBoxOverlap(BoundingBox playerBox, BoundingBox blockBox){
    return ((playerBox.min.x <= blockBox.max.x && playerBox.max.x >= blockBox.min.x) and
            (playerBox.min.y <= blockBox.max.y && playerBox.max.y >= blockBox.min.y) and
            (playerBox.min.z <= blockBox.max.z && playerBox.max.z >= blockBox.min.z));    
}

glm::vec3 resolveYCollision(glm::vec3& p_nextPlayerPosition, glm::vec3 y_movement) {

    nextPlayerBox.max.x = p_nextPlayerPosition.x + (playerWidth/2);
    nextPlayerBox.max.y = p_nextPlayerPosition.y + playerHeight;
    nextPlayerBox.max.z = p_nextPlayerPosition.z + (playerDepth/2);

    nextPlayerBox.min.x = p_nextPlayerPosition.x - (playerWidth/2);
    nextPlayerBox.min.y = p_nextPlayerPosition.y;
    nextPlayerBox.min.z = p_nextPlayerPosition.z - (playerDepth/2);


    int max_x = (int)glm::ceil(nextPlayerBox.max.x + margin);
    int max_y = (int)glm::ceil(nextPlayerBox.max.y + margin);
    int max_z = (int)glm::ceil(nextPlayerBox.max.z + margin);

    int min_x = (int)glm::floor(nextPlayerBox.min.x - margin);
    int min_y = (int)glm::floor(nextPlayerBox.min.y - margin);
    int min_z = (int)glm::floor(nextPlayerBox.min.z - margin);

    onGround = false; 

    for(int x = min_x; x< max_x; x++){
        for(int y = min_y; y< max_y; y++){
            for(int z = min_z; z< max_z; z++){
                Block* potential_block = getBlock(glm::ivec3(x, y, z));
                if (potential_block != nullptr && potential_block->type != 0) {
                    BoundingBox blockBox;
                    blockBox.max = glm::vec3(x + BLOCK_SIZE/2, y + BLOCK_SIZE/2 + gap, z + BLOCK_SIZE/2); 
                    blockBox.min = glm::vec3(x - BLOCK_SIZE/2, y - BLOCK_SIZE/2 - gap, z - BLOCK_SIZE/2);
                    if (boxBoxOverlap(nextPlayerBox, blockBox)){
                        velocity = 0.0f;
                        if (y_movement.y > 0) {  // Moving Down
                            p_nextPlayerPosition.y = blockBox.min.y - playerHeight;
                        }
                        else {  // Moving Up
                            p_nextPlayerPosition.y = blockBox.max.y; 
                            onGround = true;
                        }
                        break;
                    }                        
                }
            }
        }
    }
    
    return p_nextPlayerPosition;
}

glm::vec3 resolveXZCollision(glm::vec3 p_nextPlayerPosition, glm::vec3 p_velocity) {
    // Resolve X-axis collision
    if (p_velocity.x != 0) {
        nextPlayerBox.max.x = p_nextPlayerPosition.x + (playerWidth / 2);
        nextPlayerBox.min.x = p_nextPlayerPosition.x - (playerWidth / 2);
        nextPlayerBox.max.y = playerPosition.y + playerHeight;
        nextPlayerBox.min.y = playerPosition.y;
        nextPlayerBox.max.z = playerPosition.z + (playerDepth / 2);
        nextPlayerBox.min.z = playerPosition.z - (playerDepth / 2);

        int max_x = (int)glm::ceil(nextPlayerBox.max.x + margin);
        int max_y = (int)glm::ceil(nextPlayerBox.max.y + margin);
        int max_z = (int)glm::ceil(nextPlayerBox.max.z + margin);
        int min_x = (int)glm::floor(nextPlayerBox.min.x - margin);
        int min_y = (int)glm::floor(nextPlayerBox.min.y - margin);
        int min_z = (int)glm::floor(nextPlayerBox.min.z - margin);

        for (int x = min_x; x < max_x; x++) {
            for (int y = min_y; y < max_y; y++) {
                for (int z = min_z; z < max_z; z++) {
                    Block* potential_block = getBlock(glm::ivec3(x, y, z));
                    if (potential_block != nullptr && potential_block->type != 0) {                    
                        BoundingBox blockBox;
                        blockBox.max = glm::vec3(x + BLOCK_SIZE / 2 + gap, y + BLOCK_SIZE / 2, z + BLOCK_SIZE / 2);
                        blockBox.min = glm::vec3(x - BLOCK_SIZE / 2 - gap, y - BLOCK_SIZE / 2, z - BLOCK_SIZE / 2);
                        if (boxBoxOverlap(nextPlayerBox, blockBox)) {
                            if (p_velocity.x > 0) { // Moving right
                                p_nextPlayerPosition.x = blockBox.min.x - playerWidth / 2;
                            } else { // Moving left
                                p_nextPlayerPosition.x = blockBox.max.x + playerWidth / 2;
                            }
                            goto resolve_z; // Move to Z resolution, cause at a time we can only have one collisoin along an axis, which is the closest to the player same for z
                        }
                    }
                }
            }
        }
    }

resolve_z:
    // Resolve Z-axis collision
    if (p_velocity.z != 0) {
        nextPlayerBox.max.x = p_nextPlayerPosition.x + (playerWidth / 2);
        nextPlayerBox.min.x = p_nextPlayerPosition.x - (playerWidth / 2);
        nextPlayerBox.max.y = playerPosition.y + playerHeight;
        nextPlayerBox.min.y = playerPosition.y;
        nextPlayerBox.max.z = p_nextPlayerPosition.z + (playerDepth / 2);
        nextPlayerBox.min.z = p_nextPlayerPosition.z - (playerDepth / 2);

        int max_x = (int)glm::ceil(nextPlayerBox.max.x + margin);
        int max_y = (int)glm::ceil(nextPlayerBox.max.y + margin);
        int max_z = (int)glm::ceil(nextPlayerBox.max.z + margin);
        int min_x = (int)glm::floor(nextPlayerBox.min.x - margin);
        int min_y = (int)glm::floor(nextPlayerBox.min.y - margin);
        int min_z = (int)glm::floor(nextPlayerBox.min.z - margin);

        for (int x = min_x; x < max_x; x++) {
            for (int y = min_y; y < max_y; y++) {
                for (int z = min_z; z < max_z; z++) {
                    Block* potential_block = getBlock(glm::ivec3(x, y, z));
                    if (potential_block != nullptr && potential_block->type != 0) {                    
                        BoundingBox blockBox;
                        blockBox.max = glm::vec3(x + BLOCK_SIZE / 2, y + BLOCK_SIZE / 2, z + BLOCK_SIZE / 2 + gap);
                        blockBox.min = glm::vec3(x - BLOCK_SIZE / 2, y - BLOCK_SIZE / 2, z - BLOCK_SIZE / 2 - gap);
                        if (boxBoxOverlap(nextPlayerBox, blockBox)) {
                           if (p_velocity.z > 0) { // Moving forward
                                p_nextPlayerPosition.z = blockBox.min.z - playerDepth / 2;
                            } else { // Moving backward
                                p_nextPlayerPosition.z = blockBox.max.z + playerDepth / 2;
                            }
                            goto end_resolve; // End resolution
                        }
                    }
                }
            }
        }
    }

end_resolve:
    return p_nextPlayerPosition;
}

void rayCast(glm::vec3 cameraFront){
        // RayCasting to select blocks
        glm::vec3 rayDirection = glm::normalize(cameraFront); // unit vector in the direction of the camera
        glm::vec3 rayOrigin = cameraPos;
     
        float closestHit = rayEnd;
        glm::ivec3 bestHit = glm::ivec3(INT_MAX);
        glm::ivec3 bestPrev = glm::ivec3(INT_MAX);

        
        for (const auto& offset : rayStarts) {
            glm::vec3 curRayOrigin = rayOrigin + offset; // Adjust ray start based on offset
            glm::ivec3 prevBlockThisRay = glm::ivec3(INT_MAX); // Reset previous block to an unlikely value
            for (float i = rayStart; i < rayEnd; i += rayStep) {
                glm::vec3 point = curRayOrigin + rayDirection * i; 
                glm::ivec3 blockPosition = glm::ivec3(glm::round(point));
                Block* hitBlock = getBlock(blockPosition);
                if (hitBlock != nullptr && hitBlock->type != 0) {
                    if (i < closestHit){
                        closestHit = i; 
                        bestHit = blockPosition;

                        BoundingBox prevBlockBox;
                        prevBlockBox.min = glm::vec3(prevBlockThisRay) - glm::vec3(0.5f);
                        prevBlockBox.max = glm::vec3(prevBlockThisRay) + glm::vec3(0.5f);                        

                        BoundingBox playerBox;
                        playerBox.min = playerPosition + glm::vec3(-playerWidth / 2.0f, 0.0f, -playerDepth / 2.0f);
                        playerBox.max = playerPosition + glm::vec3(playerWidth / 2.0f, playerHeight, playerDepth / 2.0f);                        

                        //cannot be previous block(block to be placed) if block overlaps with players bounding box
                        if (!boxBoxOverlap(playerBox, prevBlockBox)) {
                            bestPrev = prevBlockThisRay; // Store the previous block position
                        }
                        else {
                            bestPrev = glm::ivec3(INT_MAX); 
                        }

                        // Stop when we hit a block                        
                        break; 
                    }
                }
                prevBlockThisRay = blockPosition;            
            }
        }
        selectedBlock = bestHit;
        previousBlock = bestPrev;
        // The closest Block could still not be face alligned, So check its manhatten distance to the previous block
        glm::ivec3 delta = selectedBlock - previousBlock;

        // check if manhattan distance is 1 to ensure block is face alligned
        if (abs(delta.x) + abs(delta.y) + abs(delta.z) != 1) {
            previousBlock = glm::ivec3(INT_MAX); // Invalid for placement
        }        
}

std::vector<int> validFaces(glm::ivec3 block) {
    std::vector<int> visibleFaces;
    glm::ivec3 directions[6] = {
        glm::ivec3(0, 1, 0),    // Top face
        glm::ivec3(0, 0, -1),   // Front face
        glm::ivec3(-1, 0, 0),   // Right face
        glm::ivec3(0, 0, 1),    // Back face
        glm::ivec3(1, 0, 0),    // Left face
        glm::ivec3(0, -1, 0)    // Bottom face
    };    
    for (int i = 0; i < 6; ++i) {
        glm::ivec3 neighbor = block + directions[i];
        Block* neighborBlock = getBlock(neighbor);
        if (neighborBlock == nullptr || neighborBlock->type == 0) { // If chunk doesn't exist or block is air
            visibleFaces.push_back(i);
        }
    }
    return visibleFaces;
}

glm::ivec3 getChunkOrigin(glm::ivec3 blockPosition) {
    return glm::ivec3(
        floor(blockPosition.x / (float)CHUNK_SIZE) * CHUNK_SIZE,
        floor(blockPosition.y / (float)CHUNK_SIZE) * CHUNK_SIZE,
        floor(blockPosition.z / (float)CHUNK_SIZE) * CHUNK_SIZE
    );
}

void calculateChunk(glm::ivec3 block){
    glm::ivec3 chunkCoord = getChunkOrigin(block);

    float size = chunks[chunkCoord].size() / 7 ;
    chunks[chunkCoord].clear();
    Chunk& chunk = chunkMap[chunkCoord];

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (chunk.blocks[x][y][z].type == 0) continue; // Skip air blocks                
                glm::ivec3 blockPosition = chunkCoord + glm::ivec3(x, y, z);                         
                for (int faceID : validFaces(blockPosition)) {
                    const float* curFace = faceVertices[faceID];
                    if (!curFace) continue;

                    for (int i = 0; i < 6; ++i) { // 6 vertices per face
                        int idx = i * 6;

                        float vx = curFace[idx + 0] + blockPosition.x;
                        float vy = curFace[idx + 1] + blockPosition.y;
                        float vz = curFace[idx + 2] + blockPosition.z;

                        float ux = curFace[idx + 3];
                        float uy = curFace[idx + 4];
                        float fid = curFace[idx + 5];

                        float blockType = chunk.blocks[x][y][z].type;

                        chunks[chunkCoord].push_back(vx);
                        chunks[chunkCoord].push_back(vy);
                        chunks[chunkCoord].push_back(vz);
                        chunks[chunkCoord].push_back(ux);
                        chunks[chunkCoord].push_back(uy);
                        chunks[chunkCoord].push_back(fid); // faceID stored as float
                        chunks[chunkCoord].push_back(blockType);             
                    }   
                }
            }
        }
    }    
    
    GLuint chunkVAO, chunkVBO;

    // Check if the chunk already has a VAO/VBO.
    if (chunkVAOMap.find(chunkCoord) == chunkVAOMap.end()) {
        // If not, this is a new chunk that just gained its first visible face.
        // Create and configure a new VAO and VBO for it.
        glGenVertexArrays(1, &chunkVAO);
        glGenBuffers(1, &chunkVBO);

        chunkVAOMap[chunkCoord] = chunkVAO;
        chunkVBOMap[chunkCoord] = chunkVBO;

        glBindVertexArray(chunkVAO);
        glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1); // UV Coords
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2); // Face ID
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3); // Block Type
    } else {
        // The chunk already exists, so just get its VBO handle for updating.
        chunkVBO = chunkVBOMap[chunkCoord];
        glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
    }
    glBufferData(GL_ARRAY_BUFFER, chunks[chunkCoord].size() * sizeof(float), chunks[chunkCoord].data(), GL_DYNAMIC_DRAW);    
}

void calculateChunkAndNeighbors(glm::ivec3 block) {
    glm::ivec3 chunkCoord = getChunkOrigin(block);
    glm::ivec3 blockOffset = block - chunkCoord;
    calculateChunk(block); // always recalc current


    if (blockOffset.x == 0){
        calculateChunk(block + glm::ivec3(-1, 0, 0));
    }
    else if (blockOffset.x == CHUNK_SIZE - 1){
        calculateChunk(block + glm::ivec3(1, 0, 0));
    }
    if (blockOffset.y == 0){
        calculateChunk(block + glm::ivec3(0, -1, 0));
    }
    else if (blockOffset.y == CHUNK_SIZE - 1){
        calculateChunk(block + glm::ivec3(0, 1, 0));
    }
    if (blockOffset.z == 0){
        calculateChunk(block + glm::ivec3(0, 0, -1));
    }
    else if (blockOffset.z == CHUNK_SIZE - 1){
        calculateChunk(block + glm::ivec3(0, 0, 1));
    }
}

void initializeSelectedBlockBuffer(){
    glGenVertexArrays(1, &selectedBlockVAO);
    glGenBuffers(1, &selectedBlockVBO);

    glBindVertexArray(selectedBlockVAO);

    glBindBuffer(GL_ARRAY_BUFFER, selectedBlockVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);        

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Face ID attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2); //blocktype is passed as a uniform variable
}

void generateTerrain(glm::vec3 currentPlayerPosition){
    chunkMap.clear();

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(g_NoiseSeed);
    noise.SetFractalOctaves(g_NoiseOctaves);
    noise.SetFractalGain(g_NoiseGain);
    noise.SetFractalLacunarity(g_NoiseLacunarity);
    noise.SetFrequency(g_NoiseFrequency);



    for (int cx = -RENDER_DIST; cx <= RENDER_DIST; cx++) {
        for (int cy = -RENDER_DIST; cy <= RENDER_DIST; cy++) {
            for (int cz = -RENDER_DIST; cz <= RENDER_DIST; cz++) {
                
                glm::ivec3 chunkOrigin = getChunkOrigin(glm::round(currentPlayerPosition)) + glm::ivec3(cx, cy, cz) * CHUNK_SIZE;   
                Chunk& currentChunk = chunkMap[chunkOrigin];

                for (int x = 0; x < CHUNK_SIZE; x++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        
                        // Calculate global coordinates for noise function
                        float globalX = (float)(chunkOrigin.x + x);
                        float globalZ = (float)(chunkOrigin.z + z);

                        float height = noise.GetNoise(globalX, globalZ);
                        height = (height + 1.0f) * amplitude;
                        height = glm::round(height);

                        for (int y = 0; y < CHUNK_SIZE; y++) {
                            // global Y position
                            int globalY = chunkOrigin.y + y;

                            if (globalY > height) {
                                continue; 
                            } else if (globalY == (int)height) {
                                currentChunk.blocks[x][y][z].type = 1; // Grass
                            } else if (globalY >= height - 5) {
                                currentChunk.blocks[x][y][z].type = 2; // Dirt
                            } else {
                                currentChunk.blocks[x][y][z].type = 3; // Stone
                            }
                        }
                    }
                }

                if (chunks.find(chunkOrigin) == chunks.end()){
                    calculateChunk(chunkOrigin);
                }
            }
        }
    }

    // int radius = TERRAIN_SIZE / 2;
    // glm::ivec3 center = glm::ivec3(0, 0, 0); // or playerPos or wherever
    
    // for (int x = -radius; x <= radius; ++x) {
    //     for (int z = -radius; z <= radius; ++z) {
    //         if (x*x + z*z <= radius*radius) { // x^2 + z^2 <= r^2 for a circle
    //             float height = noise.GetNoise((float)x, (float)z); 
    //             height = glm::round(height);
    //             glm::vec3 blockPosition = glm::vec3((float)z, height, (float)x);               
    //             glm::ivec3 key = center + glm::ivec3(blockPosition); // Center the terrain around the origin
    //             blockMap[key] = true; // dont forget to change to chunkMap
    //         }
    //     }
    // }
}


int main(){
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Create a window
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Triangle", NULL, NULL);
    if(window == NULL){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }  

    // set framebuffer size callback function
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // set mouse callback function
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core"); 

    Shader chunkShader("shaders/world/shader.vert", "shaders/world/shader.frag");         
    
    // Create and compile shaders
    generateTerrain(playerPosition);

    //Chunk Shader
    // for (auto const& [chunkCoord, chunk] : chunkMap) {
    //     calculateChunk(chunkCoord);
    // }   
        
    // Selected Block shader
    Shader selectedBlockShader("shaders/selectedBlock/shader.vert", "shaders/selectedBlock/shader.frag");        
    initializeSelectedBlockBuffer();

    
    // Set drawing mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //Load the texture
    unsigned int textureAtlas;

    glGenTextures(1, &textureAtlas);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // x axis	set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // y axis
     
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // load image, create texture and generate mipmaps
    int widthImg, heightImg, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data1 = stbi_load("assets/img/minecraft_texture_map.png", &widthImg, &heightImg, &nrChannels, 0);

    GLenum format;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;
    
    if (data1) {
        std::cout<<"yes texture"<<std::endl;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, widthImg, heightImg, 0, format, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else{
        std::cout<<"no texture";
    }
    
    stbi_image_free(data1);


    // Bind texture to the shaders
    chunkShader.use();
    chunkShader.setInt("text", 0);

    selectedBlockShader.use();
    selectedBlockShader.setInt("text", 0);

    // Main render loop
    while(!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Input processing
        processInput(window);

        // plater state update
        if(!CreativeMode){
            if(!onGround) {
                velocity -= gravity * deltaTime;
                if (velocity < -50.0f) { // Clip the lower value of velocity
                    velocity = -50.0f;
                }
            }
            glm::vec3 y_movement = glm::vec3(0.0f, velocity * deltaTime, 0.0f); // vertical movement
            glm::vec3 nextPlayerPosition = playerPosition + y_movement;
            if (!CreativeMode){
                auto nextpos = resolveYCollision(nextPlayerPosition, y_movement);
                if (getChunkOrigin(nextpos)!=getChunkOrigin(playerPosition)){
                    chunkChange = true;
                }
                playerPosition = nextpos;      
            }
            else{
                playerPosition = nextPlayerPosition;
            }
        }

        if (chunkChange){
            generateTerrain(playerPosition);
            chunkChange = false;
        }

        // Update camera position
        cameraPos = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
        
        rayCast(cameraFront);

        // Clear the screen
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader
        chunkShader.use();

        // Update viewport size
        int curWidth, curHeight;
        glfwGetFramebufferSize(window, &curWidth, &curHeight);        

        // Set uniforms
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);   
        
        glm::mat4 model = glm::mat4(1.0f);
        chunkShader.setMat4("model", model);        
    
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // in vectors, dir = target - pos | target = pos + dir
        chunkShader.setMat4("view", view);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)curWidth/(float)curHeight, 0.1f, 5000.0f);
        chunkShader.setMat4("projection", projection);

        for(const auto &chunk: chunks){
            if (chunkMap.find(chunk.first) != chunkMap.end()){
                glBindVertexArray(chunkVAOMap[chunk.first]);
                glDrawArrays(GL_TRIANGLES, 0, chunk.second.size() / 7);
            }
        }
        
        
        // highlight the selected block with seperate shader
        if (selectedBlock != glm::ivec3(INT_MAX) && !CreativeMode){
            selectedBlockShader.use();
            glm::ivec3 chunkCoord = getChunkOrigin(selectedBlock);
            glm::ivec3 localPos = getBlockLocalPosition(selectedBlock, chunkCoord);            
            selectedBlockShader.setInt("blockType", chunkMap[chunkCoord].blocks[localPos.x][localPos.y][localPos.z].type);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(selectedBlock));
            model = glm::scale(model, glm::vec3(1.001));

            selectedBlockShader.setMat4("model", model);        
            selectedBlockShader.setMat4("view", view);
            selectedBlockShader.setMat4("projection", projection);
            
            glBindVertexArray(selectedBlockVAO);
            glDrawArrays(GL_TRIANGLES, 0, sizeof(cubeVertices)/sizeof(float));
        }
        
        // Render ImGui
        renderImGui(); 

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    // Cleanup
    for (auto& [_, vbo] : chunkVBOMap) glDeleteBuffers(1, &vbo);
    for (auto& [_, vao] : chunkVAOMap) glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &textureAtlas);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();    
    
    glfwTerminate();

    return 0;
}
