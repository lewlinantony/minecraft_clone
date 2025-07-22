#================================================================================
# Project Setup
#================================================================================
cmake_minimum_required(VERSION 3.10)
project(VoxelGame)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mac specific setting for Apple Silicon
set(CMAKE_OSX_ARCHITECTURES "arm64")

#================================================================================
# Find Packages & Define Source Files
#================================================================================

# Set the main include directory for headers
include_directories(include)

# Find required system/external packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# List only the .cpp files that need to be compiled.
# Header-only files are handled automatically when they are included.
set(SOURCE_FILES
    src/main.cpp
    src/core/Game.cpp
    src/components/World.cpp
    src/rendering/VertexData.cpp
    src/utils/ThreadPool.cpp
)

# Create the final executable from the source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

#================================================================================
# Asset Handling
#================================================================================

# Ensure asset directories exist in the build folder
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)

# Copy assets to the build directory so the executable can find them at runtime.
# Using file(COPY...) is simpler than configure_file for direct copies.
file(COPY shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY assets/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#================================================================================
# Third-Party Library Definitions
#================================================================================

# --- GLAD ---
add_library(glad STATIC extern/glad/src/glad.c)
target_include_directories(glad PUBLIC extern/glad/include)

# --- stb_image ---
add_library(stb_image STATIC extern/stb/stb_image.cpp)
target_include_directories(stb_image PUBLIC extern/stb)

# --- ImGui ---
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
# Tell imgui where to find its own headers
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
# Link imgui to its dependencies. The PUBLIC keyword propagates the necessary
# include directories (like for glfw3.h) to the imgui target.
target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
)

#================================================================================
# Final Linking
#================================================================================

# Specify the include directories needed by the main VoxelGame executable
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
)

# Link all the libraries to the final executable
target_link_libraries(${PROJECT_NAME}
    glad
    stb_image
    imgui
    # Note: glfw and OPENGL_LIBRARIES are already linked to imgui,
    # which is then linked here, so they are transitively linked.
    # However, explicitly listing them is fine and can be clearer.
    glfw
    ${OPENGL_LIBRARIES}
)
