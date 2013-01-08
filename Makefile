CFLAGS=-O3 -g -std=gnu99

all:: bf

bf: bfjit.o utils.o driver.o
	$(CC) $^ -o $@

%.o: src/%.c src/bfjit.h src/utils.h
	$(CC) -c $(CFLAGS) $< -o $@
