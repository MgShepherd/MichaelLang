#ifndef _PARSING_TYPE_H_
#define _PARSING_TYPE_H_

#include "lexer.h"

#define STATEMENT_TYPES                                                                                                \
  X(S_NONE)                                                                                                            \
  X(S_DECLARATION)                                                                                                     \
  X(S_ASSIGNMENT)                                                                                                      \
  X(S_RETURN)

#define EXPRESSION_TYPES                                                                                               \
  X(E_NONE)                                                                                                            \
  X(E_COMPOUND)                                                                                                        \
  X(E_TERMINAL)

#define TERM_EXPR_TYPES                                                                                                \
  X(TE_NONE)                                                                                                           \
  X(TE_NUMERICAL)                                                                                                      \
  X(TE_BOOLEAN)

#define DATA_TYPES                                                                                                     \
  X(D_NONE)                                                                                                            \
  X(D_BOOL)                                                                                                            \
  X(D_I32)

#define SIGN                                                                                                           \
  X(SIGN_POSTIIVE)                                                                                                     \
  X(SIGN_NEGATIVE)

#define X(N) N,
typedef enum { STATEMENT_TYPES } StatementType;
typedef enum { EXPRESSION_TYPES } ExpressionType;
typedef enum { TERM_EXPR_TYPES } TerminalExprType;
typedef enum { DATA_TYPES } DataType;
typedef enum { SIGN } Sign;
#undef X

const char *s_type_to_string(StatementType s);
const char *e_type_to_string(ExpressionType e);
const char *te_type_to_string(TerminalExprType te);
const char *d_type_to_string(DataType e);

/*
 * Converts a provided token into the matching datatype
 * Will return D_NONE if unable to convert token to datatype
 */
DataType tok_to_data_type(const Token *token);

#endif // _PARSING_TYPE_H_
