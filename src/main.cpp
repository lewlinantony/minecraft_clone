#include <game/game.h>
#include <iostream>


int main() {
    Game game;
    std::cout<<"Constructor";
    game.init();
    std::cout<<"init";    
    game.run();
    std::cout<<"run";    
    game.cleanup();
    return 0;
}


/*

optimize chunk meshing further maybe greedy meshing(rn bitwise face culling is plenty fast but we'll see)

World::calculateChunk() calls Renderer::initWorldObjects() directly to generate GL buffers.
This creates a circular dependency (World needs Renderer, Renderer needs World).

for now dont draw -y faces at vertical limits, but im sure we can figure out something to cull the faces facing away from the player

*/