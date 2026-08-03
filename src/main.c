#include "lexer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define EXAMPLE_FILE_NAME "examples/basic.mgs"

int main() {
  char *data = read_file(EXAMPLE_FILE_NAME);
  if (data == NULL) {
    fprintf(stderr, "Failed to read file " EXAMPLE_FILE_NAME "\n");
    return -1;
  }

  lex(data);
  free(data);
  return 0;
}
