BFJIT
=====

A high-performance VM for the emerging language, brainf*ck.  The goal
is to have a (fast) interpreter which then JIT compiles hot loops.

The Bytecode
------------

Neither the interpreter nor the JIT understands BF source; instead
they work with a bytecode based IR. The `bc_from_source` function in
bytecode.c parses BF source and generates a stream of bytecodes
representing it. The term bytecode is somewhat misleading since a
bytecode is four bytes long, usually followed by one or more four-byte
payloads.

Translating to bytecode is important for performance. Individual BF
instructions do very little and sequences like `+++` can be quickly
executed at one go (by increasing the current cell’s value by `3`, in
this case) than by three separate bytecode dispatches. To this end,
`bc_from_source` attempts to do some basic peephole optimizations by
recognizing common BF idioms. For instance, one way to add the
contents of the current cell to a cell three indices away (and clear
the current cell in the process) is `[->>>+<<<]`. `bc_from_source`
recognizes this pattern and emits a specialized bytecode,
`BC_MOVE_VALUE`, which is specially handled by the interpreter and the
JIT.

The Interpreter
---------------

The interpreter is quite simplistic. It uses GCC's labels as values
feature to provide for a quick dispatch mechanism. Every loop has a
heat counter associated with it, whose numerical value is stored as
payload following the `BC_LOOP_BEGIN` bytecode. The interpreter checks
this value and calls in the JIT once a method is hot. The JIT does its
thing, and stores away a pointer to the generated code in the
`program_t` structure. The interpreter then calls into this generated
code. The compiler also patches the `BC_LOOP_BEGIN` to a
`BC_COMPILED_LOOP` so that the next iteration directly executes the
JITed version of the loop.

The Compiler
------------

The compiler uses dynasm to generate a code-generator; and as a
consequence most of the compiler really lives in `codegen.dasc`. This
might change if I later add some analysis or optimization passes which
run only when compiling the bytecode, but for now the codegen
practically is the compiler. The only compiler specific optimization
is that the compiler tries to not emit a redundant `cmp` instruction
if the last instruction makes it unnecessary (by virtue of being an
add instruction with a negative constant as one operand).

Performance
-----------

Running `mandlebrot.bf` takes 1.25 seconds on average, which is at par
with compiling BF to C and then compiling the resultant C file with
`gcc -O3`. While I’d call this pretty good, I think there is still
some room for improvement, perhaps studying the machine code generated
for typical BF programs will yield some insight.

Future Work
-----------

There are two improvements I can think of, believe will improve
performance, but haven’t had time to implement:

 * A register allocator should improve performance, but probably not
as much as in normal programming languages -- BF is a very cache
friendly language. :)

 * Some basic auto-vectorization. Maybe we can say "this loop simply
adds these two blocks of cells together, one by one" and emit some SSE
instructions to that effect.
