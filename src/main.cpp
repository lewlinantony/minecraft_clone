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

cleanup the new face culling logic

there is a bit of a wiat before the first few chunks spawn in, look into that

when the first load of chunks are loading even tho we implemented a priority queue for block breaking chunk generation
it still takes a hefty amount of time to like update those chunks


for now dont draw -y faces at vertical limits, but im sure we can figure out something to cull the faces facing away from the player

*/