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
const float COLLISION_THRESHOLD = 0.2;

// world constants
const float terrainSize = 101.0f; //odd number please for symmetry

// Physics constants
const float gravity = 30.0f;
const float jumpVelocity = 9.0f;
const float cameraSpeed = 10.0f;
const float MOUSE_SENSITIVITY = 0.1f;

// Player dimensions
float playerWidth = 0.6;
float playerDepth = 0.6f;
float playerHeight = 1.8;
float eyeHeight = 1.6f;

// Player bounding box
struct BoundingBox{
    glm::vec3 max;
    glm::vec3 min;
};
BoundingBox nextPlayerBox;


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
std::unordered_map<glm::ivec3, bool> blockMap; // map to store blocks in the world

// Player state
float velocity = 0.0f;
glm::vec3 playerPosition    = glm::vec3(0.0f, 5.0f, 0.0f);
bool onGround = false;


// Camera state
float yaw = -90.0f;
float pitch = 0.0f;
glm::vec3 cameraPos         = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
glm::vec3 cameraFront       = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp          = glm::vec3(0.0f, 1.0f,  0.0f);
glm::vec3 cameraRight;

// Mouse state
bool firstMouse = true;
bool spaceWasPressed = false;
bool spaceIsPressed = false;
bool tabWasPressed = false;
bool tabIsPressed = false;
float lastX = SCREEN_WIDTH/2;
float lastY = SCREEN_HEIGHT/2;
float MOVE_SCALE = 0.1f; 

// Frame timing
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

//Uniforms
int blockType;

// Collision detection variables
bool YCollision = false;
bool XZCollision = false;
float gap = 0.01; // gap between player and block during to avoid collision issues
float margin = 0.5f; // constant to grab blocks around the player for collision detection
// Global minPen variables for debugging
float minPenYPos = FLT_MAX, minPenYNeg = FLT_MAX;
float minPenXPos = FLT_MAX, minPenXNeg = FLT_MAX;
float minPenZPos = FLT_MAX, minPenZNeg = FLT_MAX;
// Store currently colliding blocks for debug
std::vector<glm::ivec3> collidingBlocksY;
std::vector<glm::ivec3> collidingBlocksXZ;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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

    xoffset *= MOUSE_SENSITIVITY;
    yoffset *= MOUSE_SENSITIVITY;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;
        
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

    //in out
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        playerPosition += front * speed * MOVE_SCALE;
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        playerPosition -= front * speed * MOVE_SCALE;
    }

    //left right    
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        playerPosition += right * speed * MOVE_SCALE;
    }    
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        playerPosition -= right * speed * MOVE_SCALE;
    }        
    
    // jump
    spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if( spaceIsPressed && !spaceWasPressed && onGround){
        std::cout << "Jumping!" << std::endl;     
        velocity += jumpVelocity;
        onGround = false;
    }
    spaceWasPressed = spaceIsPressed;
}

bool boxesOverlap(BoundingBox playerBox, BoundingBox blockBox){
    return ((playerBox.min.x <= blockBox.max.x && playerBox.max.x >= blockBox.min.x) and
            (playerBox.min.y <= blockBox.max.y && playerBox.max.y >= blockBox.min.y) and
            (playerBox.min.z <= blockBox.max.z && playerBox.max.z >= blockBox.min.z));    
}

void resolveYCollision(glm::vec3& p_nextPlayerPosition){
        nextPlayerBox.max.x = p_nextPlayerPosition.x + (playerWidth/2);
        nextPlayerBox.max.y = p_nextPlayerPosition.y + playerHeight;
        nextPlayerBox.max.z = p_nextPlayerPosition.z + (playerDepth/2);

        nextPlayerBox.min.x = p_nextPlayerPosition.x - (playerWidth/2);
        nextPlayerBox.min.y = p_nextPlayerPosition.y;
        nextPlayerBox.min.z = p_nextPlayerPosition.z - (playerDepth/2);


        float max_x = glm::ceil(nextPlayerBox.max.x + margin);
        float max_y = glm::ceil(nextPlayerBox.max.y + margin);
        float max_z = glm::ceil(nextPlayerBox.max.z + margin);

        float min_x = glm::floor(nextPlayerBox.min.x - margin);
        float min_y = glm::floor(nextPlayerBox.min.y - margin);
        float min_z = glm::floor(nextPlayerBox.min.z - margin);

        // Reset global minPenY values
        minPenYPos = FLT_MAX;
        minPenYNeg = FLT_MAX;
        // Clear Y colliding blocks at the start of collision check
        collidingBlocksY.clear();

        YCollision = false;
        for(float x = min_x; x< max_x; x++){
            for(float y = min_y; y< max_y; y++){
                for(float z = min_z; z< max_z; z++){
                    if (blockMap[glm::ivec3(x,y,z)]){
                        BoundingBox blockBox;
                        blockBox.max = glm::vec3(x + BLOCK_SIZE/2, y + BLOCK_SIZE/2 + gap, z + BLOCK_SIZE/2); 
                        blockBox.min = glm::vec3(x - BLOCK_SIZE/2, y - BLOCK_SIZE/2 - gap, z - BLOCK_SIZE/2);
                        if (boxesOverlap(nextPlayerBox, blockBox)){
                            minPenYNeg = std::min(minPenYNeg, blockBox.max.y - nextPlayerBox.min.y);        
                            minPenYPos = std::min(minPenYPos, nextPlayerBox.max.y - blockBox.min.y); 
                            YCollision = true;    
                            // Add colliding block position for Y only if not already present
                            glm::ivec3 blockPos(x, y, z);
                            bool alreadyPresent = false;
                            for (const auto& blk : collidingBlocksY) {
                                if (blk == blockPos) {
                                    alreadyPresent = true;
                                    break;
                                }
                            }
                            if (!alreadyPresent) {
                                collidingBlocksY.push_back(blockPos);
                            }
                        }                        
                    }
                }
            }
        }

        if(YCollision){
            std::cout << "Before Y collision resolution: (" 
                    << p_nextPlayerPosition.x << ", " 
                    << p_nextPlayerPosition.y << ", " 
                    << p_nextPlayerPosition.z << ")" << std::endl;            
            if ((minPenYNeg < FLT_MAX && minPenYPos < FLT_MAX) && (minPenYNeg < COLLISION_THRESHOLD || minPenYPos < COLLISION_THRESHOLD)) {
                velocity = 0.0f;
                if(minPenYNeg<minPenYPos){
                    p_nextPlayerPosition.y += minPenYNeg ;   
                    onGround = true;         
                }
                else{
                    p_nextPlayerPosition.y -= minPenYPos ;
                    onGround = false;
                }
            }
            std::cout << "Before Y collision resolution: (" 
                    << p_nextPlayerPosition.x << ", " 
                    << p_nextPlayerPosition.y << ", " 
                    << p_nextPlayerPosition.z << ")" << std::endl;                        
        }
        else{
            onGround = false;
        } 
}

void resolveXZCollision(glm::vec3& p_nextPlayerPosition){
        nextPlayerBox.max.x = p_nextPlayerPosition.x + (playerWidth/2);
        nextPlayerBox.max.y = p_nextPlayerPosition.y + playerHeight;
        nextPlayerBox.max.z = p_nextPlayerPosition.z + (playerDepth/2);

        nextPlayerBox.min.x = p_nextPlayerPosition.x - (playerWidth/2);
        nextPlayerBox.min.y = p_nextPlayerPosition.y;
        nextPlayerBox.min.z = p_nextPlayerPosition.z - (playerDepth/2);


        float max_x = glm::ceil(nextPlayerBox.max.x + margin);
        float max_y = glm::ceil(nextPlayerBox.max.y + margin);
        float max_z = glm::ceil(nextPlayerBox.max.z + margin);

        float min_x = glm::floor(nextPlayerBox.min.x - margin);
        float min_y = glm::floor(nextPlayerBox.min.y - margin);
        float min_z = glm::floor(nextPlayerBox.min.z - margin);

        // Reset global minPenXZ values
        minPenXPos = FLT_MAX;
        minPenXNeg = FLT_MAX;
        minPenZPos = FLT_MAX;
        minPenZNeg = FLT_MAX;
        // Clear XZ colliding blocks at the start of collision check
        collidingBlocksXZ.clear();
        XZCollision = false;
        int count = 0;
        for(float x = min_x; x< max_x; x++){
            for(float y = min_y; y< max_y; y++){
                for(float z = min_z; z< max_z; z++){
                    if (blockMap[glm::ivec3(x,y,z)]){
                        BoundingBox blockBox;
                        blockBox.max = glm::vec3(x + BLOCK_SIZE/2 + gap, y + BLOCK_SIZE/2, z + BLOCK_SIZE/2 + gap); // without this gap there is the teleportation issue for -x and -z not sure why but this fixes it
                        blockBox.min = glm::vec3(x - BLOCK_SIZE/2 - gap, y - BLOCK_SIZE/2, z - BLOCK_SIZE/2 - gap);
                        if (boxesOverlap(nextPlayerBox, blockBox)){
                                // std::cout << "XZCollision with block at: (" 
                                //         << x << ", " 
                                //         << y << ", " 
                                //         << z << ")" << std::endl;
                                count++;
                                minPenXNeg = std::min(minPenXNeg, std::max(0.0f, blockBox.max.x - nextPlayerBox.min.x));
                                minPenXPos = std::min(minPenXPos, std::max(0.0f, nextPlayerBox.max.x - blockBox.min.x)); 
                                
                                minPenZNeg = std::min(minPenZNeg, std::max(0.0f, blockBox.max.z - nextPlayerBox.min.z));
                                minPenZPos = std::min(minPenZPos, std::max(0.0f, nextPlayerBox.max.z - blockBox.min.z));

                                XZCollision = true;
                                // Add colliding block position for XZ only if not already present
                                glm::ivec3 blockPos(x, y, z);
                                bool alreadyPresent = false;
                                for (const auto& blk : collidingBlocksXZ) {
                                    if (blk == blockPos) {
                                        alreadyPresent = true;
                                        break;
                                    }
                                }
                                if (!alreadyPresent) {
                                    collidingBlocksXZ.push_back(blockPos);
                                }
                        }                        
                    }
                }
            }
        }

        if(XZCollision){
            std::cout << "minPenXNeg: " << minPenXNeg << std::endl;
            std::cout << "minPenXPos: " << minPenXPos << std::endl;
            std::cout << "minPenZNeg: " << minPenZNeg << std::endl;
            std::cout << "minPenZPos: " << minPenZPos << std::endl;
            std::cout << "XZCollision count: " << count << std::endl;
            std::cout << "Before XZ collision resolution: (" 
                      << p_nextPlayerPosition.x << ", " 
                      << p_nextPlayerPosition.y << ", " 
                      << p_nextPlayerPosition.z << ")" << std::endl;
            // X axis
            if (minPenXNeg < FLT_MAX && minPenXPos < FLT_MAX && (minPenXNeg < COLLISION_THRESHOLD || minPenXPos < COLLISION_THRESHOLD)) {
                if (minPenXNeg < minPenXPos) {
                    p_nextPlayerPosition.x += minPenXNeg;
                } else {
                    p_nextPlayerPosition.x -= minPenXPos;
                }
            }            
            // Z axis
            if (minPenZNeg < FLT_MAX && minPenZPos < FLT_MAX && (minPenZNeg < COLLISION_THRESHOLD || minPenZPos < COLLISION_THRESHOLD)) {
                if (minPenZNeg < minPenZPos) {
                    p_nextPlayerPosition.z += minPenZNeg;
                } else {
                    p_nextPlayerPosition.z -= minPenZPos;
                }
            }
            std::cout << "After XZ collision resolution: (" 
                      << p_nextPlayerPosition.x << ", " 
                      << p_nextPlayerPosition.y << ", " 
                      << p_nextPlayerPosition.z << ")" << std::endl;            

        } 
        
}

glm::vec3 resolveCollision(glm::vec3 p_nextPlayerPosition){
    
    resolveXZCollision(p_nextPlayerPosition);     
    resolveYCollision(p_nextPlayerPosition);    

    return p_nextPlayerPosition;    

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
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
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

    
    for (unsigned int x = 0; x < terrainSize; x++) {
        for (unsigned int y = 0; y < 1; y++) {
            for (unsigned int z = 0; z < terrainSize; z++) {
                float height = noise.GetNoise((float)x, (float)z); 
                glm::vec3 Pos((float)z - (terrainSize-1)/2.0f, height, (float)x - (terrainSize-1)/2.0f);
                glm::ivec3 key = glm::ivec3(glm::floor(Pos));
                blockMap[key] = true;
                std::cout << "Block at: (" 
                        << key.x << ", " 
                        << key.y << ", " 
                        << key.z << ")" << std::endl;
            }
        }
    }



    // Create and compile shaders
    Shader shader("shaders/shader.vert", "shaders/shader.frag");

    // Create and configure buffers
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // glGenBuffers(1, &EBO);

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
        playerPosition.y += (velocity * deltaTime);
        playerPosition = resolveCollision(playerPosition);

        // Update camera position
        cameraPos = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);


        //imgui stuff
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug Window");
        ImGui::Text("Player Position: %.2f, %.2f, %.2f", playerPosition.x, playerPosition.y, playerPosition.z);
        ImGui::Text("Player Bounding Box:");
        ImGui::Text("  min: (%.2f, %.2f, %.2f)", 
            playerPosition.x - playerWidth/2, 
            playerPosition.y, 
            playerPosition.z - playerDepth/2);
        ImGui::Text("  max: (%.2f, %.2f, %.2f)", 
            playerPosition.x + playerWidth/2, 
            playerPosition.y + playerHeight, 
            playerPosition.z + playerDepth/2);
        ImGui::Text("On Ground: %s", onGround ? "Yes" : "No");
        ImGui::Text("Velocity: %.2f", velocity);
        ImGui::Text("Y Collision: %s", YCollision ? "Yes" : "No");
        ImGui::Text("XZ Collision: %s", XZCollision ? "Yes" : "No");
        ImGui::Text("minPenYNeg: %s", minPenYNeg == FLT_MAX ? "none" : std::to_string(minPenYNeg).c_str());
        ImGui::Text("minPenYPos: %s", minPenYPos == FLT_MAX ? "none" : std::to_string(minPenYPos).c_str());
        ImGui::Text("minPenXNeg: %s", minPenXNeg == FLT_MAX ? "none" : std::to_string(minPenXNeg).c_str());
        ImGui::Text("minPenXPos: %s", minPenXPos == FLT_MAX ? "none" : std::to_string(minPenXPos).c_str());
        ImGui::Text("minPenZNeg: %s", minPenZNeg == FLT_MAX ? "none" : std::to_string(minPenZNeg).c_str());
        ImGui::Text("minPenZPos: %s", minPenZPos == FLT_MAX ? "none" : std::to_string(minPenZPos).c_str());
        // Print currently colliding blocks for Y
        if (!collidingBlocksY.empty()) {
            ImGui::Text("Y Colliding Blocks:");
            for (const auto& blk : collidingBlocksY) {
                ImGui::Text("  (%d, %d, %d)", blk.x, blk.y, blk.z);
            }
        } else {
            ImGui::Text("Y Colliding Blocks: none");
        }
        // Print currently colliding blocks for XZ
        if (!collidingBlocksXZ.empty()) {
            ImGui::Text("XZ Colliding Blocks:");
            for (const auto& blk : collidingBlocksXZ) {
                ImGui::Text("  (%d, %d, %d)", blk.x, blk.y, blk.z);
            }
        } else {
            ImGui::Text("XZ Colliding Blocks: none");
        }
        if (ImGui::Button("Reset Position")) {
            playerPosition = glm::vec3(0.0f, 5.0f, 0.0f);
            velocity = 0.0f;
            onGround = false;
        }
        ImGui::End();                


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
        for(unsigned int x = 0; x<terrainSize; x++){
            for(unsigned int y = 0; y<1; y++){
                for(unsigned int z = 0; z<terrainSize; z++){

                    glm::mat4 model = glm::mat4(1.0f);

                    float height = noise.GetNoise((float)x, (float)z); // scale and offset
                    glm::vec3 Pos((float)z - (terrainSize-1)/2.0f, height, (float)x - (terrainSize-1)/2.0f);
                    
                    model = glm::translate(model, glm::floor(Pos));
                    shader.setMat4("model", model);
                    if (y==8){
                        blockType = 0;
                    }
                    // else if (y>=6){
                    //     blockType = 1;
                    // }
                    // else {
                    //     blockType = 2;
                    // }
                    shader.setInt("blockType", blockType);
                    glDrawArrays(GL_TRIANGLES,0,sizeof(vertices)/sizeof(float));
                }
            }
        }
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();    
    
    glfwTerminate();
    return 0;
}

