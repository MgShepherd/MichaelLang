#ifndef _PARSING_IDENTIFIER_H_
#define _PARSING_IDENTIFIER_H_

#include "lexer.h"
#include "parsing/type.h"
#include <stdlib.h>

typedef struct {
  char *name;
  DataType d_type;
  bool variable;
} Identifier;

typedef struct {
  Identifier *elements;
  size_t count;
  size_t capacity;
} Identifiers;

/*
 * Checks whether an identifier exists for the provided token and will return it if it does
 * Returns NULL if the identifier does not exist
 */
const Identifier *get_identifier(const Identifiers *identifiers, const Token *token);

#endif // _PARSING_IDENTIFIER_H_
