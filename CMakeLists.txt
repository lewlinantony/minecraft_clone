cmake_minimum_required(VERSION 3.10)
project(minecraft_clone)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mac specific setting
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Source files
set(SOURCE_FILES
    src/main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Create directory structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# Copy shader files to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert ${CMAKE_CURRENT_BINARY_DIR}/shaders/shader.vert COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/shader.frag COPYONLY)

# Add GLAD library
add_library(glad STATIC extern/glad/src/glad.c)
target_include_directories(glad PUBLIC extern/glad/include)

# Add stb_image library if needed
add_library(stb_image STATIC extern/stb/stb_image.cpp)
target_include_directories(stb_image PUBLIC extern/stb)

# Add ImGui library
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    glad
    stb_image
    imgui
    glfw
    ${OPENGL_LIBRARIES}
)