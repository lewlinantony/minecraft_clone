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
Some issues when breaking blocks at chunk boundaries i think

optimize chunk meshing further maybe greedy meshing

rethink the not rendering bottom faces of the blocks below the player to a more refined and consistent approach 
like setting a y limit and never rendering its bottom, and something similar for xz blocks and review the part where
we consider non rendered blocks as air blocks

World::calculateChunk() calls Renderer::initWorldObjects() directly to generate GL buffers.
This creates a circular dependency (World needs Renderer, Renderer needs World).


*/