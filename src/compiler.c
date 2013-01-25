#include "compiler.h"

#include "assert.h"
#include "bytecode.h"

#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"

#include "codegen.inc"

#include <sys/mman.h>
#include <unistd.h>

struct codepage {
  size_t size;
  codepage_t *next;
  byte page[];
};

static void ensure_space(program_t *program, size_t size) {
  if (unlikely((program->begin + size) > program->limit)) {
    int pagesize = getpagesize();

    size += sizeof(codepage_t);
    size = ((size + pagesize - 1) / pagesize) * pagesize;

    codepage_t *new_page =
        mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);

    assert(new_page != MAP_FAILED);

    new_page->size = size;
    new_page->next = program->codepages;
    program->codepages = new_page;

    program->begin = (intptr_t) new_page->page;
    program->limit = (intptr_t) program + size;
  }
}

static compiled_code_t make_exec(program_t *prog, dasm_State **state) {
  size_t size;
  int dasm_status = dasm_link(state, &size);
  assert(dasm_status == DASM_S_OK);
  (void) dasm_status;

  ensure_space(prog, size);

  /*  For performance reasons, we care about page RWX access only in
   *  when asserts are enabled.  */
  int result =
      mprotect(prog->codepages, prog->codepages->size, PROT_READ | PROT_WRITE);
  assert(result == 0);
  (void) result;

  compiled_code_t code = (compiled_code_t) prog->begin;
  prog->begin += size;

  dasm_encode(state, code);
  dasm_free(state);

  result = mprotect(prog->codepages, size, PROT_EXEC | PROT_READ);
  assert(result == 0);
  (void) result;

  return code;
}

int compile_and_install(program_t *p, byte *loop) {
  dasm_State *state;
  dasm_init(&state, 1);
  dasm_setup(&state, actions);

  codegen(p, state, loop);

  compiled_code_t code = make_exec(p, &state);

  if (p->compiled_code_len == p->compiled_code_capacity) {
    p->compiled_code =
        realloc(p->compiled_code,
                sizeof(compiled_code_t) * p->compiled_code_capacity * 2);
    p->compiled_code_capacity *= 2;
  }

  p->compiled_code[p->compiled_code_len++] = code;

  uint32_t *patch = (uint32_t *) loop;
  patch[0] = BC_COMPILED_LOOP;
  patch[1] = p->compiled_code_len - 1;

  return p->compiled_code_len - 1;
}

void free_all_compiled_code(program_t *prog) {
  for (codepage_t *cp_i = prog->codepages; cp_i; ) {
    codepage_t *cp_j = cp_i->next;
    munmap(cp_i, cp_i->size);
    cp_i = cp_j;
  }
}
