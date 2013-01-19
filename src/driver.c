#include "bfjit.h"
#include "bytecode.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_whole_file(const char *file_name) {
  FILE *fptr = fopen(file_name, "r");
  if (!fptr) die("could not open source file `%s'", file_name);

  fseek(fptr, 0, SEEK_END);
  size_t file_size = ftell(fptr);
  fseek(fptr, 0, SEEK_SET);

  char *file_contents = malloc(file_size + 1);
  file_contents[file_size] = 0;

  size_t result = fread(file_contents, 1, file_size, fptr);
  if (result != file_size) {
    die("could not read source file `%s' completely", file_name);
  }

  fclose(fptr);
  return file_contents;
}

static void dispatch(const char *file_name, int print_only) {
  char *source = read_whole_file(file_name);
  program_t *prog = p_new(source, 1024);
  if (print_only) {
    bc_dump(stdout, prog->bytecode);
  } else {
    p_exec(prog, 30000);
  }
  p_destroy(prog);
}

int main(int argc, char **argv) {
  if (argc == 1) die("usage: %s [--print-bc-only] <source file name>", argv[0]);
  if (argc == 2) {
    dispatch(argv[1], 0);
  } else {
    dispatch(argv[2], !strcmp(argv[1], "--print-bc-only"));
  }
  return 0;
}
