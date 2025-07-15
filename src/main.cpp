#include "core/Game.h"

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


