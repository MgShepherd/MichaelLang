#include "parsing/expression.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

unsigned char parse_terminal_expr(TerminalExpr *term, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx);

unsigned char parse_expression(Expression *expression, const Tokens *tokens, const Identifiers *identifiers,
                               size_t *idx) {
  if (*idx >= tokens->count) {
    fprintf(stderr, "Attempted to get value token but reached end of input\n");
    return 1;
  }

  TerminalExpr term;
  if (parse_terminal_expr(&term, tokens, identifiers, idx) != 0) {
    return 1;
  }

  if (*idx >= tokens->count || tokens->elements[*idx].t_type != T_ARITHMETIC) {
    expression->e_type = E_TERM;
    expression->e_union.terminal = term;
    return 0;
  }

  const Token *op = &tokens->elements[(*idx)++];
  assert(op->t_type == T_ARITHMETIC);
  Expression rhs;
  if (parse_expression(&rhs, tokens, identifiers, idx) != 0) {
    return 1;
  }

  Expression *rhs_ptr = malloc(sizeof(Expression));
  if (rhs_ptr == NULL) {
    fprintf(stderr, "Failed to allocate memory for Right hand side of expression\n");
    return 1;
  }
  *rhs_ptr = rhs;

  ArithmeticExpr arith = {
      .lhs = term,
      .op = op,
      .rhs = rhs_ptr,
  };

  expression->e_type = E_ARITHMETIC;
  expression->e_union.arithmetic = arith;
  return 0;
}

void expression_free(Expression *expression) {
  switch (expression->e_type) {
  case E_ARITHMETIC:
    expression_free(expression->e_union.arithmetic.rhs);
    free(expression->e_union.arithmetic.rhs);
  default:
    break;
  }
}

unsigned char parse_terminal_expr(TerminalExpr *term, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx) {
  const Token *next = &tokens->elements[(*idx)++];

  if (next->t_type == T_ARITHMETIC) {
    if (strcmp(next->item, "+") == 0) {
      term->sign = SIGN_POSTIIVE;
    } else if (strcmp(next->item, "-") == 0) {
      term->sign = SIGN_NEGATIVE;
    } else {
      fprintf(stderr, "Invalid sign for terminal expression: %s\n", next->item);
      return 1;
    }

    next = &tokens->elements[(*idx)++];
  } else {
    term->sign = SIGN_POSTIIVE;
  }

  if (next->t_type == T_IDENTIFIER) {
    if (get_identifier(identifiers, next) == NULL) {
      fprintf(stderr, "Undefined variable: %s\n", next->item);
      return 1;
    }

    term->tok = next;
    return 0;
  }

  if (next->t_type != T_NUMERIC_LIT) {
    fprintf(stderr, "Invalid token type for expression: %s\n", t_type_to_string(next->t_type));
    return 1;
  }

  term->tok = next;
  return 0;
}
