.PHONY: all clean

CFLAGS=-O3 -Wall -Werror -g -std=gnu99 -DNDEBUG -I./
HEADERS=src/bfjit.h src/utils.h src/compiler.h src/interpreter.h src/bytecode.h \
	codegen.inc

all:: bf

bf: bfjit.o utils.o driver.o compiler.o interpreter.o bytecode.o
	$(CC) $^ -o $@

%.o: src/%.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

codegen.inc: src/codegen.dasc
	lua dynasm/dynasm.lua src/codegen.dasc > codegen.inc

clean:
	$(RM) *.o
	$(RM) codegen.inc
	$(RM) bf
