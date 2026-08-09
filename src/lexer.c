#include "lexer.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_ARRAY_LEN_FACTOR 0.6
#define TOKEN_ARRAY_REALLOC_FACTOR 2

unsigned char insert_token(TokenArray *token_arr, const char *input, size_t token_start, size_t token_end);
TokenType get_token_type(const char *token);
bool is_separator_token(char current);
bool is_valid_identifier(const char *token);
bool is_numeric_literal(const char *token);

#define X(N)                                                                                                           \
  case N:                                                                                                              \
    return #N;

const char *t_type_to_string(TokenType t) {
  switch (t) {
    TOKEN_TYPES
  default:
    return "unknown";
  }
}
#undef X

typedef struct {
  const char *key;
  TokenType value;
} TokenMapping;

static const TokenMapping mappings[] = {
    {.key = "func", .value = T_KEYWORD},
    {.key = "i32", .value = T_KEYWORD},
    {.key = "return", .value = T_KEYWORD},
    {.key = "->", .value = T_ARROW},
};

static const size_t NUM_TOKEN_MAPPINGS = (sizeof(mappings) / sizeof(mappings[0]));

unsigned char lexer_process_tokens(TokenArray *token_arr, const char *data) {
  assert(data != NULL);

  token_arr->tokens = NULL;
  const size_t data_len = strlen(data);
  // If file is empty, valid program but no further processing is needed
  if (data_len == 0) {
    return 0;
  }

  token_arr->capacity = data_len * TOKEN_ARRAY_LEN_FACTOR;
  // Guard against getting 0 capacity when token input size is very small
  if (token_arr->capacity == 0) {
    token_arr->capacity = 1;
  }

  token_arr->count = 0;
  token_arr->tokens = malloc(token_arr->capacity * sizeof(Token));

  if (token_arr->tokens == NULL) {
    fprintf(stderr, "Failed to allocate required space for tokens array\n");
    return 1;
  }

  size_t token_start = 0;
  for (size_t i = 0; i < data_len; i++) {
    const bool is_separator = is_separator_token(data[i]);

    if (isspace(data[i]) > 0 || is_separator) {
      if (insert_token(token_arr, data, token_start, i) != 0) {
        fprintf(stderr, "Failed to insert token into tokens array\n");
        token_array_free(token_arr);
        return 1;
      }
      token_start = i + 1;
    }

    // If the current char is a separator, insert another token for the separator itself
    if (is_separator && insert_token(token_arr, data, i, i + 1) != 0) {
      fprintf(stderr, "Failed to insert token into tokens array\n");
      token_array_free(token_arr);
      return 1;
    }
  }

  if (token_start < data_len && insert_token(token_arr, data, token_start, data_len) != 0) {
    fprintf(stderr, "Failed to insert token into tokens array\n");
    token_array_free(token_arr);
    return 1;
  }

  return 0;
}

void token_array_free(TokenArray *token_arr) {
  if (token_arr != NULL && token_arr->tokens != NULL) {
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
  assert(token_arr->tokens != NULL);
  // If the token is empty, don't insert it but continue with the program
  if (token_end == 0 || token_start >= token_end) {
    return 0;
  }

  if (token_arr->count >= token_arr->capacity) {
    token_arr->capacity = token_arr->capacity * TOKEN_ARRAY_REALLOC_FACTOR;
    Token *new_tokens = realloc(token_arr->tokens, token_arr->capacity * sizeof(Token));
    if (new_tokens == NULL) {
      fprintf(stderr, "Failed to allocate additional required space for tokens array\n");
      return 1;
    }
    token_arr->tokens = new_tokens;
  }

  token_arr->tokens[token_arr->count].item = string_slice(input, token_start, token_end);
  if (token_arr->tokens[token_arr->count].item == NULL) {
    fprintf(stderr, "Failed to create token string for token starting at position %zu, ending at %zu\n", token_start,
            token_end);
    return 1;
  }
  token_arr->tokens[token_arr->count].t_type = get_token_type(token_arr->tokens[token_arr->count].item);
  if (token_arr->tokens[token_arr->count].t_type == T_NONE) {
    fprintf(stderr, "Failed to process token type for token: %s\n", token_arr->tokens[token_arr->count].item);
    free(token_arr->tokens[token_arr->count].item);
    return 1;
  }
  token_arr->count++;

  return 0;
}

TokenType get_token_type(const char *token) {
  switch (token[0]) {
  case '(':
    return T_LEFT_PAREN;
  case ')':
    return T_RIGHT_PAREN;
  case '{':
    return T_LEFT_CURLY;
  case '}':
    return T_RIGHT_CURLY;
  case '=':
    return T_EQUALS;
  case ';':
    return T_SEMI;
  case ':':
    return T_COLON;
  }

  for (size_t i = 0; i < NUM_TOKEN_MAPPINGS; i++) {
    if (strcmp(mappings[i].key, token) == 0) {
      return mappings[i].value;
    }
  }

  if (is_valid_identifier(token)) {
    return T_IDENTIFIER;
  }

  if (is_numeric_literal(token)) {
    return T_NUMERIC_LIT;
  }

  return T_NONE;
}

bool is_valid_identifier(const char *token) {
  assert(token[0] != '\0');
  if (token[0] != '_' && !isalpha(token[0])) {
    return false;
  }

  size_t i = 1;
  while (token[i] != '\0') {
    if (token[i] != '_' && !isalpha(token[i]) && !isdigit(token[i])) {
      return false;
    }
    i++;
  }

  return true;
}

bool is_numeric_literal(const char *token) {
  assert(token[0] != '\0');
  size_t i = 0;
  while (token[i] != '\0') {
    if (!isdigit(token[i])) {
      return false;
    }
    i++;
  }

  return true;
}

bool is_separator_token(char current) {
  return current == ':' || current == '{' || current == '}' || current == '(' || current == ')' || current == ';' ||
         current == '=';
}
