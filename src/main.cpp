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
refactor the collision detection stuff cleaner potentially into its own class
optimize chunk meshing further maybe greedy meshing
*/