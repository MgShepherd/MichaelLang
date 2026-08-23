#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"

// Current Grammar Implemented for Parsing
//
// Program := [Statement]
//
// Statement := FunctionStatement | DeclarationStatement | ReturnStatement
// FunctionStatement := 'func' T_IDENTIFIER '(' ')' '->' DataType '{' [Statement] '}'
// DeclarationStatement := T_IDENTIFIER ':' DataType '=' Expression ';'
// ReturnStatement := 'return' Expression ';'
// Expression := TerminalExpr | ArithmeticExpr
// TerminalExpr := T_IDENTIFIER | T_NUMERIC_LITERAL
// ArithmeticExpr := TerminalExpr ArithmeticOp Expression
//
// DataType := 'i32'
// ArithmeticOp := '+'

#define STATEMENT_TYPES                                                                                                \
  X(S_NONE)                                                                                                            \
  X(S_DECLARATION)                                                                                                     \
  X(S_RETURN)

#define EXPRESSION_TYPES                                                                                               \
  X(E_NONE)                                                                                                            \
  X(E_ARITHMETIC)                                                                                                      \
  X(E_TERM)

#define X(N) N,
typedef enum { STATEMENT_TYPES } StatementType;
typedef enum { EXPRESSION_TYPES } ExpressionType;
#undef X

const char *s_type_to_string(StatementType s);
const char *e_type_to_string(ExpressionType e);

typedef struct Expression Expression;

typedef struct {
  const Token *tok;
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

typedef struct Statement Statement;

typedef struct {
  Statement *elements;
  size_t count;
  size_t capacity;
} Statements;

typedef struct {
  const Token *identifier;
  const Token *data_type;
  Expression expr;
} DeclarationStatement;

typedef struct {
  Expression expr;
} ReturnStatement;

// TODO: Functions eventually need to take parameters - for now always empty
typedef struct {
  const Token *identifier;
  const Token *return_type;
  Statements statements;
} Function;

typedef struct {
  Function *elements;
  size_t count;
  size_t capacity;
} Functions;

// Create the Statement "tagged union" by attached the StatementUnion with an enum value
typedef union {
  DeclarationStatement dec;
  ReturnStatement ret;
} StatementUnion;

struct Statement {
  StatementType s_type;
  StatementUnion s_union;
};

typedef struct {
  Functions functions;
} Program;

/*
 * parse_tokens will convert the filled tokens into a Program Syntax Tree
 * Will output result into the program arguement - will overwrite any existing data
 * Allocates program on the heap so will need to be freed - use program_free for this
 * Note: Created program stores pointers to tokens, so tokens should not be freed until program can also be freed
 */
unsigned char parse_tokens(Program *program, const Tokens *tokens);

/*
 * Frees the memory associated with a program
 */
void program_free(Program *program);

#endif // _PARSER_H_
