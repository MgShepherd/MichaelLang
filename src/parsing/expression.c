#include "parsing/expression.h"
#include "parsing/identifier.h"
#include "parsing/type.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

unsigned char parse_terminal_expr(TerminalExpr *term, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx);
unsigned char parse_numerical_expr(NumericalExpr *num, const Tokens *tokens, const Identifiers *identifiers,
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
    expression->e_type = E_TERMINAL;
    expression->e_union.term = term;
    return 0;
  }

  // TODO: When support boolean expressions, check the type of lhs matches whats needed for operator
  const Token *op = &tokens->elements[(*idx)++];
  assert(op->t_type == T_ARITHMETIC);

  // TODO: Check that the rhs expression has same type as the lhs
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

  CompoundExpr comp = {
      .lhs = term,
      .op = op,
      .rhs = rhs_ptr,
  };

  expression->e_type = E_COMPOUND;
  expression->e_union.comp = comp;
  return 0;
}

void expression_free(Expression *expression) {
  switch (expression->e_type) {
  case E_COMPOUND:
    expression_free(expression->e_union.comp.rhs);
    free(expression->e_union.comp.rhs);
  default:
    break;
  }
}

unsigned char parse_terminal_expr(TerminalExpr *term, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx) {
  switch (tokens->elements[*idx].t_type) {
  case T_ARITHMETIC:
  case T_NUMERIC_LIT:
    NumericalExpr num;
    const unsigned char result = parse_numerical_expr(&num, tokens, identifiers, idx);
    if (result != 0) {
      return result;
    }
    term->te_type = TE_NUMERICAL;
    term->t_union.num = num;
    break;
  case T_IDENTIFIER:
    const Identifier *ident = get_identifier(identifiers, &tokens->elements[*idx]);
    if (ident == NULL) {
      fprintf(stderr, "Undefined varaible: %s\n", tokens->elements[*idx].item);
      return 1;
    }

    switch (ident->d_type) {
    case D_I32:
      NumericalExpr num;
      const unsigned char result = parse_numerical_expr(&num, tokens, identifiers, idx);
      if (result != 0) {
        return result;
      }
      term->te_type = TE_NUMERICAL;
      term->t_union.num = num;
      return result;
    default:
      fprintf(stderr, "Unexpected variable type: %s\n", d_type_to_string(ident->d_type));
      return 1;
    }
  default:
    fprintf(stderr, "Invalid token type for beginning of expression\n");
    return 1;
  }

  return 0;
}

unsigned char parse_numerical_expr(NumericalExpr *num, const Tokens *tokens, const Identifiers *identifiers,
                                   size_t *idx) {
  const Token *next = &tokens->elements[(*idx)++];

  if (next->t_type == T_ARITHMETIC) {
    if (strcmp(next->item, "+") == 0) {
      num->sign = SIGN_POSTIIVE;
    } else if (strcmp(next->item, "-") == 0) {
      num->sign = SIGN_NEGATIVE;
    } else {
      fprintf(stderr, "Invalid sign for terminal expression: %s\n", next->item);
      return 1;
    }

    next = &tokens->elements[(*idx)++];
  } else {
    num->sign = SIGN_POSTIIVE;
  }

  if (next->t_type == T_IDENTIFIER) {
    if (get_identifier(identifiers, next) == NULL) {
      fprintf(stderr, "Undefined variable: %s\n", next->item);
      return 1;
    }

    num->tok = next;
    return 0;
  }

  if (next->t_type != T_NUMERIC_LIT) {
    fprintf(stderr, "Invalid token type for expression: %s\n", t_type_to_string(next->t_type));
    return 1;
  }
  num->tok = next;

  return 0;
}
