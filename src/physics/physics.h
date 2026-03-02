#pragma once
#include <physics/collision.h>


// Forward Declarations
class World;
struct Player;
class Collision;

struct Physics {
    void updatePhysics( Player& player, Collision& collision, World& world, float deltaTime, bool& playerMovedChunks ) ;
};