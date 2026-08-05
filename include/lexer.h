#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdlib.h>

#include "utils.h"

// Using X-Macros to create TokenType enum with a generated t_type_to_string method
#define TOKEN_TYPES                                                                                                    \
  X(T_INVALID)                                                                                                         \
  X(T_IDENTIFIER)                                                                                                      \
  X(T_NUMERIC_LIT)                                                                                                     \
  X(T_KEYWORD)                                                                                                         \
  X(T_ARROW)                                                                                                           \
  X(T_LEFT_PAREN)                                                                                                      \
  X(T_RIGHT_PAREN)                                                                                                     \
  X(T_LEFT_CURLY)                                                                                                      \
  X(T_RIGHT_CURLY)                                                                                                     \
  X(T_EQUALS)                                                                                                          \
  X(T_COLON)                                                                                                           \
  X(T_SEMI)

#define X(N) N,
typedef enum { TOKEN_TYPES } TokenType;
#undef X

const char *t_type_to_string(TokenType t);

typedef struct {
  TokenType t_type;
  char *item;
} Token;

typedef struct {
  Token *tokens;
  size_t count;
  size_t capacity;
} TokenArray;

/*
 * Converts the file_data into a array of tokens, output into the tokens parameter
 * Expects TokenArray to be an uninitialised object, if not any data should be freed before this call
 * Will return 0 on success, 1 on failure (alongside logging the failure cause to stderr)
 * Note: Once TokenArray has been created, data parameter can be freed as string copies are made for each element
 */
unsigned char lexer_process_tokens(TokenArray *token_arr, const FileData *file_data);

/*
 * Frees the memory associated with the token array object
 */
void token_array_free(TokenArray *token_arr);

#endif // _LEXER_H_
