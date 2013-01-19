#ifndef __COMPILER__H
#define __COMPILER__H

#include "bfjit.h"

int compile_and_install(program_t *prog, byte *loop);
void free_all_compiled_code(program_t *prog);

#endif
