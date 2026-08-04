#include "lexer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define EXAMPLE_FILE_NAME "examples/basic.mgs"

int main() {
  FileData file_data;
  if (read_file(&file_data, EXAMPLE_FILE_NAME) != 0) {
    fprintf(stderr, "Failed to read file " EXAMPLE_FILE_NAME "\n");
    return 1;
  }

  TokenArray token_arr;
  if (lexer_process_tokens(&token_arr, &file_data) != 0) {
    fprintf(stderr, "Failed to convert file input into tokens\n");
    file_data_free(&file_data);
    return 1;
  }

  file_data_free(&file_data);

  token_array_free(&token_arr);

  return 0;
}
