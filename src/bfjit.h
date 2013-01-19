#ifndef __BFJIT__H
#define __BFJIT__H

#include <stdint.h>
#include <stdio.h>

#include "utils.h"

typedef byte *(*compiled_code_t) (byte *arena);

#ifndef kHotLoopThreshold
#define kHotLoopThreshold 15
#endif

typedef struct codepage codepage_t;

typedef struct {
  const char *src;
  byte *bytecode;
  compiled_code_t *compiled_code;
  int compiled_code_len;
  int compiled_code_capacity;

  codepage_t *codepages;
  intptr_t begin;
  intptr_t limit;

  /*  program options */
  unsigned int *loop_stack;
  int loop_stack_size;
} program_t;

program_t *p_new(const char *source, int maximum_loop_nesting);
void p_exec(program_t *program, int min_arena_size);
void p_destroy(program_t *program);

#endif
