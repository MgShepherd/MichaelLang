#include "lexer.h"
#include "llvm.h"
#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

// TODO: Replace hardcoded file with argument
#define EXAMPLE_FOLDER "tests/00 - Basic/"
#define BUILD_FOLDER "build/"
#define EXAMPLE_FILE_NAME "input"
#define SRC_EXTENSION ".mgs"
#define OBJ_EXTENSION ".o"

int main() {
  char *data = read_file(EXAMPLE_FOLDER EXAMPLE_FILE_NAME SRC_EXTENSION);
  if (data == NULL) {
    fprintf(stderr, "Failed to read file " EXAMPLE_FILE_NAME "\n");
    return 1;
  }

  TokenArray token_arr;
  if (lexer_process_tokens(&token_arr, data) != 0) {
    fprintf(stderr, "Failed to convert file input into tokens\n");
    free(data);
    return 1;
  }

  free(data);

  Program program;
  if (parse_tokens(&program, &token_arr) != 0) {
    fprintf(stderr, "Failed to convert processed tokens into a program\n");
    token_array_free(&token_arr);
    return 1;
  }

  const char *obj_file = BUILD_FOLDER EXAMPLE_FILE_NAME OBJ_EXTENSION;
  if (program_to_object_file(&program, obj_file) != 0) {
    fprintf(stderr, "Failed to build LLVM IR from program AST\n");
    program_free(&program);
    token_array_free(&token_arr);
    return 1;
  }

  if (obj_to_executable(obj_file, BUILD_FOLDER EXAMPLE_FILE_NAME) != 0) {
    fprintf(stderr, "Failed to generate native executable from obj\n");
    program_free(&program);
    token_array_free(&token_arr);
    return 1;
  }

  printf("Compilation succeeded\n");
  program_free(&program);
  token_array_free(&token_arr);

  return 0;
}
