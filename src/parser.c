#include "parser.h"
#include "dynamic_array.h"
#include "lexer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUNCTION_ARRAY_LEN_FACTOR 0.1
#define ARRAY_REALLOC_FACTOR 2
#define FUNCTION_STATEMENTS_LEN_ESTIMATE 10

/*
 * Will parse a series of functions either until end of input to build a program
 * Starts processing from the first element in elements
 * Will output the parsed statements into the functions parameter
 * Assumes functions already has an allocated element array, with a count and capacity
 * Returns 0 on success, 1 if there was a failure
 */
unsigned char parse_functions(Functions *functions, const Tokens *tokens);

/*
 * Will parse a series of statements either until a token of type exit_token is reached
 * Starts processing elements from the provided idx and will update the idx pointer to end up at end of processed
 * elements Will output the parsed statements into the statements parameter Assumes statements already has an
 * allocated element array, with a count and capacity Will error if exit_token is not found before reaching the end of
 * the input Returns 0 on success, 1 if there was a failure
 */
unsigned char parse_statements(Statements *statements, const Tokens *tokens, size_t *idx, TokenType exit_token);

/*
 * Will parse a function and all its statements into provided function parameter
 * Will return 0 on success, 1 on failure
 * Will additionally update the idx pointer to point at the next statement if valid
 */
unsigned char parse_function(Function *function, const Tokens *tokens, size_t *idx);

/*
 * Will set the dec property of statement union for a valid declaration statement
 * Will return an s_type of S_NONE when statement is not a valid declaration
 * Will additionally update the idx pointer to point at the next statement if valid
 */
Statement parse_dec_statement(const Tokens *tokens, size_t *idx);

/*
 * Will set the ret property of statement union for a valid return statement
 * Will return an s_type of S_NONE when statement is not a valid return
 * Will additionally update the idx pointer to point at the next statement if valid
 */
Statement parse_ret_statement(const Tokens *tokens, size_t *idx);

/*
 * expect_next will read the next token and check it matches the expected token type
 * If matches will return the token pointer, otherwise will return NULL
 * Will update the idx pointer to point at the next token when valid
 */
const Token *expect_next(TokenType expected, const Tokens *tokens, size_t *idx);

/*
 * parse_expression will attempt to convert the next tokens into an expression
 * Currently, this is either a T_IDENTIFIER or T_NUMERIC_LIT but may be expanded in future
 * Will update the idx pointer to point at the next token when valid
 * Will return the e_type E_NONE if no next token exists or not a valid expression
 */
Expression parse_expression(const Tokens *tokens, size_t *idx);

/*
 * Frees functions array as well as nested statement arrays
 */
void functions_free(Functions *functions);

/*
 * Frees a single statement including all nested expressions
 */
void statement_free(Statement *statement);

/*
 * Frees a single expression
 */
void expression_free(Expression *expression);

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
#undef X

unsigned char parse_tokens(Program *program, const Tokens *tokens) {
  assert(tokens != NULL && tokens->count > 0);
  program->functions.elements = NULL;

  size_t capacity = tokens->count * FUNCTION_ARRAY_LEN_FACTOR;
  // Guard against getting 0 capacity when token input size is very small
  if (capacity == 0) {
    capacity = 1;
  }
  Functions functions;
  dyn_array_init(&functions, sizeof(Function), capacity);

  if (parse_functions(&functions, tokens) != 0) {
    fprintf(stderr, "Failed to parse functions\n");
    functions_free(&functions);

    return 1;
  }

  program->functions = functions;

  return 0;
}

unsigned char parse_functions(Functions *functions, const Tokens *tokens) {
  size_t idx = 0;
  while (idx < tokens->count) {
    if (tokens->elements[idx].t_type != T_KEYWORD) {
      fprintf(stderr, "Expected %s token for function begin, got %s\n", t_type_to_string(T_KEYWORD),
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

unsigned char parse_statements(Statements *statements, const Tokens *tokens, size_t *idx, TokenType exit_token) {
  while (*idx < tokens->count && tokens->elements[*idx].t_type != exit_token) {
    Statement statement;
    statement.s_type = S_NONE;

    switch (tokens->elements[*idx].t_type) {
    case T_IDENTIFIER:
      statement = parse_dec_statement(tokens, idx);
      break;
    case T_KEYWORD:
      statement = parse_ret_statement(tokens, idx);
      break;
    default:
      fprintf(stderr, "Unexpected Token Type: %s\n", t_type_to_string(tokens->elements[*idx].t_type));
    }

    if (statement.s_type == S_NONE) {
      fprintf(stderr, "Failed to process next statement\n");
      return 1;
    }

    dyn_array_insert(statements, statement);
  }

  if (*idx == tokens->count) {
    fprintf(stderr, "Reached end of input before finding block ending token\n");
    return 1;
  }

  return 0;
}

void program_free(Program *program) {
  if (program != NULL && program->functions.elements != NULL) {
    functions_free(&program->functions);
  }
}

void functions_free(Functions *functions) {
  for (size_t i = 0; i < functions->count; i++) {
    for (size_t j = 0; j < functions->elements[i].statements.count; j++) {
      statement_free(&functions->elements[i].statements.elements[j]);
    }
    dyn_array_free(&functions->elements[i].statements);
  }
  dyn_array_free(functions);
}

void statement_free(Statement *statement) {
  switch (statement->s_type) {
  case S_DECLARATION:
    expression_free(&statement->s_union.dec.expr);
    break;
  case S_RETURN:
    expression_free(&statement->s_union.ret.expr);
    break;
  default:
    break;
  }
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

Statement parse_dec_statement(const Tokens *tokens, size_t *idx) {
  Statement statement;
  statement.s_type = S_NONE;
  DeclarationStatement dec;

  dec.identifier = expect_next(T_IDENTIFIER, tokens, idx);
  if (dec.identifier == NULL) {
    fprintf(stderr, "Failed to read identifier for declaration statement\n");
    return statement;
  }

  if (expect_next(T_COLON, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read colon for declaration statement\n");
    return statement;
  }

  // TODO: Currently just checking for any keyword here, need to ensure this a datatype
  dec.data_type = expect_next(T_KEYWORD, tokens, idx);
  if (dec.data_type == NULL) {
    fprintf(stderr, "Failed to read data type for declaration statement\n");
    return statement;
  }

  if (expect_next(T_EQUALS, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read equals for declaration statement\n");
    return statement;
  }

  dec.expr = parse_expression(tokens, idx);
  if (dec.expr.e_type == E_NONE) {
    fprintf(stderr, "Failed to parse expression for declaration statement\n");
    return statement;
  }

  if (expect_next(T_SEMI, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for declaration statement\n");
    return statement;
  }

  statement.s_type = S_DECLARATION;
  statement.s_union.dec = dec;

  return statement;
}

Statement parse_ret_statement(const Tokens *tokens, size_t *idx) {
  Statement statement;
  statement.s_type = S_NONE;
  ReturnStatement ret;

  if (expect_next(T_KEYWORD, tokens, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for return statement\n");
    return statement;
  }

  ret.expr = parse_expression(tokens, idx);
  if (ret.expr.e_type == E_NONE) {
    fprintf(stderr, "Failed to parse expression for return statement\n");
    return statement;
  }

  if (expect_next(T_SEMI, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for return statement\n");
    return statement;
  }

  statement.s_type = S_RETURN;
  statement.s_union.ret = ret;

  return statement;
}

unsigned char parse_function(Function *function, const Tokens *tokens, size_t *idx) {
  if (expect_next(T_KEYWORD, tokens, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for function\n");
    return 1;
  }

  function->identifier = expect_next(T_IDENTIFIER, tokens, idx);
  if (function->identifier == NULL) {
    fprintf(stderr, "Failed to get identifier for function\n");
    return 1;
  }

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

  // TODO: Currently just checking for any keyword here, should make sure this is a datatype
  function->return_type = expect_next(T_KEYWORD, tokens, idx);
  if (function->return_type == NULL) {
    fprintf(stderr, "Expected return type for function\n");
    return 1;
  }

  if (expect_next(T_LEFT_CURLY, tokens, idx) == NULL) {
    fprintf(stderr, "Expected opening curly brace before function body\n");
    return 1;
  }

  Statements statements;
  dyn_array_init(&statements, sizeof(Statement), FUNCTION_STATEMENTS_LEN_ESTIMATE);

  if (parse_statements(&statements, tokens, idx, T_RIGHT_CURLY) != 0) {
    fprintf(stderr, "Failed to process function body\n");
    dyn_array_free(&statements);
    return 1;
  }
  function->statements = statements;

  if (expect_next(T_RIGHT_CURLY, tokens, idx) == NULL) {
    fprintf(stderr, "Expected closing curly brace after function body\n");
    dyn_array_free(&statements);
    return 1;
  }

  return 0;
}

const Token *expect_next(TokenType expected, const Tokens *tokens, size_t *idx) {
  if (*idx >= tokens->count) {
    fprintf(stderr, "Attempted to get next token but reached end of input\n");
    return NULL;
  }

  const Token *next = &tokens->elements[*idx];
  if (next->t_type != expected) {
    fprintf(stderr, "Expected Token Type %s, but got %s\n", t_type_to_string(expected), t_type_to_string(next->t_type));
    return NULL;
  }

  *idx += 1;
  return next;
}

Expression parse_expression(const Tokens *tokens, size_t *idx) {
  Expression expr;
  expr.e_type = E_NONE;

  if (*idx >= tokens->count) {
    fprintf(stderr, "Attempted to get value token but reached end of input\n");
    return expr;
  }

  const Token *next = &tokens->elements[(*idx)++];
  if (next->t_type != T_IDENTIFIER && next->t_type != T_NUMERIC_LIT) {
    fprintf(stderr, "Expected next token to be literal or identifier, got %s\n", t_type_to_string(next->t_type));
    return expr;
  }

  TerminalExpr term = {.tok = next};
  if (*idx >= tokens->count || tokens->elements[*idx].t_type != T_ARITHMETIC) {
    expr.e_type = E_TERM;
    expr.e_union.terminal = term;
    return expr;
  }

  const Token *op = &tokens->elements[(*idx)++];
  Expression rhs = parse_expression(tokens, idx);

  Expression *rhs_ptr = malloc(sizeof(Expression));
  if (rhs_ptr == NULL) {
    fprintf(stderr, "Failed to allocate memory for Right hand side of expression\n");
    return expr;
  }
  *rhs_ptr = rhs;

  ArithmeticExpr arith = {
      .lhs = term,
      .op = op,
      .rhs = rhs_ptr,
  };

  expr.e_type = E_ARITHMETIC;
  expr.e_union.arithmetic = arith;
  return expr;
}
