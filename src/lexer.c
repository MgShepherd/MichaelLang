#include "lexer.h"
#include "dynamic_array.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_ARRAY_LEN_FACTOR 0.6
#define CONTINUE_PROCESSING_STATUS 2

unsigned char get_next_token(Token *tok, const char *input, size_t *i);
/*
 * All processing functions return following status codes:
 * 0 - Indicates the processing was successful
 * 1 - Indicates there was an error when processing
 * 2 - Indicates token is not the processed type, processing should continue
 */
unsigned char process_as_keyword(Token *tok, const char *input, size_t *idx);
unsigned char process_as_symbol(Token *tok, const char *input, size_t *idx);
unsigned char process_as_identifier(Token *tok, const char *input, size_t *idx);
unsigned char process_as_numeric_lit(Token *tok, const char *input, size_t *idx);

bool next_token_matches(const char *compare, const char *input, size_t idx);

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

unsigned char lexer_process_tokens(Tokens *tokens, const char *data) {
  assert(data != NULL);

  tokens->elements = NULL;
  const size_t data_len = strlen(data);
  // If file is empty, valid program but no further processing is needed
  if (data_len == 0) {
    return 0;
  }

  size_t capacity = data_len * TOKEN_ARRAY_LEN_FACTOR;
  // Guard against getting 0 capacity when token input size is very small
  if (capacity == 0) {
    capacity = 1;
  }

  dyn_array_init(tokens, sizeof(Token), capacity);

  size_t idx = 0;
  while (idx < data_len) {
    if (isspace(data[idx])) {
      idx++;
      continue;
    }

    Token tok;
    if (get_next_token(&tok, data, &idx) != 0) {
      fprintf(stderr, "Failed to get next token starting from character: %c\n", data[idx]);
      return 1;
    }

    dyn_array_insert(tokens, tok);
  }

  return 0;
}

unsigned char get_next_token(Token *tok, const char *input, size_t *idx) {
  tok->t_type = T_NONE;

  unsigned char result = process_as_keyword(tok, input, idx);
  if (result != CONTINUE_PROCESSING_STATUS) {
    return result;
  }

  result = process_as_symbol(tok, input, idx);
  if (result != CONTINUE_PROCESSING_STATUS) {
    return result;
  }

  result = process_as_identifier(tok, input, idx);
  if (result != CONTINUE_PROCESSING_STATUS) {
    return result;
  }

  result = process_as_numeric_lit(tok, input, idx);
  if (result != CONTINUE_PROCESSING_STATUS) {
    return result;
  }

  return 1;
}

unsigned char process_as_keyword(Token *tok, const char *input, size_t *idx) {
  static const char *KEYWORDS[] = {"func", "return", "var"};

  for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(char *); i++) {
    if (!next_token_matches(KEYWORDS[i], input, *idx)) {
      continue;
    }

    const size_t keyword_len = strlen(KEYWORDS[i]);
    // Note: For now all keywords must end with a space (or end of input), eventually may be able to end with
    // parentheses
    if (*idx + keyword_len < strlen(input) && !isspace(input[*idx + keyword_len])) {
      continue;
    }

    char *out_buf = malloc(keyword_len + 1);
    if (out_buf == NULL) {
      fprintf(stderr, "Failed to allocate require memory for processing keyword\n");
      return 1;
    }
    strcpy(out_buf, KEYWORDS[i]);

    tok->item = out_buf;
    tok->t_type = T_KEYWORD;

    // Increment idx so we are now pointing at the element after the space
    *idx += keyword_len + 1;

    return 0;
  }

  return CONTINUE_PROCESSING_STATUS;
}

unsigned char process_as_symbol(Token *tok, const char *input, size_t *idx) {
  static const TokenMapping SYMBOL_MAPPINGS[] = {
      {.key = "(", .value = T_LEFT_PAREN},  {.key = ")", .value = T_RIGHT_PAREN}, {.key = "{", .value = T_LEFT_CURLY},
      {.key = "}", .value = T_RIGHT_CURLY}, {.key = "->", .value = T_ARROW},      {.key = "i32", .value = T_DATATYPE},
      {.key = ":", .value = T_COLON},       {.key = "=", .value = T_EQUALS},      {.key = ";", .value = T_SEMI},
      {.key = "+", .value = T_ARITHMETIC},  {.key = "-", .value = T_ARITHMETIC},
  };

  for (size_t i = 0; i < sizeof(SYMBOL_MAPPINGS) / sizeof(TokenMapping); i++) {
    if (!next_token_matches(SYMBOL_MAPPINGS[i].key, input, *idx)) {
      continue;
    }

    const size_t symbol_len = strlen(SYMBOL_MAPPINGS[i].key);
    char *out_buf = malloc(symbol_len + 1);
    if (out_buf == NULL) {
      fprintf(stderr, "Failed to allocate memory for symbol processing\n");
      return 1;
    }
    strcpy(out_buf, SYMBOL_MAPPINGS[i].key);

    tok->item = out_buf;
    tok->t_type = SYMBOL_MAPPINGS[i].value;

    // Increment idx so we are now pointer at the element directly after the symbol
    *idx += symbol_len;

    return 0;
  }

  return CONTINUE_PROCESSING_STATUS;
}

unsigned char process_as_identifier(Token *tok, const char *input, size_t *idx) {
  const size_t input_len = strlen(input);
  if (*idx >= strlen(input)) {
    return CONTINUE_PROCESSING_STATUS;
  }

  size_t i = *idx;
  if (input[i] != '_' && !isalpha(input[i])) {
    return CONTINUE_PROCESSING_STATUS;
  }
  i++;

  while (i < input_len) {
    if (input[i] != '_' && !isalpha(input[i]) && !isdigit(input[i])) {
      break;
    }
    i++;
  }

  tok->item = string_slice(input, *idx, i);
  if (tok->item == NULL) {
    fprintf(stderr, "Failed to allocate memory for identifier processing\n");
    return 1;
  }
  tok->t_type = T_IDENTIFIER;

  *idx = i;

  return 0;
}

unsigned char process_as_numeric_lit(Token *tok, const char *input, size_t *idx) {
  size_t i = *idx;

  if (!isdigit(input[i++])) {
    return CONTINUE_PROCESSING_STATUS;
  }

  while (i < strlen(input)) {
    if (!isdigit(input[i])) {
      break;
    }
    i++;
  }

  tok->item = string_slice(input, *idx, i);
  if (tok->item == NULL) {
    fprintf(stderr, "Failed to allocate memory for numeric literal processing\n");
    return 1;
  }
  tok->t_type = T_NUMERIC_LIT;

  *idx = i;

  return 0;
}

bool next_token_matches(const char *compare, const char *input, size_t idx) {
  size_t i = 0;
  const size_t input_len = strlen(input);
  while (i < strlen(compare)) {
    if (idx + i >= input_len || input[idx + i] != compare[i]) {
      return false;
    }
    i++;
  }

  return true;
}
