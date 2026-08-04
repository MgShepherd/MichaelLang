#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdlib.h>

#include "utils.h"

typedef enum { IDENTIFIER } TokenType;

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
unsigned char lexer_process_tokens(TokenArray *token_arr, const FileData *data);

/*
 * Frees the memory associated with the token array object
 */
void token_array_free(TokenArray *token_arr);

#endif // _LEXER_H_
