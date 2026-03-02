#include <game/game.h>
#include <iostream>


int main() {
    Game game;
    std::cout<<"Constructor"<<std::endl;
    game.init();
    std::cout<<"init"<<std::endl;    
    game.run();
    std::cout<<"run"<<std::endl;    
    game.cleanup();
    return 0;
}


/*

optimize chunk meshing further maybe greedy meshing

World::calculateChunk() calls Renderer::initWorldObjects() directly to generate GL buffers.
This creates a circular dependency (World needs Renderer, Renderer needs World).

consider like not setting a y limit cause because of culling we never see anything apart from the top face anyways

*/
