#pragma once

#include "Block.h"
#include "config.h"

// CHUNK
struct Chunk {
    Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};