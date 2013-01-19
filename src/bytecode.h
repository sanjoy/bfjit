#ifndef __BYTECODE__H
#define __BYTECODE__H

#include <stdio.h>

#include "utils.h"

/*  A brainfuck script is compiled into a sequence of bytecodes.  A
 *  bytecode stream is a sequence of bytecodes followed by optional
 *  "payloads", a bytecode specific packet of information.  Below,
 *  payload[i] refers to the ith payload, retrievable by
 *  get_payload(pc, i).  */

enum bytecode {
  BC_INVALID,

  BC_SHIFT,  /*  shift the data pointer payload[0] places to the
              *  right.  payload[0] may be negative.  */
  BC_ADD,  /*  add payload[0] to the current cell.  */

  BC_OUTPUT, BC_INPUT,  /*  input and output  */

  BC_LOOP_BEGIN, BC_LOOP_END,
  /* usual brainfuck looping constructs.
   *
   * BC_LOOP_BEGIN:
   *
   * payload[0] contains the index into the heat_counters array that
   *   measures the heat of this loop.
   *
   * payload[1] contains the length of the loop including the pairing
   *   BC_LOOP_END instruction.
   *
   *
   * BC_LOOP_END:
   *
   * payload[0] contains the length of the loop excluding this
   *   BC_LOOP_END instruction.
   */

  BC_COMPILED_LOOP,  /*  a loop that has been compiled to machine
                      *  code.  Such loops are functions of the
                      *  signature compiled_code_t.  */

  BC_HLT, /*  end the brainfuck program  */



  BC_ZERO, /*  optimization bytecode, shortcut for the sequence [-]  */

  BC_NUM_BYTECODES
};

byte *bc_from_source(const char *source, unsigned int *loop_stack,
                     int loop_stack_size);
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
    case BC_ZERO:
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
