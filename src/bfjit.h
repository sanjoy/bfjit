#ifndef __BFJIT__H
#define __BFJIT__H

#include <stdint.h>
#include <stdio.h>

typedef uint8_t byte;

enum bytecode {
  BC_INVALID,
  BC_SHIFT,
  BC_ADD,
  BC_OUTPUT, BC_INPUT,
  BC_LOOP_BEGIN, BC_LOOP_END,
  BC_COMPILED_LOOP,
  BC_HLT,
  BC_NUM_BYTECODES
};

typedef byte *(*compiled_code_t) (char *arena);

#define kHotFunctionThreshold 255;

typedef struct {
  const char *src;
  byte *bytecode;
  compiled_code_t *compiled_code;
  int compiled_code_len;
  int compiled_code_capacity;
  int heat_counters[256];
} program_t;

program_t *p_new(const char *source);
void p_exec(program_t *program, int min_arena_size);
void p_print_bc(FILE *fp, program_t *program);
void p_destroy(program_t *program);

#endif
