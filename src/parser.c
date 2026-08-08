#include "parser.h"
#include "lexer.h"

#include <assert.h>
#include <stdio.h>

#define STATEMENT_ARRAY_LEN_FACTOR 0.4
#define STATEMENT_ARRAY_REALLOC_FACTOR 2

/*
 * Will set the dec property of statement union for a valid declaration statement
 * Will return an s_type of S_INVALID when statement is not a valid declaration
 * Will additionally update the idx pointer to point at the next statement if valid
 */
Statement parse_dec_statement(const TokenArray *token_arr, size_t *idx);

/*
 * expect_next will read the next token and check it matches the expected token type
 * If matches will return the token pointer, otherwise will return NULL
 * Will update the idx pointer to point at the next token when valid
 */
const Token *expect_next(TokenType expected, const TokenArray *token_arr, size_t *idx);

unsigned char insert_statement(StatementArray *statement_arr, Statement *statement);

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

  size_t i = 0;
  while (i < token_arr->count) {
    Statement statement;
    statement.s_type = S_INVALID;

    switch (token_arr->tokens[i].t_type) {
    case T_IDENTIFIER:
      statement = parse_dec_statement(token_arr, &i);
      break;
    default:
      fprintf(stderr, "Unexpected Token Type: %s\n", t_type_to_string(token_arr->tokens[i].t_type));
    }

    if (statement.s_type == S_INVALID) {
      fprintf(stderr, "Failed to process next statement\n");
      free(statement_arr.statements);
      return 1;
    }

    if (insert_statement(&statement_arr, &statement) != 0) {
      fprintf(stderr, "Failed to insert statement\n");
      free(statement_arr.statements);
      return 1;
    }
  }

  program->statement_arr = statement_arr;

  return 0;
}

void program_free(Program *program) {
  if (program != NULL && program->statement_arr.statements != NULL) {
    free(program->statement_arr.statements);
  }
  program->statement_arr.statements = NULL;
  program->statement_arr.count = 0;
  program->statement_arr.capacity = 0;
}

Statement parse_dec_statement(const TokenArray *token_arr, size_t *idx) {
  Statement statement;
  statement.s_type = S_INVALID;
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

  dec.value = expect_next(T_NUMERIC_LIT, token_arr, idx);
  if (dec.value == NULL) {
    fprintf(stderr, "Failed to read numeric literal for declaration statement\n");
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

const Token *expect_next(TokenType expected, const TokenArray *token_arr, size_t *idx) {
  if (*idx >= token_arr->count) {
    fprintf(stderr, "Expected Token type %s, but reached end of input\n", t_type_to_string(expected));
    return NULL;
  }

  if (token_arr->tokens[*idx].t_type != expected) {
    fprintf(stderr, "Expected Token Type %s, but got %s\n", t_type_to_string(expected),
            t_type_to_string(token_arr->tokens[*idx].t_type));
    return NULL;
  }

  *idx += 1;
  return &token_arr->tokens[*idx - 1];
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
