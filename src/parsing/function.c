#include "parsing/function.h"

#include "dynamic_array.h"
#include "parsing/statement.h"
#include "parsing/utils.h"
#include <stdio.h>

#define FUNCTION_STATEMENTS_LEN_ESTIMATE 10
#define FUNCTION_VARIABLES_LEN_ESTIMATE 10

unsigned char parse_function(Function *function, const Tokens *tokens, size_t *idx);

unsigned char parse_functions(Functions *functions, const Tokens *tokens) {
  size_t idx = 0;
  while (idx < tokens->count) {
    if (tokens->elements[idx].t_type != T_FUNCTION) {
      fprintf(stderr, "Expected func token for function begin, got %s\n",
              t_type_to_string(tokens->elements[idx].t_type));
      return 1;
    }

    Function func;
    if (parse_function(&func, tokens, &idx) != 0) {
      fprintf(stderr, "Failed to parse function\n");
      return 1;
    }

    dyn_array_insert(functions, func);
  }

  return 0;
}

void functions_free(Functions *functions) {
  for (size_t i = 0; i < functions->count; i++) {
    statements_free(&functions->elements[i].statements);
    dyn_array_free(&functions->elements[i].identifiers);
  }
  dyn_array_free(functions);
}

unsigned char parse_function(Function *function, const Tokens *tokens, size_t *idx) {
  if (expect_next(T_FUNCTION, tokens, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for function\n");
    return 1;
  }

  const Token *ident_tok = expect_next(T_IDENTIFIER, tokens, idx);
  if (ident_tok == NULL) {
    fprintf(stderr, "Failed to get identifier for function\n");
    return 1;
  }
  function->name = ident_tok->item;

  if (expect_next(T_LEFT_PAREN, tokens, idx) == NULL) {
    fprintf(stderr, "Expected opening parenthesis in function\n");
    return 1;
  }

  if (expect_next(T_RIGHT_PAREN, tokens, idx) == NULL) {
    fprintf(stderr, "Expected closing parenthesis in function\n");
    return 1;
  }

  if (expect_next(T_ARROW, tokens, idx) == NULL) {
    fprintf(stderr, "Expected arrow in function\n");
    return 1;
  }

  function->return_type = tok_to_data_type(tokens->elements[(*idx)++].t_type);
  if (function->return_type == D_NONE) {
    fprintf(stderr, "Expected return type for function to be valid data type\n");
    return 1;
  }

  if (expect_next(T_LEFT_CURLY, tokens, idx) == NULL) {
    fprintf(stderr, "Expected opening curly brace before function body\n");
    return 1;
  }

  Statements statements;
  dyn_array_init(&statements, sizeof(Statement), FUNCTION_STATEMENTS_LEN_ESTIMATE);

  Identifiers identifiers;
  dyn_array_init(&identifiers, sizeof(Identifier), FUNCTION_VARIABLES_LEN_ESTIMATE);

  if (parse_statements(&statements, &identifiers, tokens, idx, T_RIGHT_CURLY) != 0) {
    fprintf(stderr, "Failed to process function body\n");
    dyn_array_free(&statements);
    dyn_array_free(&identifiers);
    return 1;
  }
  function->statements = statements;
  function->identifiers = identifiers;

  if (expect_next(T_RIGHT_CURLY, tokens, idx) == NULL) {
    fprintf(stderr, "Expected closing curly brace after function body\n");
    dyn_array_free(&statements);
    dyn_array_free(&identifiers);
    return 1;
  }

  return 0;
}
