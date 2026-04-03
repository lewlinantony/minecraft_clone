#include <renderer/renderer.h>
#include <world/world.h>
#include <core/camera.h>
#include <player/player.h>
#include <iostream>


void Renderer::initShaders() {
    chunkShader = std::make_unique<Shader>("shaders/world/shader.vert", "shaders/world/shader.frag");
    selectedBlockShader = std::make_unique<Shader>("shaders/selectedBlock/shader.vert", "shaders/selectedBlock/shader.frag");
    
    chunkShader->use();
    chunkShader->setInt("text", 0); // "text" is the texture sampler uniform in the shader

    selectedBlockShader->use();
    selectedBlockShader->setInt("text", 0);
}

void Renderer::initTextures() {
    glGenTextures(1, &textureAtlas);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load image data using stb_image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("assets/img/128x_texture_atlas.png", &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        else {
            std::cerr << "Unsupported image format with " << nrChannels << " channels." << std::endl;
            stbi_image_free(data);
            return;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture" << std::endl;
    }
    
    stbi_image_free(data);
}

void Renderer::initSelectedBlockObjects() {
    // Chunk render objects are initialised as they are created

    // This function initializes the VAO/VBO for the selected block outline
    glGenVertexArrays(1, &selectedBlockVao);
    glGenBuffers(1, &selectedBlockVbo);

    glBindVertexArray(selectedBlockVao);

    glBindBuffer(GL_ARRAY_BUFFER, selectedBlockVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute (stride of 6 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Face ID attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Renderer::initWorldObjects(GLuint chunkVAO, GLuint chunkVBO, glm::ivec3 chunkCoord, World& world) {
    glGenVertexArrays(1, &chunkVAO);
    glGenBuffers(1, &chunkVBO);
    world.chunkVaoMap[chunkCoord] = chunkVAO;
    world.chunkVboMap[chunkCoord] = chunkVBO;

    glBindVertexArray(chunkVAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);

    // Update vertex attribute pointers for the new 10-float stride
    int stride = 10 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1); // UV Coords
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2); // Face ID
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3); // Block Type
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(4); // Normal    
}

void Renderer::initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void Renderer::render(glm::ivec3 selectedBlock, Camera& camera, Player& player, World& world, GLFWwindow* window) {
    // Clear screen
    glm::vec3 skyColor = glm::vec3(0.39f, 0.58f, 0.93f);
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get current window size
    int curWidth, curHeight;
    glfwGetFramebufferSize(window, &curWidth, &curHeight);
    
    // Create view and projection matrices
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)curWidth / (float)curHeight, 0.1f, 5000.0f);
    

    // Update frustum each frame
    Frustum frustum;
    frustum.update(projection * view);

    // --- Render World Chunks ---
    chunkShader->use();
    chunkShader->setMat4("view", view);
    chunkShader->setMat4("projection", projection);
    chunkShader->setMat4("model", glm::mat4(1.0f));

    // Set lighting uniforms
    chunkShader->setVec3("lightPos", glm::vec3(player.position.x, player.position.y + 100, player.position.z)); // Light high above player
    chunkShader->setVec3("viewPos", camera.position);
    chunkShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));    
    chunkShader->setVec3("skyColor", skyColor);

    float calculatedFogMax = static_cast<float>(world.XZ_RENDER_DIST * CHUNK_SIZE);
    float calculatedFogMin = calculatedFogMax - static_cast<float>(CHUNK_SIZE)*5.0f; 
    chunkShader->setFloat("fogMax", calculatedFogMax);
    chunkShader->setFloat("fogMin", calculatedFogMin);    

    // Bind texture atlas (dosent have to be done every frame ideally but the performance impact is negligible)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    
    // Iterate through chunks within render distance of the player
    glm::ivec3 playerChunk = world.getChunkOrigin(glm::round(player.position));

    totalVisibleChunks = 0;
    inFrustumChunks = 0;
    
    std::vector<glm::ivec3> visibleChunkOrigins;

    for (int cx = -world.XZ_RENDER_DIST; cx <= world.XZ_RENDER_DIST; cx++) {
        for (int cz = -world.XZ_RENDER_DIST; cz <= world.XZ_RENDER_DIST; cz++) {
            
            // Cylindrical render distance check
            if (cx * cx + cz * cz > world.XZ_RENDER_DIST * world.XZ_RENDER_DIST) {
                continue;
            }

            for (int y = -world.Y_LIMIT; y <= world.Y_LIMIT; y++) {
                totalVisibleChunks++;
                
                //decoupling Y to iterate from Y_LIMIT to -Y_LIMIT cause worlds vertical bounds is fixed and is independent of the players position
                glm::ivec3 chunkOrigin = glm::ivec3(
                    playerChunk.x + (cx * CHUNK_SIZE),
                    y * CHUNK_SIZE, 
                    playerChunk.z + (cz * CHUNK_SIZE)
                );

                // Define chunk Bounding Box
                glm::vec3 min = glm::vec3(chunkOrigin);
                glm::vec3 max = min + glm::vec3(CHUNK_SIZE);    
                
                // Frustum culling
                if (!frustum.isBoxVisible(min, max)) {
                    continue; // Skip
                }                
                
                // Check if the chunk has a VAO and mesh data to render
                auto vaoIt = world.chunkVaoMap.find(chunkOrigin);
                if (vaoIt != world.chunkVaoMap.end()) {
                    
                    auto countIt = world.chunkVertexCountMap.find(chunkOrigin);
                    if (countIt != world.chunkVertexCountMap.end()) {
                        visibleChunkOrigins.push_back(chunkOrigin);
                    }
                }
            }
        }
    }

    // Pass 1: Opaque Blocks
    chunkShader->setInt("renderPass", 0); // 0 = Opaque
    for (const auto& chunkOrigin : visibleChunkOrigins) {
        auto vaoIt = world.chunkVaoMap.find(chunkOrigin);
        auto countIt = world.chunkVertexCountMap.find(chunkOrigin);
        glBindVertexArray(vaoIt->second);
        glDrawArrays(GL_TRIANGLES, 0, countIt->second);
        inFrustumChunks++;
    }

    // Pass 2: Transparent Blocks (Water)
    chunkShader->setInt("renderPass", 1); // 1 = Transparent
    // Disable depth mask so transparent faces don't occlude other transparent faces / themselves in weird ways
    // (though OpenGL order might still be off, this prevents background holes)
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND); // ensure blending is on
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& chunkOrigin : visibleChunkOrigins) {
        auto vaoIt = world.chunkVaoMap.find(chunkOrigin);
        auto countIt = world.chunkVertexCountMap.find(chunkOrigin);
        glBindVertexArray(vaoIt->second);
        glDrawArrays(GL_TRIANGLES, 0, countIt->second);
    }
    glDepthMask(GL_TRUE); // re-enable for the next frame / next elements
    
    // --- Render Selected Block Highlight ---
    if (selectedBlock != glm::ivec3(INT_MAX) && !player.creativeMode){
        selectedBlockShader->use();
        Block* block;
        {
            std::shared_lock<std::shared_mutex> lock(world.chunkMapMutex);
            block = world.getBlock(selectedBlock);
        }
        if (block) { // Ensure block exists before trying to render it
            selectedBlockShader->setInt("blockType", block->type);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(selectedBlock));
            model = glm::scale(model, glm::vec3(1.001f)); // Slightly larger to avoid z-fighting

            selectedBlockShader->setMat4("model", model);
            selectedBlockShader->setMat4("view", view);
            selectedBlockShader->setMat4("projection", projection);
            
            glBindVertexArray(selectedBlockVao);
            glDrawArrays(GL_TRIANGLES, 0, 36); // A cube has 36 vertices
        }
    }
}

void Renderer::renderImGui(Player& player, World& world, float* updateTimes, float* renderTimes, float* queueSizes, int timeIndex) {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a simple window
    ImGui::Begin("Debug Info");

    // Performance
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    // Player State 
    ImGui::SeparatorText("Player");
    ImGui::Text("  Pos:   X:%.1f Y:%.1f Z:%.1f", player.position.x, player.position.y, player.position.z);
    
    glm::ivec3 pChunk = world.getChunkOrigin(glm::round(player.position));
    // Formatting the chunk coord to look like a vector
    ImGui::Text("  Chunk: [%d, %d, %d]", pChunk.x, pChunk.y, pChunk.z); 
    
    ImGui::Spacing();
    ImGui::Checkbox("Creative Mode", &player.creativeMode);


    // Renderer Stats 
    ImGui::Spacing();
    ImGui::SeparatorText("Renderer");
    ImGui::Text("  Chunks: %d / %d", inFrustumChunks, totalVisibleChunks); // "Active / Total" format is cleaner
    ImGui::Text("  Culled: %d", totalVisibleChunks - inFrustumChunks);

    // Profiling Graphs 
    ImGui::Spacing();
    ImGui::SeparatorText("Profiling"); // specific ImGui widget for headers
    // Make graphs slightly shorter (height=60) to save screen space
    ImGui::PlotLines("Main", updateTimes, 100, timeIndex, nullptr, 0.0f, 20.0f, ImVec2(300, 50));

    ImGui::PlotLines("Workers", queueSizes, 100, timeIndex, nullptr, 0.0f, FLT_MAX, ImVec2(300, 50));
    
    ImGui::PlotLines("Render", renderTimes, 100, timeIndex, nullptr, 0.0f, 20.0f, ImVec2(300, 50)); 
    
    ImGui::End();

    // Render ImGui draw data
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::init(GLFWwindow* window) {
    initShaders();
    initTextures();
    initSelectedBlockObjects();
    initImGui(window);    
}

void Renderer::cleanup(){
    // Delete textures and buffers
    glDeleteTextures(1, &textureAtlas);
    glDeleteVertexArrays(1, &selectedBlockVao);
    glDeleteBuffers(1, &selectedBlockVbo);
    
    // Shutdown ImGui and GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();    
}