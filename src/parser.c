#include "parser.h"
#include "lexer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATEMENT_ARRAY_LEN_FACTOR 0.4
#define STATEMENT_ARRAY_REALLOC_FACTOR 2
#define FUNCTION_STATEMENTS_LEN_ESTIMATE 10

/*
 * Will parse a series of statements either until end of input or a token of type exit_token is reached
 * Starts processing tokens from the provided idx and will update the idx pointer to end up at end of processed tokens
 * If you want to keep parsing until end of input, provided T_NONE for exit token
 * Will output the parsed statements into the statement_arr parameter
 * Assumes statement_arr already has an allocated statement array, with a count and capacity
 * Returns 0 on success, 1 if there was a failure
 */
unsigned char parse_statements(StatementArray *statement_arr, const TokenArray *token_arr, size_t *idx,
                               TokenType exit_token);

/*
 * Will set the dec property of statement union for a valid declaration statement
 * Will return an s_type of S_NONE when statement is not a valid declaration
 * Will additionally update the idx pointer to point at the next statement if valid
 */
Statement parse_dec_statement(const TokenArray *token_arr, size_t *idx);

/*
 * Will set the ret property of statement union for a valid return statement
 * Will return an s_type of S_NONE when statement is not a valid return
 * Will additionally update the idx pointer to point at the next statement if valid
 */
Statement parse_ret_statement(const TokenArray *token_arr, size_t *idx);

/*
 * Will set the func property of statement union for a valid function statement
 * Will return an s_type of S_NONE when statement is not a valid function
 * Will additionally update the idx pointer to point at the next statement if valid
 */
Statement parse_func_statement(const TokenArray *token_arr, size_t *idx);

/*
 * expect_next will read the next token and check it matches the expected token type
 * If matches will return the token pointer, otherwise will return NULL
 * Will update the idx pointer to point at the next token when valid
 */
const Token *expect_next(TokenType expected, const TokenArray *token_arr, size_t *idx);

/*
 * parse_expression will check whether the next tokens can be used as expression
 * Currently, this is either a T_IDENTIFIER or T_NUMERIC_LIT but may be expanded in future
 * Will update the idx pointer to point at the next token when valid
 * Will return NULL if no next token exists or the token type does not match
 */
const Token *parse_expression(const TokenArray *token_arr, size_t *idx);

unsigned char insert_statement(StatementArray *statement_arr, Statement *statement);

void free_statement_array(StatementArray *statement_arr);

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
#undef X

unsigned char parse_tokens(Program *program, const TokenArray *token_arr) {
  assert(token_arr != NULL && token_arr->count > 0);
  program->statement_arr.statements = NULL;

  StatementArray statement_arr;
  statement_arr.capacity = token_arr->count * STATEMENT_ARRAY_LEN_FACTOR;
  // Guard against getting 0 capacity when token input size is very small
  if (statement_arr.capacity == 0) {
    statement_arr.capacity = 1;
  }

  statement_arr.count = 0;
  statement_arr.statements = malloc(statement_arr.capacity * sizeof(Statement));

  if (statement_arr.statements == NULL) {
    fprintf(stderr, "Failed to allocate required space for statements array\n");
    return 1;
  }

  size_t idx = 0;
  if (parse_statements(&statement_arr, token_arr, &idx, T_NONE) != 0) {
    fprintf(stderr, "Failed to parse statements\n");
    free_statement_array(&statement_arr);
    return 1;
  }

  program->statement_arr = statement_arr;

  return 0;
}

unsigned char parse_statements(StatementArray *statement_arr, const TokenArray *token_arr, size_t *idx,
                               TokenType exit_token) {
  while (*idx < token_arr->count && token_arr->tokens[*idx].t_type != exit_token) {
    Statement statement;
    statement.s_type = S_NONE;

    switch (token_arr->tokens[*idx].t_type) {
    case T_IDENTIFIER:
      statement = parse_dec_statement(token_arr, idx);
      break;
    case T_KEYWORD:
      if (strcmp(token_arr->tokens[*idx].item, "return") == 0) {
        statement = parse_ret_statement(token_arr, idx);
      } else if (strcmp(token_arr->tokens[*idx].item, "func") == 0) {
        statement = parse_func_statement(token_arr, idx);
      } else {
        fprintf(stderr, "Unexpected keyword token: %s\n", token_arr->tokens[*idx].item);
      }
      break;
    default:
      fprintf(stderr, "Unexpected Token Type: %s\n", t_type_to_string(token_arr->tokens[*idx].t_type));
    }

    if (statement.s_type == S_NONE) {
      fprintf(stderr, "Failed to process next statement\n");
      return 1;
    }

    if (insert_statement(statement_arr, &statement) != 0) {
      fprintf(stderr, "Failed to insert statement\n");
      return 1;
    }
  }

  return 0;
}

void program_free(Program *program) {
  if (program != NULL && program->statement_arr.statements != NULL) {
    free_statement_array(&program->statement_arr);
  }
}

Statement parse_dec_statement(const TokenArray *token_arr, size_t *idx) {
  Statement statement;
  statement.s_type = S_NONE;
  DeclarationStatement dec;

  dec.identifier = expect_next(T_IDENTIFIER, token_arr, idx);
  if (dec.identifier == NULL) {
    fprintf(stderr, "Failed to read identifier for declaration statement\n");
    return statement;
  }

  if (expect_next(T_COLON, token_arr, idx) == NULL) {
    fprintf(stderr, "Failed to read colon for declaration statement\n");
    return statement;
  }

  // TODO: Currently just checking for any keyword here, need to ensure this a datatype
  dec.data_type = expect_next(T_KEYWORD, token_arr, idx);
  if (dec.data_type == NULL) {
    fprintf(stderr, "Failed to read data type for declaration statement\n");
    return statement;
  }

  if (expect_next(T_EQUALS, token_arr, idx) == NULL) {
    fprintf(stderr, "Failed to read equals for declaration statement\n");
    return statement;
  }

  dec.expr = parse_expression(token_arr, idx);
  if (dec.expr == NULL) {
    fprintf(stderr, "Failed to get value token\n");
    return statement;
  }

  if (expect_next(T_SEMI, token_arr, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for declaration statement\n");
    return statement;
  }

  statement.s_type = S_DECLARATION;
  statement.s_union.dec = dec;

  return statement;
}

Statement parse_ret_statement(const TokenArray *token_arr, size_t *idx) {
  Statement statement;
  statement.s_type = S_NONE;
  ReturnStatement ret;

  if (expect_next(T_KEYWORD, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for return statement\n");
    return statement;
  }

  ret.expr = parse_expression(token_arr, idx);
  if (ret.expr == NULL) {
    fprintf(stderr, "Failed to get value token\n");
    return statement;
  }

  if (expect_next(T_SEMI, token_arr, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for return statement\n");
    return statement;
  }

  statement.s_type = S_RETURN;
  statement.s_union.ret = ret;

  return statement;
}

Statement parse_func_statement(const TokenArray *token_arr, size_t *idx) {
  Statement statement;
  statement.s_type = S_NONE;
  FunctionStatement func;

  if (expect_next(T_KEYWORD, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for function statement\n");
    return statement;
  }

  func.identifier = expect_next(T_IDENTIFIER, token_arr, idx);
  if (func.identifier == NULL) {
    fprintf(stderr, "Failed to get identifier for function\n");
    return statement;
  }

  if (expect_next(T_LEFT_PAREN, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected opening parenthesis in function statement\n");
    return statement;
  }

  if (expect_next(T_RIGHT_PAREN, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected closing parenthesis in function statement\n");
    return statement;
  }

  if (expect_next(T_ARROW, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected arrow in function statement\n");
    return statement;
  }

  // TODO: Currently just checking for any keyword here, should make sure this is a datatype
  func.return_type = expect_next(T_KEYWORD, token_arr, idx);
  if (func.return_type == NULL) {
    fprintf(stderr, "Expected return type for function\n");
    return statement;
  }

  if (expect_next(T_LEFT_CURLY, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected opening curly brace before function body\n");
    return statement;
  }

  StatementArray statement_arr;
  statement_arr.capacity = FUNCTION_STATEMENTS_LEN_ESTIMATE;
  statement_arr.count = 0;
  statement_arr.statements = malloc(statement_arr.capacity * sizeof(Statement));

  if (statement_arr.statements == NULL) {
    fprintf(stderr, "Failed to allocate required memory for function statement array\n");
    return statement;
  }

  if (parse_statements(&statement_arr, token_arr, idx, T_RIGHT_CURLY) != 0) {
    fprintf(stderr, "Failed to process function body\n");
    free_statement_array(&statement_arr);
    return statement;
  }
  func.statement_arr = statement_arr;

  if (expect_next(T_RIGHT_CURLY, token_arr, idx) == NULL) {
    fprintf(stderr, "Expected closing curly brace after function body\n");
    free_statement_array(&statement_arr);
    return statement;
  }

  statement.s_type = S_FUNCTION;
  statement.s_union.func = func;

  return statement;
}

const Token *expect_next(TokenType expected, const TokenArray *token_arr, size_t *idx) {
  if (*idx >= token_arr->count) {
    fprintf(stderr, "Attempted to get next token but reached end of input\n");
    return NULL;
  }

  const Token *next = &token_arr->tokens[*idx];
  if (next->t_type != expected) {
    fprintf(stderr, "Expected Token Type %s, but got %s\n", t_type_to_string(expected), t_type_to_string(next->t_type));
    return NULL;
  }

  *idx += 1;
  return next;
}

const Token *parse_expression(const TokenArray *token_arr, size_t *idx) {
  if (*idx >= token_arr->count) {
    fprintf(stderr, "Attempted to get value token but reached end of input\n");
    return NULL;
  }

  const Token *next = &token_arr->tokens[*idx];
  if (next->t_type != T_IDENTIFIER && next->t_type != T_NUMERIC_LIT) {
    fprintf(stderr, "Expected next token to be literal or identifier, got %s\n", t_type_to_string(next->t_type));
    return NULL;
  }

  *idx += 1;
  return next;
}

unsigned char insert_statement(StatementArray *statement_arr, Statement *statement) {
  assert(statement_arr->statements != NULL);

  if (statement_arr->count >= statement_arr->capacity) {
    statement_arr->capacity = statement_arr->capacity * STATEMENT_ARRAY_REALLOC_FACTOR;
    Statement *new_statements = realloc(statement_arr->statements, statement_arr->capacity * sizeof(Statement));
    if (new_statements == NULL) {
      fprintf(stderr, "Failed to allocate additional required space for statements array\n");
      return 1;
    }
    statement_arr->statements = new_statements;
  }

  statement_arr->statements[statement_arr->count++] = *statement;

  return 0;
}

void free_statement_array(StatementArray *statement_arr) {
  if (statement_arr == NULL) {
    return;
  }
  for (size_t i = 0; i < statement_arr->count; i++) {
    switch (statement_arr->statements[i].s_type) {
    case S_FUNCTION:
      free_statement_array(&statement_arr->statements[i].s_union.func.statement_arr);
      break;
    default:
      break;
    }
  }
  free(statement_arr->statements);
  statement_arr->statements = NULL;
  statement_arr->count = 0;
  statement_arr->capacity = 0;
}
