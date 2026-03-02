#include "core/Game.h"

void Game::processInput() {
    // Window and Mouse Lock
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }

    // toggle mouse lock
    bool tabIsPressed = glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabIsPressed && !m_input.tabWasPressed) {
        int mode = glfwGetInputMode(m_window, GLFW_CURSOR);
        if (mode == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        m_input.firstMouse = true;
    }
    m_input.tabWasPressed = tabIsPressed;

    // --- Movement ---
    float speed = m_camera.speed * m_deltaTime;
    float currentMoveScale = (m_player.onGround || m_player.creativeMode) ? 1.0f : 0.5f;

    glm::vec3 front = m_camera.front;
    front.y = 0.0f;
    if (glm::length(front) > 0.0f) { // Avoid normalization of zero vector
        front = glm::normalize(front);
    }
    glm::vec3 right = glm::normalize(glm::cross(front, m_camera.up));

    glm::vec3 xz_movement(0.0f);

    // WSAD movement
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS){
        xz_movement += front * speed * currentMoveScale;
    }
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS){
        xz_movement -= front * speed * currentMoveScale;
    }
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS){
        xz_movement += right * speed * currentMoveScale;
    }
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS){
        xz_movement -= right * speed * currentMoveScale;
    }
    
    glm::vec3 oldPos = m_player.position;
    if (!m_player.creativeMode) {
        m_player.position = resolveXZCollision(m_player.position + xz_movement, xz_movement);
    } 
    else {
        m_player.position += xz_movement;
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS){
            m_player.position.y += speed;
        }
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
            m_player.position.y -= speed;
        }
    }

    if (m_world.getChunkOrigin(m_player.position) != m_world.getChunkOrigin(oldPos)) {
        m_chunkChange = true;
    }

    // Jump
    bool spaceIsPressed = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (!m_player.creativeMode && spaceIsPressed && !m_input.spaceWasPressed && m_player.onGround) {
        m_player.velocityY += m_player.jumpVelocity;
        m_player.onGround = false;
    }
    m_input.spaceWasPressed = spaceIsPressed;

    // Block Removal
    bool mouseLeftIsPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftIsPressed && !m_input.mouseLeftWasPressed && m_selectedBlock != glm::ivec3(INT_MAX)) {
        Block* block = m_world.getBlock(m_selectedBlock);
        if(block && block->type != 0) { // Check if block exists and is not air
            m_world.setBlock(m_selectedBlock, 0); // Set to air
            calculateChunkAndNeighbors(m_selectedBlock);
        }
    }
    m_input.mouseLeftWasPressed = mouseLeftIsPressed;

    // Block Placement
    bool mouseRightIsPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (mouseRightIsPressed && !m_input.mouseRightWasPressed && m_previousBlock != glm::ivec3(INT_MAX)) {
        m_world.setBlock(m_previousBlock, curBlockType); 
        calculateChunkAndNeighbors(m_previousBlock);
    }
    m_input.mouseRightWasPressed = mouseRightIsPressed;

    if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS) {
        curBlockType = 1;
    }
    if (glfwGetKey(m_window, GLFW_KEY_2) == GLFW_PRESS) {
        curBlockType = 2;
    }
    if (glfwGetKey(m_window, GLFW_KEY_3) == GLFW_PRESS) {
        curBlockType = 3;
    }    

}

void Game::update() {
    if (!m_player.creativeMode) {
        updatePhysics();
    }
    if (m_chunkChange && !firstLoad) {
        generateTerrain();
        m_chunkChange = false;
    }

    uploadReadyMeshes();

    // Update camera position to follow player's eyes
    m_camera.position = m_player.position + glm::vec3(0.0f, m_player.eyeHeight, 0.0f);
    performRaycasting();
}

void Game::renderLoadingScreen() {
    // Clear the screen to a dark, neutral color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Start a new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- Center the Loading Window ---
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    ImVec2 window_pos = ImVec2(width * 0.5f, height * 0.5f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(320, 130)); // Existing size is sufficient

    // --- Begin the Window ---
    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

    // --- Static "Loading World" Text ---
    ImGui::Dummy(ImVec2(0.0f, 20.0f)); // Add some padding at the top
    const char* title_text = "Loading World";

    // Scale up the font for the title
    ImGui::SetWindowFontScale(1.8f);
    ImVec2 text_size = ImGui::CalcTextSize(title_text);

    // Center the title text horizontally
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - text_size.x) * 0.5f);
    ImGui::Text("%s", title_text);

    // Reset font scale for the animated part
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Space between title and animation

    // --- Animated Asterisks (Extended) ---
    // Use time to cycle through more frames (0 to 5)
    int frame = static_cast<int>(glfwGetTime() * 3.0) % 6;
    const char* animated_chars;
    switch (frame) {
        case 0:  animated_chars = "*";               break;
        case 1:  animated_chars = "* *";            break;
        case 2:  animated_chars = "* * *";          break;
        case 3:  animated_chars = "* * * *";        break;
        case 4:
        case 5:
        default: animated_chars = "* * * * *";      break; // Hold the last frame
    }

    ImVec2 animated_text_size = ImGui::CalcTextSize(animated_chars);
    // Center the animated characters horizontally
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - animated_text_size.x) * 0.5f);
    ImGui::Text("%s", animated_chars);

    ImGui::End();

    // Render the final ImGui draw data
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Game::render() {
    // Clear screen
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get current window size
    int curWidth, curHeight;
    glfwGetFramebufferSize(m_window, &curWidth, &curHeight);
    
    // Create view and projection matrices
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(FOV), (float)curWidth / (float)curHeight, 0.1f, 5000.0f); 

    // --- Render World Chunks ---
    m_chunkShader->use();
    m_chunkShader->setMat4("view", view);
    m_chunkShader->setMat4("projection", projection);
    m_chunkShader->setMat4("model", glm::mat4(1.0f));

    // Set lighting uniforms
    m_chunkShader->setVec3("lightPos", glm::vec3(m_player.position.x, m_player.position.y + 100, m_player.position.z)); // Light high above player
    m_chunkShader->setVec3("viewPos", m_camera.position);
    m_chunkShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));    

    // Bind texture atlas
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureAtlas);

    // Iterate through chunks within render distance of the player
    glm::ivec3 playerChunk = m_world.getChunkOrigin(glm::round(m_player.position));
    for (int cx = -m_world.XZ_RENDER_DIST; cx <= m_world.XZ_RENDER_DIST; cx++) {
        for (int cy = -m_world.Y_RENDER_DIST; cy <= m_world.Y_RENDER_DIST; cy++) {
            for (int cz = -m_world.XZ_RENDER_DIST; cz <= m_world.XZ_RENDER_DIST; cz++) {
                // Cylindrical render distance check
                if (cx * cx + cz * cz > m_world.XZ_RENDER_DIST * m_world.XZ_RENDER_DIST) {
                    continue;
                }
                
                glm::ivec3 chunkOrigin = playerChunk + glm::ivec3(cx, cy, cz) * CHUNK_SIZE;

                // Check if the chunk has a VAO and mesh data to render
                if (m_world.chunkVaoMap.count(chunkOrigin) && m_world.chunkMeshData.count(chunkOrigin)) {
                    glBindVertexArray(m_world.chunkVaoMap.at(chunkOrigin));
                    glDrawArrays(GL_TRIANGLES, 0, m_world.chunkMeshData.at(chunkOrigin).size() / 10);
                }
            }
        }
    }
    
    // --- Render Selected Block Highlight ---
    if (m_selectedBlock != glm::ivec3(INT_MAX) && !m_player.creativeMode){
        m_selectedBlockShader->use();
        Block* block = m_world.getBlock(m_selectedBlock);
        if (block) { // Ensure block exists before trying to render it
            m_selectedBlockShader->setInt("blockType", block->type);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(m_selectedBlock));
            model = glm::scale(model, glm::vec3(1.001f)); // Slightly larger to avoid z-fighting

            m_selectedBlockShader->setMat4("model", model);
            m_selectedBlockShader->setMat4("view", view);
            m_selectedBlockShader->setMat4("projection", projection);
            
            glBindVertexArray(m_selectedBlockVao);
            glDrawArrays(GL_TRIANGLES, 0, 36); // A cube has 36 vertices
        }
    }
    
    // --- Render Skybox ---
    glDepthFunc(GL_LEQUAL);
    m_skyboxShader->use();
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // Create view matrix without translation
    m_skyboxShader->setMat4("view", skyboxView);
    m_skyboxShader->setMat4("projection", projection);
    
    // Skybox cube
    glBindVertexArray(m_skyboxVao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // Set depth function back to default   

    // Render the ImGui overlay
    renderImGui();
}

void Game::renderImGui() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a simple window
    ImGui::Begin("Player Info");
    ImGui::Text("Position: %.2f, %.2f, %.2f", m_player.position.x, m_player.position.y, m_player.position.z);
    glm::ivec3 pChunk = m_world.getChunkOrigin(glm::round(m_player.position));
    ImGui::Text("Chunk: %d, %d, %d", pChunk.x, pChunk.y, pChunk.z);
    ImGui::Text("On Ground: %s", m_player.onGround ? "Yes" : "No");
    ImGui::Text("Velocity Y: %.2f", m_player.velocityY);
    ImGui::Checkbox("Creative Mode", &m_player.creativeMode);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    // Render ImGui draw data
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Game::updatePhysics() {
    // Apply gravity if not on the ground
    if (!m_player.onGround) {
        m_player.velocityY -= m_player.gravity * m_deltaTime;
        // Terminal velocity
        if (m_player.velocityY < -50.0f) {
            m_player.velocityY = -50.0f;
        }
    }

    glm::vec3 y_movement = glm::vec3(0.0f, m_player.velocityY * m_deltaTime, 0.0f);
    glm::vec3 nextPlayerPosition = m_player.position + y_movement;
    
    glm::vec3 oldPos = m_player.position;
    m_player.position = resolveYCollision(nextPlayerPosition, y_movement);

    // Check if player crossed a chunk boundary after physics update
    if (m_world.getChunkOrigin(m_player.position) != m_world.getChunkOrigin(oldPos)) {
        m_chunkChange = true;
    }
}

void Game::performRaycasting() {
    glm::vec3 rayDirection = glm::normalize(m_camera.front);
    glm::vec3 rayOrigin = m_camera.position;
 
    float closestHit = m_rayEnd;
    glm::ivec3 bestHit = glm::ivec3(INT_MAX);
    glm::ivec3 bestPrev = glm::ivec3(INT_MAX);

    for (const auto& offset : m_rayStarts) {
        glm::vec3 curRayOrigin = rayOrigin + offset; // Adjust ray start based on offset
        glm::ivec3 prevBlockThisRay = glm::ivec3(INT_MAX); // Reset previous block to an unlikely value

        for (float i = m_rayStart; i < closestHit; i += m_rayStep) {
            glm::vec3 point = curRayOrigin + rayDirection * i; 
            glm::ivec3 blockPosition = glm::ivec3(glm::round(point));
            
            if(blockPosition == prevBlockThisRay) continue;

            Block* hitBlock = m_world.getBlock(blockPosition);
            if (hitBlock && hitBlock->type != 0) {
                // We hit a non-air block
                if (i < closestHit){
                    closestHit = i; 
                    bestHit = blockPosition;

                    // Check if the previous block is valid for placement
                    BoundingBox prevBlockBox = BoundingBox::box(prevBlockThisRay, BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);                      

                    // here pass the position as playerposition.y - height/2 cause the y of the player is at the bottom of the player not the center
                    BoundingBox playerBox = BoundingBox::box(m_player.position - glm::vec3(0.0f, -m_player.height/2, 0.0f), m_player.width, m_player.height, m_player.depth);

                    // cannot be previous block(block to be placed) if block overlaps with players bounding box
                    if (!boxBoxOverlap(playerBox, prevBlockBox)) {
                        bestPrev = prevBlockThisRay;
                    } else {
                        bestPrev = glm::ivec3(INT_MAX); // Invalid if player would be inside it
                    }
                    break; // Found the closest hit for this thickened ray, move to the next
                }
            }
            prevBlockThisRay = blockPosition;            
        }
    }
    
    // Set the final results as member variables
    this->m_selectedBlock = bestHit;
    this->m_previousBlock = bestPrev;

    if (m_selectedBlock == glm::ivec3(INT_MAX) || m_previousBlock == glm::ivec3(INT_MAX)) return;

    // The closest Block could still not be face alligned, So check its manhatten distance to the previous block
    glm::ivec3 delta = m_selectedBlock - m_previousBlock;
    if (abs(delta.x) + abs(delta.y) + abs(delta.z) != 1) {
        this->m_previousBlock = glm::ivec3(INT_MAX);
    }        
}

void Game::generateTerrain() {
    
    std::vector<glm::ivec3> newChunksToMesh;
    glm::ivec3 playerChunkOrigin = m_world.getChunkOrigin(glm::round(m_player.position));

    for (int cx = -m_world.XZ_LOAD_DIST; cx <= m_world.XZ_LOAD_DIST; cx++) {
        for (int cy = -m_world.Y_LOAD_DIST; cy <= m_world.Y_LOAD_DIST; cy++) {
            for (int cz = -m_world.XZ_LOAD_DIST; cz <= m_world.XZ_LOAD_DIST; cz++) {
                // Use cylindrical distance
                if (cx * cx + cz * cz > m_world.XZ_LOAD_DIST * m_world.XZ_LOAD_DIST) {
                    continue;
                }
                
                glm::ivec3 chunkOrigin = playerChunkOrigin + glm::ivec3(cx, cy, cz) * CHUNK_SIZE;
                m_world.chunkDataMutex.lock(); // Lock manually

                bool chunkExists = m_world.chunkMap.count(chunkOrigin);
                m_world.chunkDataMutex.unlock();

                // If the chunk already exists, we are done with this iteration
                if (chunkExists) {
                    continue;
                }

                if (firstLoad) {
                    m_initialChunksToLoad++;
                }
                
                m_threadPool->enqueue([this, chunkOrigin] {
                    this->generateChunkData(chunkOrigin);
                });
            }
        }
    }
}

void Game::generateChunkData(glm::ivec3 chunkCoord) {
    // temporary chunk to store block data.
    Chunk newChunk;

    // iterate through every block position within this chunk's volume.
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Calculate global coordinates for noise generation
            float globalX = (float)(chunkCoord.x + x);
            float globalZ = (float)(chunkCoord.z + z);

            // Get the surface height from the noise function
            float height = m_world.noise.GetNoise(globalX, globalZ) * m_world.amplitude;
            height = glm::round(height);

            for (int y = 0; y < CHUNK_SIZE; y++) {
                int globalY = chunkCoord.y + y;

                // Determine block type based on height
                if (globalY > height) {
                    newChunk.blocks[x][y][z].type = 0; // Air
                } else if (globalY == (int)height) {
                    newChunk.blocks[x][y][z].type = 1; // Grass
                } else if (globalY >= height - 5) {
                    newChunk.blocks[x][y][z].type = 2; // Dirt
                } else {
                    newChunk.blocks[x][y][z].type = 3; // Stone
                }
            }
        }
    }

    // Lock the world's data and insert the newly generated chunk.
    {
        std::lock_guard<std::mutex> lock(m_world.chunkDataMutex);
        m_world.chunkMap[chunkCoord] = newChunk;
    }

    // now that the data exists, enqueue a job to create its mesh.
    m_threadPool->enqueue([this, chunkCoord] {
        this->createChunkMesh(chunkCoord);
    });
}

void Game::createChunkMesh(glm::ivec3 chunkCoord) {
    // Create a local vector to hold the mesh data for this chunk.
    std::vector<float> meshData;

    // reserving space before hand will prevent reallocations which can be constly overtime
    // meshData.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 0.1 * 6 * 10);


    // These normals correspond to the 6 faces of a cube
    const glm::vec3 normals[6] = {
        glm::vec3(0.0f, 1.0f, 0.0f),  // Top
        glm::vec3(0.0f, 0.0f, -1.0f), // Front
        glm::vec3(-1.0f, 0.0f, 0.0f), // Right
        glm::vec3(0.0f, 0.0f, 1.0f),  // Back
        glm::vec3(1.0f, 0.0f, 0.0f),  // Left
        glm::vec3(0.0f, -1.0f, 0.0f)  // Bottom
    };

    // iterate through every block in the chunk to build the mesh.
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                glm::ivec3 blockPosition = chunkCoord + glm::ivec3(x, y, z);
                Block* block = m_world.getBlock(blockPosition);
                if (!block || block->type == 0) continue;

                for (int faceID : getVisibleFaces(blockPosition)) {
                    const float* curFace = faceVertices[faceID];
                    if (!curFace) continue;
                    const glm::vec3 normal = normals[faceID];
                    for (int i = 0; i < 6; ++i) {
                        int idx = i * 6;
                        meshData.insert(meshData.end(), {
                            curFace[idx + 0] + blockPosition.x, curFace[idx + 1] + blockPosition.y, curFace[idx + 2] + blockPosition.z,
                            curFace[idx + 3], curFace[idx + 4], curFace[idx + 5],
                            static_cast<float>(block->type),
                            normal.x, normal.y, normal.z
                        });
                    }
                }
            }
        }
    }

    if (!meshData.empty()) {
        std::lock_guard<std::mutex> lock(m_readyMeshesMutex);
        m_readyMeshes.push({chunkCoord, std::move(meshData)});
    }
    else {
        if (firstLoad){
            m_initialChunksToLoad--;
        }
    }
}

void Game::uploadReadyMeshes() {

    double timeBudget = 0.5; 
    double startTime = glfwGetTime();

    // Process only one mesh per frame to prevent hitches from uploading too much at once.
    while (!m_readyMeshes.empty() && (glfwGetTime() - startTime) < timeBudget) {
        // Lock the queue, grab one result, and unlock quickly.
        m_readyMeshesMutex.lock();
        ChunkMeshResult result = m_readyMeshes.front();
        m_readyMeshes.pop();
        m_readyMeshesMutex.unlock();
    
        // Lock the world data while we interact with OpenGL objects and maps.
        std::lock_guard<std::mutex> lock(m_world.chunkDataMutex);
    
        GLuint chunkVAO, chunkVBO;
    
        // Check if this chunk already has a VAO/VBO.
        if (m_world.chunkVaoMap.find(result.chunkCoord) == m_world.chunkVaoMap.end()) {
            // If not, create them.
            glGenVertexArrays(1, &chunkVAO);
            glGenBuffers(1, &chunkVBO);
            m_world.chunkVaoMap[result.chunkCoord] = chunkVAO;
            m_world.chunkVboMap[result.chunkCoord] = chunkVBO;
    
            glBindVertexArray(chunkVAO);
            glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
    
            // Set up vertex attribute pointers for our 10-float stride
            int stride = 10 * sizeof(float);
            // Position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            glEnableVertexAttribArray(0);
            // UV Coords
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            // Face ID
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
            glEnableVertexAttribArray(2);
            // Block Type
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(3);
            // Normal
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
            glEnableVertexAttribArray(4);
        } else {
            // If they exist, just get the handle.
            chunkVBO = m_world.chunkVboMap.at(result.chunkCoord);
            glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
        }
    
        // upload the vertex data to the VBO.
        if (!result.meshData.empty()) {
            glBufferData(GL_ARRAY_BUFFER, result.meshData.size() * sizeof(float), result.meshData.data(), GL_DYNAMIC_DRAW);
        }
    
        // store the mesh data in the world for the render loop to know its size.
        m_world.chunkMeshData[result.chunkCoord] = std::move(result.meshData);


        if (firstLoad) {
            if (--m_initialChunksToLoad == 0) {
                firstLoad = false;
            }
        }        
    }    

}


std::vector<int> Game::getVisibleFaces(glm::ivec3 block) {
    std::vector<int> visibleFaces;
    // Directions corresponding to face IDs 0 through 5
    const glm::ivec3 directions[6] = {
        glm::ivec3(0, 1, 0),    // Top (+Y)
        glm::ivec3(0, 0, -1),   // Front (-Z)
        glm::ivec3(-1, 0, 0),   // Right (-X)
        glm::ivec3(0, 0, 1),    // Back (+Z)
        glm::ivec3(1, 0, 0),    // Left (+X)
        glm::ivec3(0, -1, 0)    // Bottom (-Y)
    };

    for (int i = 0; i < 6; ++i) {
        glm::ivec3 neighborPos = block + directions[i];
        Block* neighborBlock = m_world.getBlock(neighborPos);

        if (neighborBlock == nullptr || neighborBlock->type == 0) {// If chunk doesn't exist or block is air
            visibleFaces.push_back(i);
        }
    }
    return visibleFaces;
}

void Game::calculateChunkAndNeighbors(glm::ivec3 block) {
    glm::ivec3 chunkCoord = m_world.getChunkOrigin(block);
    glm::ivec3 blockOffset = block - chunkCoord;

    // A lambda to simplify enqueueing the meshing task
    auto remesh = [this](glm::ivec3 coord) {
        m_threadPool->priority_enqueue([this, coord] {
            this->createChunkMesh(coord);
        });
    };

    // Always remesh the chunk the block is in
    remesh(chunkCoord);

    // If the block is on a boundary, remesh the neighbor chunk too
    if (blockOffset.x == 0) {
        remesh(chunkCoord + glm::ivec3(-CHUNK_SIZE, 0, 0));
    } 
    if (blockOffset.x == CHUNK_SIZE - 1) {
        remesh(chunkCoord + glm::ivec3(CHUNK_SIZE, 0, 0));
    }
    
    if (blockOffset.y == 0) {
        remesh(chunkCoord + glm::ivec3(0, -CHUNK_SIZE, 0));
    } 
    if (blockOffset.y == CHUNK_SIZE - 1) {
        remesh(chunkCoord + glm::ivec3(0, CHUNK_SIZE, 0));
    }

    if (blockOffset.z == 0) {
        remesh(chunkCoord + glm::ivec3(0, 0, -CHUNK_SIZE));
    } 
    if (blockOffset.z == CHUNK_SIZE - 1) {
        remesh(chunkCoord + glm::ivec3(0, 0, CHUNK_SIZE));
    }
}

bool Game::boxBoxOverlap(const BoundingBox& box1, const BoundingBox& box2) const {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
           (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
           (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

glm::vec3 Game::resolveYCollision(glm::vec3 nextPlayerPos, glm::vec3 yMovement) {
    BoundingBox playerBox = BoundingBox::box(
        nextPlayerPos + glm::vec3(0.0f, m_player.height / 2.0f, 0.0f),
        m_player.width,
        m_player.height,
        m_player.depth
    );
    
    int max_x = static_cast<int>(glm::ceil(playerBox.max.x + m_collisionMargin));
    int max_y = static_cast<int>(glm::ceil(playerBox.max.y + m_collisionMargin));
    int max_z = static_cast<int>(glm::ceil(playerBox.max.z + m_collisionMargin));
    int min_x = static_cast<int>(glm::floor(playerBox.min.x - m_collisionMargin));
    int min_y = static_cast<int>(glm::floor(playerBox.min.y - m_collisionMargin));
    int min_z = static_cast<int>(glm::floor(playerBox.min.z - m_collisionMargin));

    m_player.onGround = false;

    for (int x = min_x; x < max_x; x++) {
        for (int y = min_y; y < max_y; y++) {
            for (int z = min_z; z < max_z; z++) {
                Block* block = m_world.getBlock(glm::ivec3(x, y, z));
                if (block && block->type != 0) {
                    BoundingBox blockBox = BoundingBox::box(
                        glm::vec3(x, y, z),
                        BLOCK_SIZE,
                        BLOCK_SIZE + 2.0f * m_collisionGap,
                        BLOCK_SIZE
                    );

                    if (boxBoxOverlap(playerBox, blockBox)) {
                        m_player.velocityY = 0.0f;
                        if (yMovement.y > 0) { // Moving up
                            nextPlayerPos.y = blockBox.min.y - m_player.height;
                        } else { // Moving down
                            nextPlayerPos.y = blockBox.max.y;
                            m_player.onGround = true;
                        }
                        return nextPlayerPos; // Collision handled
                    }
                }
            }
        }
    }
    return nextPlayerPos; // No collision
}

glm::vec3 Game::resolveXZCollision(glm::vec3 nextPlayerPos, glm::vec3 xzMovement) {
    glm::vec3 resolvedPos = nextPlayerPos;

    // Resolve X-axis collision
    if (xzMovement.x != 0) {
        BoundingBox playerBoxX = BoundingBox::box(
            glm::vec3(resolvedPos.x, m_player.position.y + m_player.height / 2.0f, m_player.position.z),
            m_player.width,
            m_player.height,
            m_player.depth
        );

        int max_x = static_cast<int>(glm::ceil(playerBoxX.max.x + m_collisionMargin));
        int max_y = static_cast<int>(glm::ceil(playerBoxX.max.y + m_collisionMargin));
        int max_z = static_cast<int>(glm::ceil(playerBoxX.max.z + m_collisionMargin));
        int min_x = static_cast<int>(glm::floor(playerBoxX.min.x - m_collisionMargin));
        int min_y = static_cast<int>(glm::floor(playerBoxX.min.y - m_collisionMargin));
        int min_z = static_cast<int>(glm::floor(playerBoxX.min.z - m_collisionMargin));

        for (int x = min_x; x < max_x; x++) {
            for (int y = min_y; y < max_y; y++) {
                for (int z = min_z; z < max_z; z++) {
                    if (m_world.getBlock({x, y, z}) && m_world.getBlock({x, y, z})->type != 0) {
                        BoundingBox blockBox = BoundingBox::box(
                            glm::vec3(x, y, z),
                            BLOCK_SIZE + 2.0f * m_collisionGap,
                            BLOCK_SIZE,
                            BLOCK_SIZE
                        );
                        if (boxBoxOverlap(playerBoxX, blockBox)) {
                            if (xzMovement.x > 0) { // Moving right
                                resolvedPos.x = blockBox.min.x - m_player.width / 2;
                            } else { // Moving left
                                resolvedPos.x = blockBox.max.x + m_player.width / 2;
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
    if (xzMovement.z != 0) {
        BoundingBox playerBoxZ = BoundingBox::box(
            glm::vec3(resolvedPos.x, m_player.position.y + m_player.height / 2.0f, resolvedPos.z),
            m_player.width,
            m_player.height,
            m_player.depth
        );

        int max_x = static_cast<int>(glm::ceil(playerBoxZ.max.x + m_collisionMargin));
        int max_y = static_cast<int>(glm::ceil(playerBoxZ.max.y + m_collisionMargin));
        int max_z = static_cast<int>(glm::ceil(playerBoxZ.max.z + m_collisionMargin));
        int min_x = static_cast<int>(glm::floor(playerBoxZ.min.x - m_collisionMargin));
        int min_y = static_cast<int>(glm::floor(playerBoxZ.min.y - m_collisionMargin));
        int min_z = static_cast<int>(glm::floor(playerBoxZ.min.z - m_collisionMargin));

        for (int x = min_x; x < max_x; x++) {
            for (int y = min_y; y < max_y; y++) {
                for (int z = min_z; z < max_z; z++) {
                    if (m_world.getBlock({x, y, z}) && m_world.getBlock({x, y, z})->type != 0) {
                        BoundingBox blockBox = BoundingBox::box(
                            glm::vec3(x, y, z),
                            BLOCK_SIZE,
                            BLOCK_SIZE,
                            BLOCK_SIZE + 2.0f * m_collisionGap
                        );
                        if (boxBoxOverlap(playerBoxZ, blockBox)) {
                            if (xzMovement.z > 0) { // Moving forward
                                resolvedPos.z = blockBox.min.z - m_player.depth / 2;
                            } else { // Moving backward
                                resolvedPos.z = blockBox.max.z + m_player.depth / 2;
                            }
                            goto end_resolve;
                        }
                    }
                }
            }
        }
    }

end_resolve:
    return resolvedPos;
}

void Game::initGlfw() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Voxel Game", NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(m_window);

    // This is the crucial link between the GLFW window and our Game instance
    glfwSetWindowUserPointer(m_window, this);

    // Set callbacks to our static router functions
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback_router);

    // Lock and hide the cursor
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::initGlad() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        exit(-1);
    }
}

void Game::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void Game::initShaders() {
    m_chunkShader = std::make_unique<Shader>("shaders/world/shader.vert", "shaders/world/shader.frag");
    m_selectedBlockShader = std::make_unique<Shader>("shaders/selectedBlock/shader.vert", "shaders/selectedBlock/shader.frag");
    m_skyboxShader = std::make_unique<Shader>("shaders/skybox/shader.vert", "shaders/skybox/shader.frag");
    
    m_chunkShader->use();
    m_chunkShader->setInt("text", 0);

    m_selectedBlockShader->use();
    m_selectedBlockShader->setInt("text", 0);

    m_skyboxShader->use();
    m_skyboxShader->setInt("skybox", 0);    
}

void Game::initTextures() {
    glGenTextures(1, &m_textureAtlas);
    glBindTexture(GL_TEXTURE_2D, m_textureAtlas);

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load image data using stb_image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("assets/img/64x_minecraft_text.png", &width, &height, &nrChannels, 0);

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

    std::vector<std::string> faces {
        "assets/skybox/right.bmp",
        "assets/skybox/left.bmp",
        "assets/skybox/top.bmp",
        "assets/skybox/bottom.bmp",
        "assets/skybox/front.bmp",
        "assets/skybox/back.bmp"
    };
    loadCubemap(faces);    
}

void Game::loadCubemap(std::vector<std::string> faces) {
    glGenTextures(1, &m_cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false); // Cubemaps are often oriented top-left

    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    stbi_set_flip_vertically_on_load(true); // Set it back for other textures
}

void Game::initRenderObjects() {
    // Chunk render objects are initialised as they are created

    // This function initializes the VAO/VBO for the selected block outline
    glGenVertexArrays(1, &m_selectedBlockVao);
    glGenBuffers(1, &m_selectedBlockVbo);

    glBindVertexArray(m_selectedBlockVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_selectedBlockVbo);
    // Note: This relies on s_cubeVertices being defined.
    glBufferData(GL_ARRAY_BUFFER, CUBE_VERTICES_SIZE, cubeVertices, GL_STATIC_DRAW);

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

    glGenVertexArrays(1, &m_skyboxVao);
    glGenBuffers(1, &m_skyboxVbo);

    glBindVertexArray(m_skyboxVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVbo);
    glBufferData(GL_ARRAY_BUFFER, SKYBOX_VERTICES_SIZE, &skyboxVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);    
}

void Game::onMouseMovement(double xpos, double ypos) {
    if (glfwGetInputMode(m_window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
        return; // Don't move camera if cursor is visible
    }

    if (m_input.firstMouse) {
        m_input.lastX = static_cast<float>(xpos);
        m_input.lastY = static_cast<float>(ypos);
        m_input.firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - m_input.lastX;
    float yoffset = m_input.lastY - static_cast<float>(ypos); // reversed since y-coordinates go from top to bottom
    m_input.lastX = static_cast<float>(xpos);
    m_input.lastY = static_cast<float>(ypos);

    xoffset *= m_camera.sensitivity;
    yoffset *= m_camera.sensitivity;

    m_camera.yaw += xoffset;
    m_camera.pitch += yoffset;

    // Clamp the pitch to avoid flipping the camera
    if (m_camera.pitch > m_camera.maxPitch) m_camera.pitch = m_camera.maxPitch;
    if (m_camera.pitch < -m_camera.maxPitch) m_camera.pitch = -m_camera.maxPitch;

    // Recalculate the camera's front vector
    glm::vec3 direction;
    direction.x = cos(glm::radians(m_camera.yaw)) * cos(glm::radians(m_camera.pitch));
    direction.y = sin(glm::radians(m_camera.pitch));
    direction.z = sin(glm::radians(m_camera.yaw)) * cos(glm::radians(m_camera.pitch));
    m_camera.front = glm::normalize(direction);
}

void Game::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Game::mouse_callback_router(GLFWwindow* window, double xpos, double ypos) {
    // we reroute the function becuase glfw expects a c style functions without any attachments to any objects, but if we dont know the object, we cant get member like camera to update data
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->onMouseMovement(xpos, ypos);
    }
}

Game::Game() :
    m_window(nullptr), m_deltaTime(0.0f), m_lastFrame(0.0f), m_chunkChange(false),
    m_rayStart(0.1f), m_rayEnd(4.0f), m_rayStep(0.1f),
    m_rayStarts({
        glm::vec3(0), glm::vec3(0.05f, 0, 0), glm::vec3(-0.05f, 0, 0),
        glm::vec3(0, 0.05f, 0), glm::vec3(0, -0.05f, 0), glm::vec3(0, 0, 0.05f),
        glm::vec3(0, 0, -0.05f)
        }),
    m_collisionGap(0.01f), m_collisionMargin(0.5f),
    m_selectedBlock(glm::ivec3(INT_MAX)),
    m_previousBlock(glm::ivec3(INT_MAX)),
    curBlockType(1)
    {}

void Game::init() {
    initGlfw();
    initGlad();
    initImGui();
    initShaders();
    initTextures();
    initRenderObjects();

    // Configure the noise generator
    m_world.noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_world.noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_world.noise.SetSeed(m_world.g_NoiseSeed);
    m_world.noise.SetFractalOctaves(m_world.g_NoiseOctaves);
    m_world.noise.SetFractalGain(m_world.g_NoiseGain);
    m_world.noise.SetFractalLacunarity(m_world.g_NoiseLacunarity);
    m_world.noise.SetFrequency(m_world.g_NoiseFrequency);

    // checking the number of available cores
    unsigned int num_threads = std::thread::hardware_concurrency();
    m_threadPool = std::make_unique<ThreadPool>(num_threads > 1 ? num_threads - 1 : 1);  

    m_player.position = glm::vec3(0.0f, (m_world.noise.GetNoise(0.0f, 0.0f) + 1. * m_world.amplitude) + 3.0f, 0.0f);

    // Generate the initial terrain around the player
    generateTerrain();
}

void Game::run() {
    // Enable OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // The main game loop
    while (!glfwWindowShouldClose(m_window)) {
        if (firstLoad) {
            // --- LOADING SCREEN LOOP ---
            glfwPollEvents(); // Keep the window responsive

            if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(m_window, true);
            }            

            uploadReadyMeshes();

            // Render our dedicated loading screen
            renderLoadingScreen();

            // Swap buffers to show the loading screen
            glfwSwapBuffers(m_window);

        } 
        else {        
            float currentFrame = static_cast<float>(glfwGetTime());
            m_deltaTime = currentFrame - m_lastFrame;
            m_lastFrame = currentFrame;

            if (m_deltaTime > 0.02f) { 
                m_deltaTime = 0.02f;
            }

            processInput();
            update();
            render();

            glfwSwapBuffers(m_window);
            glfwPollEvents();
        }
    }
}

//debug prints here
void Game::debug(){

}

void Game::cleanup() {
    // Delete all OpenGL objects
    for (auto& pair : m_world.chunkVboMap) glDeleteBuffers(1, &pair.second);
    for (auto& pair : m_world.chunkVaoMap) glDeleteVertexArrays(1, &pair.second);
    glDeleteTextures(1, &m_textureAtlas);
    glDeleteVertexArrays(1, &m_selectedBlockVao);
    glDeleteBuffers(1, &m_selectedBlockVbo);
    glDeleteVertexArrays(1, &m_skyboxVao);
    glDeleteBuffers(1, &m_skyboxVbo);
    glDeleteTextures(1, &m_cubemapTexture);    
    
    // Shutdown ImGui and GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwTerminate();
}

