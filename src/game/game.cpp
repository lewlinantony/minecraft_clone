#include <game/game.h>

void Game::processInput() {
    // Close window
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
        m_player.position = m_collision.resolveXZCollision(m_player.position + xz_movement, xz_movement, m_world, m_player);
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
        m_playerMovedChunks = true;
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
            // Prevent removing bedrock in survival mode
            if (m_player.creativeMode || m_selectedBlock.y != -m_world.Y_LIMIT) {
                m_world.setBlock(m_selectedBlock, 0); // Set to air
                m_world.calculateChunkAndNeighbors(m_selectedBlock, m_player.position); // Recalculate meshes
            }
        }
    }
    m_input.mouseLeftWasPressed = mouseLeftIsPressed;

    // Block Placement
    bool mouseRightIsPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (mouseRightIsPressed && !m_input.mouseRightWasPressed && m_previousBlock != glm::ivec3(INT_MAX)) {
        Block* block = m_world.getBlock(m_previousBlock);
        if(block && block->type == 0) { // Check if block is air
            m_world.setBlock(m_previousBlock, m_curBlockType); 
            m_world.calculateChunkAndNeighbors(m_previousBlock, m_player.position);
        }
    }
    m_input.mouseRightWasPressed = mouseRightIsPressed;

    // Toggle world generation
    bool pIsPressed = glfwGetKey(m_window, GLFW_KEY_P) == GLFW_PRESS;
    if (m_player.creativeMode && pIsPressed && !m_input.pWasPressed) {
        generateWorld = !generateWorld;
    }
    m_input.pWasPressed = pIsPressed;


    if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS) {
        m_curBlockType = 1;
    }
    if (glfwGetKey(m_window, GLFW_KEY_2) == GLFW_PRESS) {
        m_curBlockType = 2;
    }
    if (glfwGetKey(m_window, GLFW_KEY_3) == GLFW_PRESS) {
        m_curBlockType = 3;
    }    
}

void Game::update() {

    // physics and chunk updation only for non-creative mode
    if (!m_player.creativeMode) {
        m_physics.updatePhysics(m_player, m_collision, m_world, m_deltaTime, m_playerMovedChunks);
    }
    if (m_playerMovedChunks && generateWorld) {
        m_world.generateTerrain(m_player.position);
        m_playerMovedChunks = false;
    }
    // Update camera position to follow player's eyes
    m_camera.position = m_player.position + glm::vec3(0.0f, m_player.eyeHeight, 0.0f);
    performRaycasting();
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
                    if (!m_collision.boxBoxOverlap(playerBox, prevBlockBox)) {
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

void Game::initWorld() {
    m_world.init(m_player.position);  
}

void Game::initRenderer() {
    m_renderer.init(m_window); 
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

void Game::mouse_callback_router(GLFWwindow* window, double xpos, double ypos) {
    // we reroute the function becuase glfw expects a c style functions without any attachments to any objects, but if we dont know the object, we cant get member like camera to update data
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->onMouseMovement(xpos, ypos);
    }
}

void Game::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Game::init() {
    initGlfw();
    initGlad();
    initWorld();
    initRenderer();
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
        m_renderer.render(m_selectedBlock, m_camera, m_player, m_world, m_window);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Game::cleanup() {
    m_world.cleanup();
    m_renderer.cleanup();    
    glfwTerminate();
}
