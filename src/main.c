#include "lexer.h"
#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define EXAMPLE_FILE_NAME "examples/basic.mgs"

// TODO: Test with empty file - most places are expecting file length greater than 0, but we should handle empty file
// gracefully

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

  Program program;
  if (parse_tokens(&program, &token_arr) != 0) {
    fprintf(stderr, "Failed to convert processed tokens into a program\n");
    token_array_free(&token_arr);
    return 1;
  }

  printf("Program has %zu statements\n", program.statement_arr.count);
  for (size_t i = 0; i < program.statement_arr.count; i++) {
    printf("Statement has type: %s\n", s_type_to_string(program.statement_arr.statements[i].s_type));
    switch (program.statement_arr.statements[i].s_type) {
    case S_DECLARATION:
      printf("Declaration Statement: Identifier: %s, Data Type: %s, Value: %s\n",
             program.statement_arr.statements[i].s_union.dec.identifier->item,
             program.statement_arr.statements[i].s_union.dec.data_type->item,
             program.statement_arr.statements[i].s_union.dec.value->item);
      break;
    default:
      printf("Unexpected statement type %s\n", s_type_to_string(program.statement_arr.statements[i].s_type));
    }
  }

  program_free(&program);
  token_array_free(&token_arr);

  return 0;
}
