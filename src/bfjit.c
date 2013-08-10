#include "bfjit.h"

#include "bytecode.h"
#include "compiler.h"
#include "interpreter.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

program_t *p_new(const char *program_source, int loop_stack_size) {
  program_t *prog = alloc(sizeof(program_t));

  prog->loop_stack_size = loop_stack_size;
  prog->loop_stack = alloc(loop_stack_size);

  prog->src = program_source;
  prog->bytecode =
       bc_from_source(program_source, prog->loop_stack, loop_stack_size);
  prog->compiled_code_capacity = 16;
  prog->compiled_code =
      alloc(sizeof(compiled_code_t) * prog->compiled_code_capacity);
  prog->compiled_code_len = 0;

  prog->codepages = NULL;
  prog->begin = 0;
  prog->limit = 0;

  return prog;
}

void p_exec(program_t *program, int arena_size) {
  byte *arena = calloc(sizeof(byte), arena_size);
  interpret(program, arena, arena_size);
  free(arena);
}

void p_destroy(program_t *prog) {
  free_all_compiled_code(prog);
  free(prog->bytecode);
  free(prog->compiled_code);
  free(prog);
}
