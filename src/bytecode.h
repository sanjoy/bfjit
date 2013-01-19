#ifndef __BYTECODE__H
#define __BYTECODE__H

#include <stdio.h>

#include "utils.h"

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

byte *bc_from_source(const char *source, int loop_nest_limit);
void bc_dump(FILE *fptr, byte *bc);

#define kByteCodeLen 4
#define kPayloadLen 4

static always_inline(int get_bytecode(byte *pc));
static always_inline(uint32_t get_payload(byte *pc, int payload_index));
static always_inline(int get_total_length(int bc));

static int get_bytecode(byte *pc) {
  return *pc;
}

static uint32_t get_payload(byte *pc, int payload_index) {
  uint32_t *buffer = (uint32_t *) (pc + kByteCodeLen);
  return buffer[payload_index];
}

static int get_total_length(int bc) {
  switch (bc) {
    case BC_OUTPUT:
    case BC_INPUT:
    case BC_HLT:
      return kByteCodeLen;

    case BC_SHIFT:
    case BC_ADD:
    case BC_LOOP_END:
      return kByteCodeLen + kPayloadLen;

    case BC_COMPILED_LOOP:
    case BC_LOOP_BEGIN:
      return kByteCodeLen + 2 * kPayloadLen;
  }
  return -1;
}

#endif
