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


// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float BLOCK_SIZE = 1.0f; 
const float COLLISION_THRESHOLD = 0.2f;
const float TERRAIN_SIZE = 61.0f;

// Physics constants
const float gravity = 30.0f;
const float jumpVelocity = 9.0f;
const float cameraSpeed = 10.0f;
const float mosueSensitivity = 0.1f;

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
glm::vec3 playerPosition = glm::vec3(0.0f, 10.0f, 0.0f);
bool onGround = false;

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
const float rayEnd = 5.0f;
const float rayStep = 0.05f;
glm::ivec3 selectedBlock; // Currently selected block 

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

// Block map
std::unordered_map<glm::ivec3, bool> blockMap; // Map to store blocks in the world


// Function declarations
bool boxBoxOverlap(BoundingBox playerBox, BoundingBox blockBox);
bool pointBoxOverlap(glm::vec3 point, BoundingBox box);
glm::vec3 resolveYCollision(glm::vec3& p_nextPlayerPosition, glm::vec3 y_movement);
glm::vec3 resolveXZCollision(glm::vec3 p_nextPlayerPosition, glm::vec3 p_velocity);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void renderImGui();

bool boxBoxOverlap(BoundingBox playerBox, BoundingBox blockBox){
    return ((playerBox.min.x <= blockBox.max.x && playerBox.max.x >= blockBox.min.x) and
            (playerBox.min.y <= blockBox.max.y && playerBox.max.y >= blockBox.min.y) and
            (playerBox.min.z <= blockBox.max.z && playerBox.max.z >= blockBox.min.z));    
}

bool pointBoxOverlap(glm::vec3 point, BoundingBox box) {
    return (point.x >= box.min.x && point.x <= box.max.x &&
            point.y >= box.min.y && point.y <= box.max.y &&
            point.z >= box.min.z && point.z <= box.max.z);
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
                if (blockMap[glm::ivec3(x,y,z)]){
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
                    if (blockMap[glm::ivec3(x, y, z)]) {
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
                    if (blockMap[glm::ivec3(x, y, z)]) {
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

    xoffset *= mosueSensitivity;
    yoffset *= mosueSensitivity;

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
    playerPosition = resolveXZCollision(nextPlayerPosition, xz_movement);
    
    // jump
    spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if( spaceIsPressed && !spaceWasPressed && onGround){
        velocity += jumpVelocity;
        onGround = false;
    }
    spaceWasPressed = spaceIsPressed;
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
    if (ImGui::Button("Reset Position")) {
        playerPosition = glm::vec3(0.0f, 5.0f, 0.0f);
        velocity = 0.0f;
        onGround = false;
    }

    ImGui::Text("Selected Block: (%d, %d, %d)", selectedBlock.x, selectedBlock.y, selectedBlock.z);

    ImGui::End();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

    // Configure viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core"); 

    // main block coords
   float vertices[] = {

        // Top face (+Y) → faceID = 0
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,    

        // Front face (+Y) → faceID = 0
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,

        // Right face (–X) → faceID = 1
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 2.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 2.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 2.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 2.0f,

        // Bottom face (+Z) → faceID = 5
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 3.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 3.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 3.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 3.0f,        

        // Left face (+X) → faceID = 2
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 4.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 4.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 4.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 4.0f,

        // Bottom face (–Y) → faceID = 3
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 5.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 5.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 5.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 5.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 5.0f,         
    };

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(1337);
    noise.SetFrequency(0.05f);
    
    for (unsigned int x = 0; x < TERRAIN_SIZE; x++) {
        for (unsigned int z = 0; z < TERRAIN_SIZE; z++) {
            for (int y = -2; y < 4; y++) {  
                float height = noise.GetNoise((float)x, (float)z); 
                height = glm::round(height);
                glm::vec3 blockPosition;
                if (y==height)
                    blockPosition = glm::vec3((float)z - (TERRAIN_SIZE-1)/2.0f, height, (float)x - (TERRAIN_SIZE-1)/2.0f);
                else if (y < height) {
                    blockPosition = glm::vec3((float)z - (TERRAIN_SIZE-1)/2.0f, (float)y, (float)x - (TERRAIN_SIZE-1)/2.0f);
                }
                else{
                    continue; // skip if y is above the terrain height
                }
                glm::ivec3 key = glm::ivec3(glm::floor(blockPosition));
                blockMap[key] = true;
            }
        }
    }

    // Create and compile shaders
    Shader shader("shaders/shader.vert", "shaders/shader.frag");

    // Create and configure buffers
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);    

    // Set drawing mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    unsigned int text1;

    glGenTextures(1, &text1);
    glBindTexture(GL_TEXTURE_2D, text1);

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

    shader.use();
    shader.setInt("text", 0);

    // Main render loop
    while(!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Input processing
        processInput(window);

        // plater state update
        if(!onGround) {
            velocity -= gravity * deltaTime;
        }
        glm::vec3 y_movement = glm::vec3(0.0f, velocity * deltaTime, 0.0f); // vertical movement
        glm::vec3 nextPlayerPosition = playerPosition + y_movement;
        playerPosition = resolveYCollision(nextPlayerPosition, y_movement);

        // Update camera position
        cameraPos = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);

        //RayCasting to select blocks
        glm::vec3 ray = glm::normalize(cameraFront);
        glm::vec3 rayOrigin = cameraPos;
        selectedBlock = glm::ivec3(INT_MAX, INT_MAX, INT_MAX); // Reset selected block to an unlikely value

        for (float i = rayStart; i < rayEnd; i += rayStep) {
            glm::vec3 point = rayOrigin + ray * i;
            glm::ivec3 blockPosition = glm::ivec3(glm::round(point));
            if (blockMap.find(blockPosition) != blockMap.end() and blockMap[blockPosition]) {
                selectedBlock = blockPosition;
                break; // Stop when we hit a block
            }
        }

        // Clear the screen
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader
        shader.use();

        // Set uniforms
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, text1);        
    
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // in vectors, dir = target - pos | target = pos + dir
        shader.setMat4("view", view);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH/SCREEN_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);


        // Bind VAO
        glBindVertexArray(VAO);

        // Draw 
        for (const auto& block : blockMap) {
            if (block.second) { // Check if the block exists

                glm::mat4 model = glm::mat4(1.0f);
                glm::vec3 blockPosition = glm::vec3(block.first); // Get position from blockMap key
                model = glm::translate(model, blockPosition);

                shader.setMat4("model", model);

                // Set block type (you can modify this logic as needed)
                blockType = 0; 
                if (glm::ivec3(blockPosition) == selectedBlock) {
                    blockType = 2; 
                }
                shader.setInt("blockType", blockType);

                glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(float));
            }
        }
        
        // Render ImGui
        renderImGui(); 

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();    
    
    glfwTerminate();

    return 0;
}
