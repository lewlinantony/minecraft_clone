#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <unordered_map>
#include <functional> // For std::hash



int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;
int blockType;

struct boundingBox{
    glm::vec3 max;
    glm::vec3 min;
};

float gravity = 30.0f;
float velocity = 0.0f;
float jumpVelocity = 9.0f;
float onAirScale = 1.0f;
float cameraSpeed = 10.0f;
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float lastX = SCREEN_WIDTH/2;
float lastY = SCREEN_HEIGHT/2;
float yaw = -90.0f;
float pitch = 0.0f;
float gap = 0.01;
float eyeHeight = 1.6f;
float playerWidth = 0.6;
float playerDepth = 0.6f;
float playerHeight = 1.8;
float margin = 0.5f;

bool firstMouse = true;
bool onGround = false;
bool spaceWasPressed = false;
bool spaceIsPressed = false;
bool YCollision = false;
bool XCollision = false;

glm::vec3 playerPosition    = glm::vec3(0.0f, 10.0f, 0.0f);
glm::vec3 nextPlayerPosition;
glm::vec3 cameraPos         = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
glm::vec3 cameraFront       = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp          = glm::vec3(0.0f, 1.0f,  0.0f);
glm::vec3 cameraRight;

boundingBox nextPlayerBox;

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

std::unordered_map<glm::ivec3, bool> blockMap;

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

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

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
        onAirScale = 1.0f;
    }
    else{
        onAirScale = 0.5f;
    }

    //in out
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        playerPosition += front * speed * onAirScale;
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        playerPosition -= front * speed * onAirScale;
    }

    //left right    
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        playerPosition += right * speed * onAirScale;
    }    
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        playerPosition -= right * speed * onAirScale;
    }        

    // up down
    // if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
    //     cameraPos += cameraUp * speed;
    // }    
    // if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
    //     cameraPos -= cameraUp * speed;
    // }    
    
    // jump
    spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if( spaceIsPressed && !spaceWasPressed && onGround){
        std::cout<<"\n Jump \n"<<std::endl;      
        velocity += jumpVelocity;
        onGround = false;
    }
    spaceWasPressed = spaceIsPressed;
}

bool boxesOverlap(boundingBox playerBox, boundingBox blockBox){
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

        float minPenYPos = FLT_MAX, minPenYNeg = FLT_MAX;
        

        YCollision = false;
        int count = 0;
        std::cout<<"YCollision check..."<<std::endl;
        for(float x = min_x; x< max_x; x++){
            for(float y = min_y; y< max_y; y++){
                for(float z = min_z; z< max_z; z++){
                    if (blockMap[glm::ivec3(x,y,z)]){
                        boundingBox blockBox;
                        blockBox.max = glm::vec3(x + 0.5f, y + 0.5f + gap, z + 0.5f); //add gap for now, remove later if not necessary
                        blockBox.min = glm::vec3(x - 0.5f, y - 0.5f - gap, z - 0.5f);
                        if (boxesOverlap(nextPlayerBox, blockBox)){
                                count++;
                                minPenYNeg = std::min(minPenYNeg, blockBox.max.y - nextPlayerBox.min.y);        
                                minPenYPos = std::min(minPenYPos, nextPlayerBox.max.y - blockBox.min.y); 
                                YCollision = true;
                                std::cout<<"YCollision detected"<<std::endl;
                                std::cout<<"block position :"<<x<<" "<<y<<" "<<z<<std::endl;    
                        }                        
                    }
                }
            }
        }
        if(YCollision){
            std::cout<<"YCollision count :"<<count<<std::endl;
            std::cout<<"Current next player position? :"<<p_nextPlayerPosition.x<<" "<<p_nextPlayerPosition.y<<" "<<p_nextPlayerPosition.z<<std::endl;
            if(minPenYNeg<minPenYPos){
                p_nextPlayerPosition.y += minPenYNeg ;   
                velocity = 0.0f;
                onGround = true;         
            }
            else{
                p_nextPlayerPosition.y -= minPenYPos ;
            }
            std::cout<<"Updated next player position? :"<<p_nextPlayerPosition.x<<" "<<p_nextPlayerPosition.y<<" "<<p_nextPlayerPosition.z<<std::endl;
        }
        else{
            onGround = false;
            std::cout<<"no YCollision"<<std::endl;
        } 
        std::cout<<"\n";
}

void resolveXCollision(glm::vec3& p_nextPlayerPosition){
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

        std::cout<<"Next Player position: "<<p_nextPlayerPosition.x<<" "<<p_nextPlayerPosition.y<<" "<<p_nextPlayerPosition.z<<std::endl;
        std::cout<<"Maximum X: "<<max_x<<std::endl;
        std::cout<<"Minimum X: "<<min_x<<std::endl;
        std::cout<<"Maximum Y: "<<max_y<<std::endl;
        std::cout<<"Minimum Y: "<<min_y<<std::endl;
        std::cout<<"Maximum Z: "<<max_z<<std::endl;
        std::cout<<"Minimum Z: "<<min_z<<std::endl;


        float minPenXPos = FLT_MAX, minPenXNeg = FLT_MAX;
        

        XCollision = false;
        int count = 0;
        std::cout<<"XCollision check..."<<std::endl;
        for(float x = min_x; x< max_x; x++){
            for(float y = min_y; y< max_y; y++){
                for(float z = min_z; z< max_z; z++){
                    if (blockMap[glm::ivec3(x,y,z)]){
                        boundingBox blockBox;
                        blockBox.max = glm::vec3(x + 0.5f , y + 0.5f, z + 0.5f);
                        blockBox.min = glm::vec3(x - 0.5f , y - 0.5f, z - 0.5f);
                        if (boxesOverlap(nextPlayerBox, blockBox)){
                                count++;
                                minPenXNeg = std::min(minPenXNeg, std::max(0.0f, blockBox.max.x - nextPlayerBox.min.x));
                                minPenXPos = std::min(minPenXPos, std::max(0.0f, nextPlayerBox.max.x - blockBox.min.x)); 
                                XCollision = true;
                                std::cout<<"XCollision detected"<<std::endl;
                                std::cout<<"block position :"<<x<<" "<<y<<" "<<z<<std::endl;    
                                std::cout << "minPenXNeg: " << minPenXNeg << ", minPenXPos: " << minPenXPos << std::endl;
                        }                        
                    }
                }
            }
        }
        if(XCollision){
            std::cout<<"XCollision count :"<<count<<std::endl;
            std::cout<<"Current Next Player Pos:"<<p_nextPlayerPosition.x<<" "<<p_nextPlayerPosition.y<<" "<<p_nextPlayerPosition.z<<std::endl;
            if(minPenXNeg<minPenXPos){
                p_nextPlayerPosition.x += minPenXNeg; // the updated player poistion will be the block position + (playerwidth/2) so dont overthink that
            }
            else{
                p_nextPlayerPosition.x -= minPenXPos;
            }
            std::cout<<"Updated Next Player Pos:"<<p_nextPlayerPosition.x<<" "<<p_nextPlayerPosition.y<<" "<<p_nextPlayerPosition.z<<std::endl;

        } 
        else{
            std::cout<<"no XCollision"<<std::endl;
        }   

        std::cout<<"\n";
        
}
glm::vec3 resolveZCollision(glm::vec3 p_nextPlayerPosition){
        nextPlayerBox.max.x = p_nextPlayerPosition.x + (playerWidth/2);
        nextPlayerBox.max.y = p_nextPlayerPosition.y + playerHeight;
        nextPlayerBox.max.z = p_nextPlayerPosition.z + (playerDepth/2);

        nextPlayerBox.min.x = p_nextPlayerPosition.x - (playerWidth/2);
        nextPlayerBox.min.y = p_nextPlayerPosition.y;
        nextPlayerBox.min.z = p_nextPlayerPosition.z - (playerDepth/2);

        //calculate the range of blocks to check for collision
        float max_x = glm::ceil(p_nextPlayerPosition.x + (playerWidth/2));
        float max_y = glm::ceil(p_nextPlayerPosition.y + playerHeight);
        float max_z = glm::ceil(p_nextPlayerPosition.z + (playerDepth/2));

        float min_x = glm::floor(p_nextPlayerPosition.x - (playerWidth/2));
        float min_y = glm::floor(p_nextPlayerPosition.y);
        float min_z = glm::floor(p_nextPlayerPosition.z - (playerDepth/2));

        return p_nextPlayerPosition;    
}


glm::vec3 resolveCollision(glm::vec3 p_nextPlayerPosition){

    
    resolveYCollision(p_nextPlayerPosition);
    
    resolveXCollision(p_nextPlayerPosition);


        

         

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

    for (unsigned int x = 0; x < 7; x++) {
        for (unsigned int y = 0; y < 1; y++) {
            for (unsigned int z = 0; z < 7; z++) {
                glm::vec3 Pos((float)z - 3.0f, (float)y, (float)x - 3.0f);
                glm::ivec3 key = glm::ivec3(glm::floor(Pos));
                blockMap[key] = true;
                std::cout << "Block at: (" 
                        << key.x << ", " 
                        << key.y << ", " 
                        << key.z << ")" << std::endl;
            }
        }
    }
    blockMap[glm::ivec3(glm::floor(glm::vec3(0.0f, 1.0f, 0.0f)))] = true;

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
        
        nextPlayerPosition = playerPosition;

        if(!onGround) {
            velocity -= gravity * deltaTime;
        }
        nextPlayerPosition.y += (velocity * deltaTime);

        playerPosition = resolveCollision(nextPlayerPosition);
        
        // cameraPos = glm::vec3(playerPosition.x, -0.5f,playerPosition.z) + glm::vec3(0.0f, eyeHeight, 0.0f);   // test only
        cameraPos = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);


        // Clear the screen
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader
        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, text1);        
    

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // in vectors, dir = target - pos | target = pos + dir
        shader.setMat4("view", view);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH/SCREEN_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);


        // Bind VAO
        glBindVertexArray(VAO);

        // Draw 
        for(unsigned int x = 0; x<7; x++){
            for(unsigned int y = 0; y<1; y++){
                for(unsigned int z = 0; z<7; z++){
                    glm::mat4 model = glm::mat4(1.0f);
                    glm::vec3 Pos((float)z - 3.0f, (float)y, (float)x - 3.0f);
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

        //single block at center for testing
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 Pos(0.0f, 1.0f, 0.0f);
        model = glm::translate(model, glm::floor(Pos));
        shader.setMat4("model", model);
        shader.setInt("blockType", blockType);
        glDrawArrays(GL_TRIANGLES,0,sizeof(vertices)/sizeof(float));



        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
    
    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    glfwTerminate();
    return 0;
}


/*
* the idea of using booleans(maybe for each direction of the axis) to limit collision like onGround variable for y axis
*/