#include "parsing/type.h"

#include <assert.h>
#include <string.h>

#define X(N)                                                                                                           \
  case N:                                                                                                              \
    return #N;

const char *s_type_to_string(StatementType s) {
  switch (s) {
    STATEMENT_TYPES
  default:
    return "unknown";
  }
}

const char *e_type_to_string(ExpressionType e) {
  switch (e) {
    EXPRESSION_TYPES
  default:
    return "unknown";
  }
}

const char *te_type_to_string(TerminalExprType e) {
  switch (e) {
    TERM_EXPR_TYPES
  default:
    return "unknown";
  }
}

const char *d_type_to_string(DataType e) {
  switch (e) {
    DATA_TYPES
  default:
    return "unknown";
  }
}

DataType tok_to_data_type(TokenType t_type) {
  switch (t_type) {
  case T_I32:
    return D_I32;
  case T_BOOL:
    return D_BOOL;
  default:
    return D_NONE;
  }
}
