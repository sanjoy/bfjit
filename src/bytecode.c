#include "bytecode.h"

#include "bfjit.h"

#include <assert.h>
#include <stdlib.h>

#define LIMIT ((int) ((1 << 30) - 1))

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

#define append_byte4(value) do {                        \
    if (unlikely(bytecode_len + 4 >= (capacity))) {     \
      capacity *= 2;                                    \
      bytecode = realloc(bytecode, capacity);           \
    }                                                   \
    byte *location = (bytecode + bytecode_len);         \
    *((uint32_t *) location) = (uint32_t) (value);      \
    bytecode_len += 4;                                  \
  } while(0)

#define peephole_pass(name)                                             \
  static int peephole_ ## name (                                        \
      byte **bc_ptr, int *bc_len_ptr, int *cap_ptr, int loop_begin_idx, \
      int loop_length)

#define peephole_prologue                                               \
  /*  DO NOT touch these variables: */                                  \
  byte *bytecode = *bc_ptr;                                             \
  int capacity = *cap_ptr;                                              \
                                                                        \
  /* Be careful about modifying this one:  */                           \
  int bytecode_len = *bc_len_ptr;                                       \
                                                                        \
  /*  Modifying these are okay */                                       \
  byte *loop_begin = &bytecode[loop_begin_idx];                         \
  byte *loop_body_begin = &loop_begin[get_total_length(BC_LOOP_BEGIN)]; \
  int loop_body_length = loop_length - get_total_length(BC_LOOP_BEGIN);

#define peephole_successful                     \
  *bc_len_ptr = bytecode_len;                   \
  *bc_ptr = bytecode;                           \
  *cap_ptr = capacity;                          \
  return 1;

#define peephole_failed                         \
  return 0;

#define peep_bc_check(bc, payload)                              \
  peep_check(get_bytecode(loop_body_begin) == bc &&             \
             get_payload(loop_body_begin, 0) == payload)        \
  loop_body_begin += get_total_length(bc)

#define peep_bc_bind_pl0(bc, pl_var)                    \
  peep_check(get_bytecode(loop_body_begin) == bc);      \
  pl_var = get_payload(loop_body_begin, 0);             \
  loop_body_begin += get_total_length(bc);

#define peep_check(condn) if (!(condn)) peephole_failed

/*  Optimize [-] to a direct zero assignment.  */
peephole_pass(nullifying_loop) {
  peephole_prologue;

  peep_check(loop_body_length == get_total_length(BC_ADD));
  peep_bc_check(BC_ADD, -1);

  bytecode_len -= loop_length;
  append_byte4(BC_ZERO);

  peephole_successful;
}

peephole_pass(move_value) {
  peephole_prologue;
  const int expected_length = 2 * get_total_length(BC_ADD) +
                              2 * get_total_length(BC_SHIFT);

  peep_check(loop_body_length == expected_length);
  uint32_t distance;

  peep_bc_check(BC_ADD, -1);
  peep_bc_bind_pl0(BC_SHIFT, distance);
  peep_bc_check(BC_ADD, 1);
  peep_bc_check(BC_SHIFT, -distance);

  bytecode_len -= loop_length;
  append_byte4(BC_MOVE_VALUE);
  append_byte4(distance);

  peephole_successful;
}

byte *bc_from_source(const char *source, unsigned int *loop_stack_ui,
		     int loop_stack_size) {
  int capacity = 16;
  uint32_t heat_counters_len = 0;

  byte *bytecode = malloc(capacity);
  int bytecode_len = 0;

  src_t src;
  src.src = source;
  src.index = 0;

  assert(sizeof(unsigned int) >= sizeof(uint32_t));
  uint32_t *loop_stack = (uint32_t *) loop_stack_ui;
  int loop_stack_index = 0;

  while (1) {
    char c = src.src[src.index];
    switch (c) {
      case '<':
      case '>':
        append_byte4(BC_SHIFT);
        append_byte4(fold_actions(&src, '>', '<'));
        break;

      case '+':
      case '-':
        append_byte4(BC_ADD);
        append_byte4(fold_actions(&src, '+', '-'));
        break;

      case '.':
        src.index++;
        append_byte4(BC_OUTPUT);
        break;

      case ',':
        src.index++;
        append_byte4(BC_INPUT);
        break;

      case '[':
        src.index++;
	if (loop_stack_index == loop_stack_size) die("stack overflow!");
        loop_stack[loop_stack_index++] = bytecode_len;
        append_byte4(BC_LOOP_BEGIN);
        append_byte4(heat_counters_len);
        heat_counters_len = (heat_counters_len + 1) % kNumHeatCounters;
        /* we need not worry heat_counters_len wrapping around. */
        append_byte4(0); /* this will be adjusted on the `]'  */
        break;

      case ']': {
        src.index++;
        if (loop_stack_index == 0) die("unexpected `]'");
        uint32_t begin_pc = loop_stack[--loop_stack_index];
        uint32_t delta = bytecode_len - begin_pc;

        if (peephole_nullifying_loop(&bytecode, &bytecode_len, &capacity,
                                     begin_pc, delta)) {
          break;
        }

        if (peephole_move_value(&bytecode, &bytecode_len, &capacity,begin_pc,
                                delta)) {
          break;
        }

        append_byte4(BC_LOOP_END);
        append_byte4(delta);
        byte *patch_pc = &bytecode[begin_pc] + kByteCodeLen + kPayloadLen;
        *((uint32_t *) patch_pc) = bytecode_len - begin_pc;
        break;
      }

      case 0:
        goto end;

      default:
        src.index++;
    }
  }

end:
  append_byte4(BC_HLT);
  if (loop_stack_index != 0) die("unterminated loop!");
  return bytecode;
}

void bc_dump(FILE *fptr, byte *pc) {
  intptr_t begin = (intptr_t) pc;
  while (1) {
    fprintf(fptr, "%d: ", (int) ((intptr_t) pc - begin));

    int bc = get_bytecode(pc);

    switch (bc) {
      case BC_INVALID:
        fprintf(fptr, "invalid");
        break;

      case BC_SHIFT:
        fprintf(fptr, "shift [delta = %d]", get_payload(pc, 0));
        break;

      case BC_ADD:
        fprintf(fptr, "add [value = %d]", get_payload(pc, 0));
        break;

      case BC_MOVE_VALUE:
        fprintf(fptr, "move_value [value = %d]", get_payload(pc, 0));
        break;

      case BC_ZERO:
        fprintf(fptr, "zero");
        break;

      case BC_OUTPUT:
        fprintf(fptr, "output");
        break;

      case BC_INPUT:
        fprintf(fptr, "input");
        break;

      case BC_LOOP_BEGIN:
        fprintf(fptr, "loop-begin [counter-idx = %d] [length = %d]",
                get_payload(pc, 0), get_payload(pc, 1));
        break;

      case BC_LOOP_END:
        fprintf(fptr, "loop-end [length = %d]", get_payload(pc, 0));
        break;

      case BC_COMPILED_LOOP:
        fprintf(fptr, "compiled-loop");
        break;

      case BC_HLT:
        fprintf(fptr, "hlt");
        goto end;
    }

    fprintf(fptr, "\n");
    pc += get_total_length(bc);
  }

end:
  fprintf(fptr, "\n");
}
