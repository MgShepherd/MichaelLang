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
#define FUNCTION_VARIABLES_LEN_ESTIMATE 10

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
unsigned char parse_statements(Statements *statements, Identifiers *identifiers, const Tokens *tokens, size_t *idx,
                               TokenType exit_token);

/*
 * Will parse a function and all its statements into provided function parameter
 * Will return 0 on success, 1 on failure
 * Will additionally update the idx pointer to point at the next statement if valid
 */
unsigned char parse_function(Function *function, const Tokens *tokens, size_t *idx);

/*
 * Will set the dec property of statement union for a valid declaration statement
 * Will output result to statement parameter
 * Will additionally update the idx pointer to point at the next statement if valid
 * Will update the provided variables array to include newly declared variable
 * Returns 0 on success, 1 on failure
 */
unsigned char parse_dec_statement(Statement *statement, const Tokens *tokens, Identifiers *identifiers, size_t *idx);

/*
 * Will set the ret property of statement union for a valid return statement
 * Will output result to statement parameter
 * Will additionally update the idx pointer to point at the next statement if valid
 * Returns 0 on success, 1 on failure
 */
unsigned char parse_ret_statement(Statement *statement, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx);

/*
 * parse_expression will attempt to convert the next tokens into an expression and output to provided expression
 * Will update the idx pointer to point at the next token when valid
 * Returns 0 on success, 1 on failure
 */
unsigned char parse_expression(Expression *expression, const Tokens *tokens, const Identifiers *identifiers,
                               size_t *idx);

/*
 * parse_terminal_expr will attempt to convert the next tokens into a terminal expression
 * This can either be a numerical constant (potentially signed) or a variable identifier
 * Returns 0 on success, 1 on failure
 */
unsigned char parse_terminal_expr(TerminalExpr *term, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx);

/*
 * expect_next will read the next token and check it matches the expected token type
 * If matches will return the token pointer, otherwise will return NULL
 * Will update the idx pointer to point at the next token when valid
 */
const Token *expect_next(TokenType expected, const Tokens *tokens, size_t *idx);

/*
 * expect_keyword will read the next token, check it is a keyword and check if the keyword matches the provided
 * If matches will return the token pointer, otherwise will return NULL
 * Will update the idx pointer to point at the next token when valid
 */
const Token *expect_keyword(const char *keyword, const Tokens *tokens, size_t *idx);

/*
 * Converts a provided token into the matching datatype
 * Will return D_NONE if unable to convert token to datatype
 */
DataType tok_to_data_type(const Token *token);

/*
 * Checks whether a provided token has a identifier in the identifiers list
 */
bool identifier_exists(const Identifiers *identifiers, const Token *token);

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

const char *d_type_to_string(DataType e) {
  switch (e) {
    DATA_TYPES
  default:
    return "unknown";
  }
}

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

unsigned char parse_statements(Statements *statements, Identifiers *identifiers, const Tokens *tokens, size_t *idx,
                               TokenType exit_token) {
  while (*idx < tokens->count && tokens->elements[*idx].t_type != exit_token) {
    Statement statement;
    unsigned char result = 1;

    switch (tokens->elements[*idx].t_type) {
    case T_IDENTIFIER:
      result = parse_dec_statement(&statement, tokens, identifiers, idx);
      break;
    case T_KEYWORD:
      result = parse_ret_statement(&statement, tokens, identifiers, idx);
      break;
    default:
      fprintf(stderr, "Unexpected Token Type: %s\n", t_type_to_string(tokens->elements[*idx].t_type));
    }

    if (result != 0) {
      fprintf(stderr, "Failed to process next statement\n");
      return result;
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
    dyn_array_free(&functions->elements[i].identifiers);
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

unsigned char parse_dec_statement(Statement *statement, const Tokens *tokens, Identifiers *identifiers, size_t *idx) {
  statement->s_type = S_NONE;
  DeclarationStatement dec;
  Identifier ident;

  dec.identifier = expect_next(T_IDENTIFIER, tokens, idx);
  if (dec.identifier == NULL) {
    fprintf(stderr, "Failed to read identifier for declaration statement\n");
    return 1;
  }

  if (expect_next(T_COLON, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read colon for declaration statement\n");
    return 1;
  }

  if (expect_keyword("var", tokens, idx) != NULL) {
    ident.variable = true;
    dec.variable = true;
  }

  const Token *next = expect_next(T_DATATYPE, tokens, idx);
  if (next == NULL) {
    fprintf(stderr, "Failed to read data type for declaration statement\n");
    return 1;
  }

  dec.data_type = tok_to_data_type(next);
  if (dec.data_type == D_NONE) {
    fprintf(stderr, "Failed to parse keyword into datatype\n");
    return 1;
  }

  if (expect_next(T_EQUALS, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read equals for declaration statement\n");
    return 1;
  }

  if (parse_expression(&dec.expr, tokens, identifiers, idx) != 0) {
    fprintf(stderr, "Failed to parse expression for declaration statement\n");
    return 1;
  }

  if (expect_next(T_SEMI, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for declaration statement\n");
    return 1;
  }

  ident.d_type = dec.data_type;
  ident.name = dec.identifier->item;
  dyn_array_insert(identifiers, ident);

  statement->s_type = S_DECLARATION;
  statement->s_union.dec = dec;

  return 0;
}

unsigned char parse_ret_statement(Statement *statement, const Tokens *tokens, const Identifiers *identifiers,
                                  size_t *idx) {
  statement->s_type = S_NONE;
  ReturnStatement ret;

  if (expect_keyword("return", tokens, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for return statement\n");
    return 1;
  }

  if (parse_expression(&ret.expr, tokens, identifiers, idx) != 0) {
    fprintf(stderr, "Failed to parse expression for return statement\n");
    return 1;
  }

  if (expect_next(T_SEMI, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for return statement\n");
    return 1;
  }

  statement->s_type = S_RETURN;
  statement->s_union.ret = ret;

  return 0;
}

unsigned char parse_function(Function *function, const Tokens *tokens, size_t *idx) {
  if (expect_keyword("func", tokens, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for function\n");
    return 1;
  }

  function->name = expect_next(T_IDENTIFIER, tokens, idx);
  if (function->name == NULL) {
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

  const Token *next = expect_next(T_DATATYPE, tokens, idx);
  if (next == NULL) {
    fprintf(stderr, "Expected return type for function\n");
    return 1;
  }

  function->return_type = tok_to_data_type(next);
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
    if (!identifier_exists(identifiers, next)) {
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

DataType tok_to_data_type(const Token *token) {
  assert(token->t_type == T_DATATYPE);
  if (strcmp("i32", token->item) == 0) {
    return D_I32;
  }
  return D_NONE;
}

bool identifier_exists(const Identifiers *identifiers, const Token *token) {
  for (size_t i = 0; i < identifiers->count; i++) {
    if (strcmp(identifiers->elements[i].name, token->item) == 0) {
      return true;
    }
  }
  return false;
}
