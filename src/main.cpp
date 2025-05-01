#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
} 

void processInput(GLFWwindow* window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
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

    // Set up vertex data
    float vertices[] = {
    //   cords                  textureUV
         -0.5f, -0.5f, -0.5f,     0.0f, 0.0f, //top
         -0.5f,  0.5f, -0.5f,     0.0f, 1.0f,
          0.5f,  0.5f, -0.5f,     1.0f, 1.0f,
         -0.5f, -0.5f, -0.5f,     0.0f, 0.0f,
          0.5f, -0.5f, -0.5f,     1.0f, 0.0f,         
          0.5f,  0.5f, -0.5f,     1.0f, 1.0f,

         -0.5f, -0.5f, -0.5f,     0.0f, 0.0f, //right
         -0.5f,  0.5f, -0.5f,     0.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,     1.0f, 1.0f,
         -0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 
         -0.5f, -0.5f,  0.5f,     1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,     1.0f, 1.0f,

          0.5f, -0.5f, -0.5f,     0.0f, 0.0f, //left
          0.5f,  0.5f, -0.5f,     0.0f, 1.0f,
          0.5f,  0.5f,  0.5f,     1.0f, 1.0f,
          0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 
          0.5f, -0.5f,  0.5f,     1.0f, 0.0f,
          0.5f,  0.5f,  0.5f,     1.0f, 1.0f,


         -0.5f, -0.5f, -0.5f,     0.0f, 0.0f, //front
          0.5f, -0.5f, -0.5f,     1.0f, 0.0f,         
          0.5f, -0.5f,  0.5f,     1.0f, 1.0f,         
         -0.5f, -0.5f, -0.5f,     0.0f, 0.0f, 
         -0.5f, -0.5f,  0.5f,     0.0f, 1.0f,
          0.5f, -0.5f,  0.5f,     1.0f, 1.0f,         

          0.5f,  0.5f, -0.5f,     0.0f, 0.0f, //back
         -0.5f,  0.5f, -0.5f,     1.0f, 0.0f,         
         -0.5f,  0.5f,  0.5f,     1.0f, 1.0f,         
          0.5f,  0.5f, -0.5f,     0.0f, 0.0f, 
          0.5f,  0.5f,  0.5f,     0.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,     1.0f, 1.0f,         


         -0.5f, -0.5f,  0.5f,     0.0f, 0.0f,//bottom
         -0.5f,  0.5f,  0.5f,     0.0f, 1.0f,
          0.5f,  0.5f,  0.5f,     1.0f, 1.0f, 
         -0.5f, -0.5f,  0.5f,     0.0f, 0.0f,
          0.5f, -0.5f,  0.5f,     1.0f, 0.0f,         
          0.5f,  0.5f,  0.5f,     1.0f, 1.0f,         
        
    };

    // unsigned int indices[] = {
    //     0, 1, 2,
    //     0, 3, 2,

    //     0, 1, 5,
    //     0, 4, 5,

    //     0, 3, 7,
    //     0, 4, 7,

    //     3, 2, 6,
    //     3, 7, 6,

    //     1, 2, 6, 
    //     1, 5, 6,

    //     4, 7, 6, 
    //     4, 5, 6
    // };      

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

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);    
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load image, create texture and generate mipmaps
    int widthImg, heightImg, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data1 = stbi_load("assets/img/tessaract.png", &widthImg, &heightImg, &nrChannels, 0);

    GLenum format;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;
    
    if (data1) {
        std::cout<<"yes texture1"<<std::endl;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, widthImg, heightImg, 0, format, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else{
        std::cout<<"no texture1";
    }
    
    stbi_image_free(data1);

    shader.use();
    shader.setInt("texture1", 0);

    // Main render loop
    while(!glfwWindowShouldClose(window))
    {
        // Input processing
        processInput(window);

        // Clear the screen
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader
        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, text1);        



        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -10.0f));
        shader.setMat4("view", view);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH/SCREEN_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        // Bind VAO
        glBindVertexArray(VAO);

        // Draw triangle
        for(unsigned int i = 0; i<3; i++){
            for(unsigned int j = 0; j<3; j++){
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3((float)j - 1.0f, (float)i - 1.0f, 0.0f));
                model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(1.0f, 1.0f, 1.0f));
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES,0,sizeof(vertices)/sizeof(float));
            }
        }

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