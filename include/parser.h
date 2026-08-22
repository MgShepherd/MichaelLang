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
// Expression := T_IDENTIFIER | T_NUMERIC_LITERAL
//
// DataType := 'i32'

#define STATEMENT_TYPES                                                                                                \
  X(S_NONE)                                                                                                            \
  X(S_DECLARATION)                                                                                                     \
  X(S_FUNCTION)                                                                                                        \
  X(S_RETURN)

#define X(N) N,
typedef enum { STATEMENT_TYPES } StatementType;
#undef X

const char *s_type_to_string(StatementType s);

typedef struct Statement Statement;

typedef struct {
  Statement *elements;
  size_t count;
  size_t capacity;
} Statements;

// TODO: Value should eventually be replaced with an expression - for now it is just a single value
typedef struct {
  const Token *identifier;
  const Token *data_type;
  const Token *expr;
} DeclarationStatement;

typedef struct {
  const Token *expr;
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
