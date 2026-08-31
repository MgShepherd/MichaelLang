#include "parsing/identifier.h"

#include <string.h>

const Identifier *get_identifier(const Identifiers *identifiers, const Token *token) {
  for (size_t i = 0; i < identifiers->count; i++) {
    if (strcmp(identifiers->elements[i].name, token->item) == 0) {
      return &identifiers->elements[i];
    }
  }
  return NULL;
}
