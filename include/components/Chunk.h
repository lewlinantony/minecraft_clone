#pragma once

#include "components/Block.h"
#include "config.h"

struct Chunk {
    Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};