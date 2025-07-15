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
    if (m_chunkChange) {
        generateTerrain();
        m_chunkChange = false;
    }
    // Update camera position to follow player's eyes
    m_camera.position = m_player.position + glm::vec3(0.0f, m_player.eyeHeight, 0.0f);
    performRaycasting();
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
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)curWidth / (float)curHeight, 0.1f, 5000.0f);

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
                    glDrawArrays(GL_TRIANGLES, 0, m_world.chunkMeshData.at(chunkOrigin).size() / 7);
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
                // If chunk data already exists, skip it
                if (m_world.chunkMap.count(chunkOrigin)) {
                    continue;
                }
                
                // Chunk does not exist, so generate its block data
                Chunk& currentChunk = m_world.chunkMap[chunkOrigin];
                newChunksToMesh.push_back(chunkOrigin);

                for (int x = 0; x < CHUNK_SIZE; x++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        float globalX = (float)(chunkOrigin.x + x);
                        float globalZ = (float)(chunkOrigin.z + z);
                        float height = m_world.noise.GetNoise(globalX, globalZ) * m_world.amplitude; // Scale noise to 0-2*amp
                        height = glm::round(height);

                        for (int y = 0; y < CHUNK_SIZE; y++) {
                            int globalY = chunkOrigin.y + y;
                            if (globalY > height) {
                                currentChunk.blocks[x][y][z].type = 0; // Air
                            } else if (globalY == (int)height) {
                                currentChunk.blocks[x][y][z].type = 1; // Grass
                            } else if (globalY >= height - 5) {
                                currentChunk.blocks[x][y][z].type = 2; // Dirt
                            } else if (globalY >= -40) {
                                currentChunk.blocks[x][y][z].type = 3; // Stone
                            }
                        }
                    }
                }
            }
        }
    }

    // After generating block data, create the meshes for the new chunks
    for (const auto& chunkCoord : newChunksToMesh) {
        calculateChunk(chunkCoord);
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

        if ((i == 5 && neighborPos.y < m_player.position.y)) {     // Bottom
            continue;
        }

        if (neighborBlock == nullptr || neighborBlock->type == 0) {// If chunk doesn't exist or block is air
            visibleFaces.push_back(i);
        }
    }
    return visibleFaces;
}

void Game::calculateChunk(glm::ivec3 chunkCoord) {
    // Clear any previous mesh data for this chunk
    m_world.chunkMeshData[chunkCoord].clear();
    
    // Ensure the chunk exists in the map
    if (m_world.chunkMap.find(chunkCoord) == m_world.chunkMap.end()) {
        return; // Cannot mesh a chunk that hasn't had its block data generated
    }
    Chunk& chunk = m_world.chunkMap.at(chunkCoord);

    const glm::vec3 normals[6] = {
        glm::vec3(0.0f, 1.0f, 0.0f),    // Top (+Y)
        glm::vec3(0.0f, 0.0f, -1.0f),   // Front (-Z)
        glm::vec3(-1.0f, 0.0f, 0.0f),   // Right (-X)
        glm::vec3(0.0f, 0.0f, 1.0f),    // Back (+Z)
        glm::vec3(1.0f, 0.0f, 0.0f),    // Left (+X)
        glm::vec3(0.0f, -1.0f, 0.0f)    // Bottom (-Y)
    };    

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (chunk.blocks[x][y][z].type == 0) continue; // Skip air blocks

                glm::ivec3 blockPosition = chunkCoord + glm::ivec3(x, y, z);
                
                for (int faceID : getVisibleFaces(blockPosition)) {
                    const float* curFace = faceVertices[faceID];
                    if (!curFace) continue;

                    const glm::vec3 normal = normals[faceID];

                    for (int i = 0; i < 6; ++i) { // 6 vertices per face
                        int idx = i * 6; // 6 attributes per vertex in face data

                        // Vertex position 
                        float vx = curFace[idx + 0] + blockPosition.x;
                        float vy = curFace[idx + 1] + blockPosition.y;
                        float vz = curFace[idx + 2] + blockPosition.z;

                        // Texture coordinates
                        float ux = curFace[idx + 3];
                        float uy = curFace[idx + 4];
                        
                        // Face ID and Block Type
                        float fid = curFace[idx + 5];
                        float blockType = static_cast<float>(chunk.blocks[x][y][z].type);

                        m_world.chunkMeshData[chunkCoord].insert(m_world.chunkMeshData[chunkCoord].end(), {
                            vx, vy, vz,           // Position
                            ux, uy,               // UV Coords
                            fid,                  // Face ID
                            blockType,            // Block Type
                            normal.x, normal.y, normal.z // Normal
                        });
                    }
                }
            }
        }
    }

    GLuint chunkVAO, chunkVBO;

    // Check if the chunk already has a VAO/VBO.
    if (m_world.chunkVaoMap.find(chunkCoord) == m_world.chunkVaoMap.end()) {
        glGenVertexArrays(1, &chunkVAO);
        glGenBuffers(1, &chunkVBO);
        m_world.chunkVaoMap[chunkCoord] = chunkVAO;
        m_world.chunkVboMap[chunkCoord] = chunkVBO;

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
    } else {
        // The chunk already exists, so just get its VBO handle for updating.
        chunkVBO = m_world.chunkVboMap.at(chunkCoord);
        glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
    }
    
    // Upload the new vertex data to the VBO
    if (!m_world.chunkMeshData[chunkCoord].empty()) {
        glBufferData(GL_ARRAY_BUFFER, m_world.chunkMeshData[chunkCoord].size() * sizeof(float), m_world.chunkMeshData[chunkCoord].data(), GL_DYNAMIC_DRAW);
    }
}

void Game::calculateChunkAndNeighbors(glm::ivec3 block) {
    glm::ivec3 chunkCoord = m_world.getChunkOrigin(block);
    glm::ivec3 blockOffset = block - chunkCoord;

    // Always recalculate the mesh for the chunk the block is in
    calculateChunk(chunkCoord);

    // If the block is on a boundary, the neighbor chunk's mesh is also affected
    if (blockOffset.x == 0) {
        calculateChunk(chunkCoord + glm::ivec3(-CHUNK_SIZE, 0, 0));
    } else if (blockOffset.x == CHUNK_SIZE - 1) {
        calculateChunk(chunkCoord + glm::ivec3(CHUNK_SIZE, 0, 0));
    }
    
    if (blockOffset.y == 0) {
        calculateChunk(chunkCoord + glm::ivec3(0, -CHUNK_SIZE, 0));
    } else if (blockOffset.y == CHUNK_SIZE - 1) {
        calculateChunk(chunkCoord + glm::ivec3(0, CHUNK_SIZE, 0));
    }

    if (blockOffset.z == 0) {
        calculateChunk(chunkCoord + glm::ivec3(0, 0, -CHUNK_SIZE));
    } else if (blockOffset.z == CHUNK_SIZE - 1) {
        calculateChunk(chunkCoord + glm::ivec3(0, 0, CHUNK_SIZE));
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
    
    m_chunkShader->use();
    m_chunkShader->setInt("text", 0);

    m_selectedBlockShader->use();
    m_selectedBlockShader->setInt("text", 0);
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

void Game::cleanup() {
    // Delete all OpenGL objects
    for (auto& pair : m_world.chunkVboMap) glDeleteBuffers(1, &pair.second);
    for (auto& pair : m_world.chunkVaoMap) glDeleteVertexArrays(1, &pair.second);
    glDeleteTextures(1, &m_textureAtlas);
    glDeleteVertexArrays(1, &m_selectedBlockVao);
    glDeleteBuffers(1, &m_selectedBlockVbo);
    
    // Shutdown ImGui and GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwTerminate();
}

