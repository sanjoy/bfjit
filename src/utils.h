#ifndef __UTILS__H
#define __UTILS__H

#include <stdint.h>

typedef uint8_t byte;

#define unlikely(x) (__builtin_expect(x, 0))
#define likely(x) (__builtin_expect(x, 1))
#define always_inline(fn_decl) inline fn_decl __attribute__((always_inline))

void die(const char *format, ...) __attribute__((noreturn));

#endif
