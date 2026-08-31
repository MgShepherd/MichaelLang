#include "parsing/program.h"

#include "dynamic_array.h"
#include <assert.h>
#include <stdio.h>

#define FUNCTION_ARRAY_LEN_FACTOR 0.1

unsigned char parse_program(Program *program, const Tokens *tokens) {
  assert(tokens != NULL && tokens->count > 0);
  program->functions.elements = NULL;

  size_t capacity = tokens->count * FUNCTION_ARRAY_LEN_FACTOR;
  // Guard against getting 0 capacity when token input size is very small
  if (capacity == 0) {
    capacity = 1;
  }
  Functions functions;
  dyn_array_init(&functions, sizeof(Function), capacity);

  if (parse_functions(&functions, tokens) != 0) {
    fprintf(stderr, "Failed to parse functions\n");
    functions_free(&functions);

    return 1;
  }

  program->functions = functions;

  return 0;
}

void program_free(Program *program) {
  if (program != NULL && program->functions.elements != NULL) {
    functions_free(&program->functions);
  }
}
