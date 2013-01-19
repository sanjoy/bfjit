#include "compiler.h"

#include "assert.h"
#include "bytecode.h"

#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"

#include "codegen.inc"

#include <sys/mman.h>

typedef struct {
  size_t size;
  char code[0];
} code_buf_t;

static code_buf_t *make_exec(dasm_State **state) {
  size_t size;
  int dasm_status = dasm_link(state, &size);
  assert(dasm_status == DASM_S_OK);
  (void) dasm_status;

  // Allocate memory readable and writable so we can
  // write the encoded instructions there.
  code_buf_t *cbuf = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE,
                          MAP_ANON | MAP_PRIVATE, 0, 0);
  assert(cbuf != MAP_FAILED);

  // Store length at the beginning of the region, so we
  // can free it without additional context.
  cbuf->size = size;

  dasm_encode(state, cbuf->code);
  dasm_free(state);

  // Adjust the memory permissions so it is executable
  // but no longer writable.
  dasm_status = mprotect(cbuf, size, PROT_EXEC | PROT_READ);
  assert(dasm_status == 0);

  return cbuf;
}

int compile_and_install(program_t *p, byte *loop) {
  dasm_State *state;
  dasm_init(&state, 1);
  dasm_setup(&state, actions);

  codegen(p, state, loop);

  code_buf_t *cbuf = make_exec(&state);

  if (p->compiled_code_len == p->compiled_code_capacity) {
    p->compiled_code =
        realloc(p->compiled_code,
                sizeof(compiled_code_t) * p->compiled_code_capacity * 2);
    p->compiled_code_capacity *= 2;
  }

  p->compiled_code[p->compiled_code_len++] = (compiled_code_t) cbuf->code;

  uint32_t *patch = (uint32_t *) loop;
  patch[0] = BC_COMPILED_LOOP;
  patch[1] = p->compiled_code_len - 1;

  return p->compiled_code_len - 1;
}
