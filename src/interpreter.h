#ifndef __INTERPRETER__H
#define __INTERPRETER__H

#include "bfjit.h"

void interpret(program_t *program, byte *arena, int arena_size);

#endif
