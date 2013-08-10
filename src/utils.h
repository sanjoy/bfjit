#ifndef __UTILS__H
#define __UTILS__H

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t byte;
typedef uint_fast8_t bool;

#define true ((bool) 1)
#define false ((bool) 0)

#define unlikely(x) (__builtin_expect(x, false))
#define likely(x) (__builtin_expect(x, true))
#define always_inline(fn_decl) inline fn_decl __attribute__((always_inline))

void die(const char *format, ...) __attribute__((noreturn));

always_inline(static void *alloc(size_t size));

static void *alloc(size_t size) {
  void *mem = calloc(size, 1);
  if (unlikely(mem == NULL)) die("out of memory!");
  return mem;
}

#endif
