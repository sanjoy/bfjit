#ifndef __UTILS__H
#define __UTILS__H

#define unlikely(x) (__builtin_expect(x, 0))
#define likely(x) (__builtin_expect(x, 1))

void die(const char *format, ...) __attribute__((noreturn));

#endif
