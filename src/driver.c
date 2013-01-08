#include "bfjit.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

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

static void dispatch(const char *file_name) {
  char *source = read_whole_file(file_name);
  program_t *prog = p_new(source);
  //p_print_bc(stdout, prog);
  p_exec(prog, 30000);
  p_destroy(prog);
}

int main(int argc, char **argv) {
  if (argc != 2) die("usage: %s <source file name>", argv[0]);
  dispatch(argv[1]);
  return 0;
}
