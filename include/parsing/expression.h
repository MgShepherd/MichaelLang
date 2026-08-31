#ifndef _PARSING_EXPRESSION_H_
#define _PARSING_EXPRESSION_H_

#include "lexer.h"
#include "parsing/identifier.h"
#include "parsing/type.h"

typedef struct Expression Expression;

typedef struct {
  const Token *tok;
  Sign sign;
} TerminalExpr;

typedef struct {
  TerminalExpr lhs;
  const Token *op;
  Expression *rhs;
} ArithmeticExpr;

typedef union {
  TerminalExpr terminal;
  ArithmeticExpr arithmetic;
} ExpressionUnion;

struct Expression {
  ExpressionUnion e_union;
  ExpressionType e_type;
};

/*
 * parse_expression will attempt to convert the next tokens into an expression and output to provided expression
 * Will update the idx pointer to point at the next token when valid
 * Returns 0 on success, 1 on failure
 */
unsigned char parse_expression(Expression *expression, const Tokens *tokens, const Identifiers *identifiers,
                               size_t *idx);

/*
 * Frees a single expression
 */
void expression_free(Expression *expression);

#endif // _PARSING_EXPRESSION_H_
