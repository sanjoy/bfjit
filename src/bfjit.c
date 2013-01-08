#include "bfjit.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define LIMIT ((int) ((1 << 30) - 1))

#define ensure_space(buffer, length, capacity, space_needed) do {       \
    assert((space_needed) < (capacity));                                \
    if (unlikely((length) + (space_needed) >= (capacity))) {            \
      (capacity) *= 2;                                                  \
      (buffer) = realloc(buffer, capacity);                             \
    }                                                                   \
  } while(0)

typedef struct {
  const char *src;
  int index;
} src_t;

static int is_bfuck_op(char c) {
  return c == '<' || c == '>' || c == '+' || c == '-' ||
         c == '.' || c == ',' || c == '[' || c == ']';
}

static int32_t fold_actions(src_t *src, char increment, char decrement) {
  int effective_action = 0;

  while (1) {
    char c = src->src[src->index];
    if (c == 0) break;

    if (c == increment) effective_action++;
    else if (c == decrement) effective_action--;
    else if (is_bfuck_op(c)) break;

    if (effective_action == LIMIT || effective_action == (-LIMIT)) {
      die("you, sir, have pushed me to my limits (too many '%c's and '%c's) in "
          "succession", increment, decrement);
    }

    src->index++;
  }

  return effective_action;
}

/*  Expects a valid prog->src.  Fills in prog->bytecode,
 *  prog->bytecode_len,  prog->heat_counters.  */
static void translate(program_t *prog, int stack_limit) {
  int capacity = 16;
  uint32_t heat_counters_len = 0;

  prog->bytecode = malloc(capacity);
  int bytecode_len = 0;

#define append_byte(value) do {                                 \
    ensure_space(prog->bytecode, bytecode_len, capacity, 1);    \
    prog->bytecode[bytecode_len++] = value;                     \
  } while(0)

#define append_byte4(value) do {                                \
    ensure_space(prog->bytecode, bytecode_len, capacity, 4);    \
    byte *location = (prog->bytecode + bytecode_len);           \
    *((uint32_t *) location) = (uint32_t) (value);              \
    bytecode_len += 4;                                          \
  } while(0)

  src_t src;
  src.src = prog->src;
  src.index = 0;

  uint32_t *loop_stack = malloc(sizeof(uint32_t) * stack_limit);
  int loop_stack_len = 0;

  while (1) {
    char c = src.src[src.index];
    switch (c) {
      case '<':
      case '>':
        append_byte(BC_SHIFT);
        append_byte4(fold_actions(&src, '>', '<'));
        break;

      case '+':
      case '-':
        append_byte(BC_ADD);
        append_byte4(fold_actions(&src, '+', '-'));
        break;

      case '.':
        src.index++;
        append_byte(BC_OUTPUT);
        break;

      case ',':
        src.index++;
        append_byte(BC_INPUT);
        break;

      case '[':
        src.index++;
        loop_stack[loop_stack_len++] = bytecode_len;
        append_byte(BC_LOOP_BEGIN);
        append_byte4(heat_counters_len++);
        /* we need not worry heat_counters_len wrapping around. */
        append_byte4(0); /* this will be adjusted on the `]'  */
        break;

      case ']': {
        src.index++;
        if (loop_stack_len == 0) die("unexpected `]'");
        uint32_t begin_pc = loop_stack[--loop_stack_len];
        uint32_t delta = bytecode_len - begin_pc;
        append_byte(BC_LOOP_END);
        append_byte4(delta);
        *((uint32_t *) (&prog->bytecode[begin_pc] + 5)) =
            bytecode_len - begin_pc;
        break;
      }

      case 0:
        goto end;

      default:
        src.index++;
    }
  }

end:
  append_byte(BC_HLT);

  if (loop_stack_len != 0) die("unterminated loop!");

  prog->heat_counters = calloc(sizeof(byte), heat_counters_len);
  free(loop_stack);
}

program_t *p_new(const char *program_source) {
  program_t *prog = malloc(sizeof(program_t));

  prog->src = program_source;
  translate(prog, 1024);
  prog->compiled_code_capacity = 16;
  prog->compiled_code =
      malloc(sizeof(compiled_code_t) * prog->compiled_code_capacity);
  prog->compiled_code_len = 0;

  return prog;
}

void p_exec(program_t *program, int min_arena_size) {
  int arena_size = min_arena_size;
  char *arena = calloc(sizeof(char), arena_size);
  int arena_idx = 0;

  byte *pc = program->bytecode;
  while (1) {
    byte bytecode = *pc;
    uint32_t payload = *((uint32_t *) (pc + 1));

    switch (bytecode) {
      case BC_SHIFT:
        arena_idx += (int32_t) payload;
        if (unlikely(arena_idx < 0 || arena_idx >= arena_size)) {
          die("arena pointer out of bounds!");
        }
        pc += 5;
        break;

      case BC_ADD:
        arena[arena_idx] += (int32_t) payload;
        pc += 5;
        break;

      case BC_OUTPUT:
        printf("%c", arena[arena_idx]);
        pc ++;
        break;

      case BC_INPUT:
        scanf("%c", &arena[arena_idx]);
        pc ++;
        break;

      case BC_LOOP_BEGIN:
        /*  A loop is expected to run at least a few times -- hence
         *  the `unlikely'  */
        if (unlikely(!arena[arena_idx])) {
          uint32_t delta = *((uint32_t *) (pc + 5));
          pc += delta;
          break;
        }
        program->heat_counters[payload]++;
        pc += 9;
        break;

      case BC_LOOP_END:
        pc -= payload;
        break;

      case BC_HLT:
        goto end;

      case BC_COMPILED_LOOP:
        die("bc_compiled_loop");

      case BC_INVALID:
        die("uninitialized opcode!");

      default:
        die("unknown opcode encountered!");
    }
  }

end:
  free(arena);
}

void p_destroy(program_t *prog) {
  free(prog->bytecode);
  free(prog->compiled_code);
  free(prog->heat_counters);
  free(prog);
}

void p_print_bc(FILE *fptr, program_t *prog) {
  int index = 0;
  byte *pc = prog->bytecode;

  while (pc[index] != BC_HLT) {
    fprintf(fptr, "%d: ", index);

    switch (pc[index]) {
      case BC_INVALID:
        fprintf(fptr, "invalid");
        index ++;
        break;

      case BC_SHIFT:
        fprintf(fptr, "shift [delta = %d]", *((int32_t *) (pc + index + 1)));
        index += 5;
        break;

      case BC_ADD:
        fprintf(fptr, "add [value = %d]", *((int32_t *) (pc + index + 1)));
        index += 5;
        break;

      case BC_OUTPUT:
        fprintf(fptr, "output");
        index ++;
        break;

      case BC_INPUT:
        fprintf(fptr, "input");
        index ++;
        break;

      case BC_LOOP_BEGIN:
        fprintf(fptr, "loop-begin [counter-idx = %d] [length = %d]",
                *((int32_t *) (pc + index + 1)),
                *((int32_t *) (pc + index + 5)));
        index += 9;
        break;

      case BC_LOOP_END:
        fprintf(fptr, "loop-end [length = %d]",
                *((int32_t *) (pc + index + 1)));
        index += 5;
        break;

      case BC_COMPILED_LOOP:
        fprintf(fptr, "compiled-loop");
        index += 5;
        break;
    }

    fprintf(fptr, "\n");
  }

  fprintf(fptr, "%d: hlt\n", index);
}
