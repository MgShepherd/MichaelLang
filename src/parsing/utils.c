#include "parsing/utils.h"

#include <string.h>

const Token *expect_next(TokenType expected, const Tokens *tokens, size_t *idx) {
  if (*idx >= tokens->count) {
    return NULL;
  }

  const Token *next = &tokens->elements[*idx];
  if (next->t_type != expected) {
    return NULL;
  }

  *idx += 1;
  return next;
}

const Token *expect_keyword(const char *keyword, const Tokens *tokens, size_t *idx) {
  const Token *keyword_tok = expect_next(T_KEYWORD, tokens, idx);
  if (keyword_tok == NULL) {
    return NULL;
  }

  if (strcmp(keyword_tok->item, keyword) != 0) {
    return NULL;
  }

  return keyword_tok;
}
