# OpenGL Voxel Engine
A Minecraft-inspired voxel engine built from scratch in C++ and modern OpenGL, featuring an infinite, procedurally generated world managed by a multi-threaded chunking system.

<p align="center"><i>Procedural Generation & Multi-threaded Loading</i></p>
<p align="center">
  <img src="assets/showcase_flyover.gif" alt="Flyover Showcase" style="max-width: 100%; height: auto;"/>
</p>
</br>

<p align="center"><i>Physics, Raycasting & Block Interaction</i></p>
<p align="center">
  <img src="assets/showcase_interaction.gif" alt="Block Interaction Showcase" style="max-width: 100%; height: auto;"/>
</p>


---

## Key Features



- **Multi-Threaded World Generation**: Engineered a concurrent architecture to offload chunk generation and meshing from the main render loop. This decouples gameplay from world-building, eliminating stutter and ensuring a consistently smooth user experience.

- **Infinite Terrain**: Procedurally generates an endless, explorable world using FastNoiseLite (OpenSimplex2) with Fractal Brownian Motion (FBm) to create natural-looking mountains and terrain.

- **Efficient Chunk Management**: Utilizes an `std::unordered_map` for chunk storage, leveraging its average O(1) time complexity for fast access to world data as the player moves.

- **Optimized Rendering**: Employs a modern OpenGL 3.3+ core pipeline and renders only the visible faces of blocks by performing face culling during mesh generation, significantly reducing the vertex count per frame.

- **Custom Physics & Raycasting**: Features a simple physics engine with AABB collision detection and a "thickened" raycasting method for reliable block interaction.

---

## Tech Stack

- **Language**: C++17  
- **Graphics**: OpenGL 3.3+  
- **Build System**: CMake

**Libraries Used**:
- [GLFW](https://www.glfw.org/) – Window and input management  
- [GLAD](https://glad.dav1d.de/) – OpenGL function loading  
- [GLM](https://github.com/g-truc/glm) – Math library  
- [stb_image](https://github.com/nothings/stb) – Image loading  
- [Dear ImGui](https://github.com/ocornut/imgui) – Debug UI  
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) – Procedural noise generation  

---

## Build & Run

### Prerequisites

- C++17 compliant compiler (GCC, Clang, MSVC)  
- CMake 3.15+  
- Git  

### Steps

```bash
# 1. Clone the repository
git clone https://github.com/lewlinantony/minecraft_clone.git
cd minecraft_clone

# 2. Configure and build
cmake -B build
cmake --build build --config Release

# 3. Run the engine
# On Linux/macOS
./build/VoxelVerse
# On Windows
./build/Release/VoxelVerse.exe
```

## Future Improvements
- Implement more advanced mesh optimization techniques like Greedy Meshing to further reduce vertex count.

- Tune noise parameters and combine multiple noise maps to create more varied terrain (e.g., biomes, caves).

- Expand the block system to support a wider variety of types and behaviors.

## License
This project is licensed under the [MIT License](LICENSE).
