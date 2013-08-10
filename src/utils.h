#ifndef __UTILS__H
#define __UTILS__H

#include <stdint.h>

typedef uint8_t byte;
typedef uint_fast8_t bool;

#define true ((bool) 1)
#define false ((bool) 0)

#define unlikely(x) (__builtin_expect(x, false))
#define likely(x) (__builtin_expect(x, true))
#define always_inline(fn_decl) inline fn_decl __attribute__((always_inline))

void die(const char *format, ...) __attribute__((noreturn));

#endif
