#include "core/Game.h"
#include <iostream>

int main() {
    Game game;
    try {
        game.init();
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }
    game.cleanup();
    return 0;
}