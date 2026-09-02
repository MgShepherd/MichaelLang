#include "parsing/statement.h"

#include "dynamic_array.h"
#include "parsing/utils.h"
#include <stdio.h>

unsigned char parse_dec_statement(Statement *statement, const Tokens *tokens, size_t *idx);
unsigned char parse_assign_statement(Statement *statement, const Tokens *tokens, size_t *idx);
unsigned char parse_ret_statement(Statement *statement, const Tokens *tokens, size_t *idx);

void statement_free(Statement *statement);

unsigned char parse_statements(Statements *statements, const Tokens *tokens, size_t *idx, TokenType exit_token) {
  while (*idx < tokens->count && tokens->elements[*idx].t_type != exit_token) {
    Statement statement;
    unsigned char result = 1;

    switch (tokens->elements[*idx].t_type) {
    case T_IDENTIFIER:
      if (tokens->count <= *idx + 1) {
        fprintf(stderr, "Invalid tokens at end of input\n");
        return result;
      }
      if (tokens->elements[*idx + 1].t_type == T_COLON) {
        result = parse_dec_statement(&statement, tokens, idx);
      } else {
        result = parse_assign_statement(&statement, tokens, idx);
      }
      break;
    case T_RETURN:
      result = parse_ret_statement(&statement, tokens, idx);
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

void statements_free(Statements *statements) {
  for (size_t j = 0; j < statements->count; j++) {
    statement_free(&statements->elements[j]);
  }
  dyn_array_free(statements);
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

unsigned char parse_dec_statement(Statement *statement, const Tokens *tokens, size_t *idx) {
  statement->s_type = S_NONE;
  DeclarationStatement dec;

  const Token *ident = expect_next(T_IDENTIFIER, tokens, idx);
  if (ident == NULL) {
    fprintf(stderr, "Failed to read identifier for declaration statement\n");
    return 1;
  }
  assert(ident->item != NULL);
  dec.lhs = ident->item;

  if (expect_next(T_COLON, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read colon for declaration statement\n");
    return 1;
  }

  dec.variable = expect_next(T_VAR, tokens, idx) != NULL;

  dec.d_type = tok_to_data_type(tokens->elements[(*idx)++].t_type);
  if (dec.d_type == D_NONE) {
    fprintf(stderr, "Failed to parse keyword into datatype\n");
    return 1;
  }

  if (expect_next(T_EQUALS, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read equals for declaration statement\n");
    return 1;
  }

  if (parse_expression(&dec.expr, tokens, idx) != 0) {
    fprintf(stderr, "Failed to parse expression for declaration statement\n");
    return 1;
  }

  if (expect_next(T_SEMI, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for declaration statement\n");
    return 1;
  }

  statement->s_type = S_DECLARATION;
  statement->s_union.dec = dec;

  return 0;
}

unsigned char parse_assign_statement(Statement *statement, const Tokens *tokens, size_t *idx) {
  statement->s_type = S_NONE;
  AssignmentStatement assign;

  const Token *ident = expect_next(T_IDENTIFIER, tokens, idx);
  if (ident == NULL) {
    fprintf(stderr, "Failed to read identifier for assignment statement\n");
    return 1;
  }
  assert(ident->item != NULL);
  assign.lhs = ident->item;

  if (expect_next(T_EQUALS, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read equals for assignment statement\n");
    return 1;
  }

  if (parse_expression(&assign.expr, tokens, idx) != 0) {
    fprintf(stderr, "Failed to parse expression for assignment statement\n");
    return 1;
  }

  if (expect_next(T_SEMI, tokens, idx) == NULL) {
    fprintf(stderr, "Failed to read semicolon for assignment statement\n");
    return 1;
  }

  statement->s_type = S_ASSIGNMENT;
  statement->s_union.assign = assign;
  return 0;
}

unsigned char parse_ret_statement(Statement *statement, const Tokens *tokens, size_t *idx) {
  statement->s_type = S_NONE;
  ReturnStatement ret;

  if (expect_next(T_RETURN, tokens, idx) == NULL) {
    fprintf(stderr, "Expected keyword token for return statement\n");
    return 1;
  }

  if (parse_expression(&ret.expr, tokens, idx) != 0) {
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
