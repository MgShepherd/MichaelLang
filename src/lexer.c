#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

#define TOKEN_ARRAY_LEN_FACTOR 0.6

unsigned char lexer_process_tokens(TokenArray *token_arr, const FileData *data) {
  token_arr->capacity = data->count * TOKEN_ARRAY_LEN_FACTOR;
  token_arr->count = 0;
  token_arr->tokens = malloc(token_arr->capacity);

  if (token_arr->tokens == NULL) {
    fprintf(stderr, "Failed to allocate required space for tokens array\n");
    return 1;
  }

  printf("Successfully allocated tokens array\n");

  return 0;
}

void token_array_free(TokenArray *token_arr) {
  if (token_arr->tokens != NULL) {
    for (size_t i = 0; i < token_arr->count; i++) {
      free(token_arr->tokens[i].item);
    }
    free(token_arr->tokens);
  }

  token_arr->tokens = NULL;
  token_arr->count = 0;
  token_arr->capacity = 0;
}
