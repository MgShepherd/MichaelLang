#include "lexer.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define TOKEN_ARRAY_LEN_FACTOR 0.6
#define TOKEN_ARRAY_REALLOC_FACTOR 1.5

unsigned char insert_token(TokenArray *token_arr, const char *input, size_t token_start, size_t token_end);

unsigned char lexer_process_tokens(TokenArray *token_arr, const FileData *file_data) {
  assert(file_data->count > 0 && file_data->data != NULL);

  token_arr->capacity = file_data->count * TOKEN_ARRAY_LEN_FACTOR;
  token_arr->count = 0;
  token_arr->tokens = malloc(token_arr->capacity);

  if (token_arr->tokens == NULL) {
    fprintf(stderr, "Failed to allocate required space for tokens array\n");
    return 1;
  }

  size_t token_start = 0;
  for (size_t i = 0; i < file_data->count; i++) {
    if (isspace(file_data->data[i]) > 0) {
      if (insert_token(token_arr, file_data->data, token_start, i) != 0) {
        fprintf(stderr, "Failed to insert token into tokens array\n");
        token_array_free(token_arr);
        return 1;
      }
      token_start = i + 1;
    }
  }

  for (size_t i = 0; i < token_arr->count; i++) {
    printf("Processed Token with Value: [%s]\n", token_arr->tokens[i].item);
  }

  return 0;
}

void token_array_free(TokenArray *token_arr) {
  if (token_arr->tokens != NULL) {
    // TODO: Check that this doesn't need to be <=
    for (size_t i = 0; i < token_arr->count; i++) {
      free(token_arr->tokens[i].item);
    }
    free(token_arr->tokens);
  }

  token_arr->tokens = NULL;
  token_arr->count = 0;
  token_arr->capacity = 0;
}

unsigned char insert_token(TokenArray *token_arr, const char *input, size_t token_start, size_t token_end) {
  // If the token is empty, don't insert it but continue with the program
  if (token_end == 0 || token_start >= token_end) {
    return 0;
  }

  if (token_arr->count >= token_arr->capacity) {
    token_arr->capacity = token_arr->capacity * TOKEN_ARRAY_REALLOC_FACTOR;
    token_arr->tokens = realloc(token_arr->tokens, token_arr->capacity);
    if (token_arr->tokens == NULL) {
      fprintf(stderr, "Failed to allocate additional required space for tokens array");
      return 1;
    }
  }

  token_arr->tokens[token_arr->count].item = string_slice(input, token_start, token_end);
  if (token_arr->tokens[token_arr->count].item == NULL) {
    fprintf(stderr, "Failed to create token string for token starting at position %zu, ending at %zu\n", token_start,
            token_end);
    return 1;
  }
  // TODO: Actually work out real token type, rather than just always setting to identifier
  token_arr->tokens[token_arr->count++].t_type = T_IDENTIFIER;

  return 0;
}
