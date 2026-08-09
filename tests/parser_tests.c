#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

typedef struct {
  const char *name;
  Token *input_tokens;
  size_t num_input_tokens;
  unsigned char expected_result;
  StatementArray expected_statement_arr;
} ParserTest;

unsigned char compare_pointers(const char *field_name, const void *expected, const void *actual);
unsigned char compare_statement_arr(const StatementArray *expected, const StatementArray *actual);

unsigned char test_parser() {
  Token case_1_input[] = {
      {.t_type = T_IDENTIFIER, .item = "x"},   {.t_type = T_COLON, .item = ":"},
      {.t_type = T_KEYWORD, .item = "i32"},    {.t_type = T_EQUALS, .item = "="},
      {.t_type = T_NUMERIC_LIT, .item = "10"}, {.t_type = T_SEMI, .item = ";"},
  };
  Statement case_1_expected_statements[] = {
      {
          .s_type = S_DECLARATION,
          .s_union = {.dec = {.identifier = &case_1_input[0], .data_type = &case_1_input[2], .expr = &case_1_input[4]}},
      },
  };
  StatementArray case_1_expected_statement_arr = {
      .statements = case_1_expected_statements,
      .count = sizeof(case_1_expected_statements) / sizeof(Statement),
      .capacity = sizeof(case_1_expected_statements) / sizeof(Statement),
  };

  Token case_2_input[] = {
      {.t_type = T_KEYWORD, .item = "return"},
      {.t_type = T_IDENTIFIER, .item = "x"},
      {.t_type = T_SEMI, .item = ";"},
  };
  Statement case_2_expected_statements[] = {
      {
          .s_type = S_RETURN,
          .s_union = {.ret = {.expr = &case_2_input[1]}},
      },
  };
  StatementArray case_2_expected_statement_arr = {
      .statements = case_2_expected_statements,
      .count = sizeof(case_2_expected_statements) / sizeof(Statement),
      .capacity = sizeof(case_2_expected_statements) / sizeof(Statement),
  };

  Token case_3_input[] = {
      {.t_type = T_KEYWORD, .item = "func"},  {.t_type = T_IDENTIFIER, .item = "x"},
      {.t_type = T_LEFT_PAREN, .item = "("},  {.t_type = T_RIGHT_PAREN, .item = ")"},
      {.t_type = T_ARROW, .item = "->"},      {.t_type = T_KEYWORD, .item = "i32"},
      {.t_type = T_LEFT_CURLY, .item = "{"},  {.t_type = T_KEYWORD, .item = "return"},
      {.t_type = T_IDENTIFIER, .item = "x"},  {.t_type = T_SEMI, .item = ";"},
      {.t_type = T_RIGHT_CURLY, .item = "}"},
  };
  Statement case_3_inner_statements[] = {
      {.s_type = S_RETURN, .s_union = {.ret = {.expr = &case_3_input[8]}}},
  };
  Statement case_3_expected_statements[] = {
      {
          .s_type = S_FUNCTION,
          .s_union =
              {
                  .func =
                      {
                          .identifier = &case_3_input[1],
                          .return_type = &case_3_input[5],
                          .statement_arr =
                              {
                                  .statements = case_3_inner_statements,
                                  .count = sizeof(case_3_inner_statements) / sizeof(Statement),
                              },
                      },
              },
      },
  };
  StatementArray case_3_expected_statement_arr = {
      .statements = case_3_expected_statements,
      .count = sizeof(case_3_expected_statements) / sizeof(Statement),
      .capacity = sizeof(case_3_expected_statements) / sizeof(Statement),
  };

  Token unhappy_case_1_input[] = {
      {.t_type = T_IDENTIFIER, .item = "x"},   {.t_type = T_IDENTIFIER, .item = "x"},
      {.t_type = T_KEYWORD, .item = "i32"},    {.t_type = T_EQUALS, .item = "="},
      {.t_type = T_NUMERIC_LIT, .item = "10"}, {.t_type = T_SEMI, .item = ";"},
  };

  Token unhappy_case_2_input[] = {
      {.t_type = T_IDENTIFIER, .item = "x"},   {.t_type = T_COLON, .item = ":"},
      {.t_type = T_KEYWORD, .item = "i32"},    {.t_type = T_EQUALS, .item = "="},
      {.t_type = T_NUMERIC_LIT, .item = "10"},
  };

  const ParserTest tests[] = {
      {
          .name = "happy: Single declaration statement",
          .input_tokens = case_1_input,
          .num_input_tokens = sizeof(case_1_input) / sizeof(Token),
          .expected_result = 0,
          .expected_statement_arr = case_1_expected_statement_arr,
      },
      {
          .name = "happy: Single return statement",
          .input_tokens = case_2_input,
          .num_input_tokens = sizeof(case_2_input) / sizeof(Token),
          .expected_result = 0,
          .expected_statement_arr = case_2_expected_statement_arr,
      },
      {
          .name = "happy: Single function statement",
          .input_tokens = case_3_input,
          .num_input_tokens = sizeof(case_3_input) / sizeof(Token),
          .expected_result = 0,
          .expected_statement_arr = case_3_expected_statement_arr,
      },
      {
          .name = "unhappy: Invalid token type in statement",
          .input_tokens = unhappy_case_1_input,
          .num_input_tokens = sizeof(unhappy_case_1_input) / sizeof(Token),
          .expected_result = 1,
          .expected_statement_arr = {},
      },
      {
          .name = "unhappy: Not enough tokens to make statement",
          .input_tokens = unhappy_case_2_input,
          .num_input_tokens = sizeof(unhappy_case_2_input) / sizeof(Token),
          .expected_result = 1,
          .expected_statement_arr = {},
      },
  };
  const size_t num_tests = sizeof(tests) / sizeof(ParserTest);

  for (size_t i = 0; i < num_tests; i++) {
    printf("Running test: %s\n", tests[i].name);

    Program program;
    TokenArray input = {.tokens = tests[i].input_tokens, .count = tests[i].num_input_tokens};
    unsigned char result = parse_tokens(&program, &input);
    if (tests[i].expected_result != result) {
      fprintf(stderr, "Expected result status %d, got %d\n", tests[i].expected_result, result);
      program_free(&program);
      return 1;
    }

    // If we expected a failure, no more checks needed at this point
    if (tests[i].expected_result != 0) {
      continue;
    }

    if (compare_statement_arr(&tests[i].expected_statement_arr, &program.statement_arr) != 0) {
      fprintf(stderr, "Statement arrays do not match\n");
      program_free(&program);
      return 1;
    }

    program_free(&program);
  }

  return 0;
}

unsigned char compare_statement_arr(const StatementArray *expected, const StatementArray *actual) {
  if (expected->count != actual->count) {
    fprintf(stderr, "Expected %zu statements, but got %zu\n", expected->count, actual->count);
    return 1;
  }

  for (size_t i = 0; i < expected->count; i++) {
    if (actual->statements[i].s_type != expected->statements[i].s_type) {
      fprintf(stderr, "Expected statement type %s, got %s\n", s_type_to_string(expected->statements[i].s_type),
              s_type_to_string(actual->statements[i].s_type));
      return 1;
    }

    switch (expected->statements[i].s_type) {
    case S_DECLARATION:
      DeclarationStatement expected_dec = expected->statements[i].s_union.dec;
      DeclarationStatement actual_dec = actual->statements[i].s_union.dec;

      if (compare_pointers("identifier", expected_dec.identifier, actual_dec.identifier) != 0) {
        return 1;
      }

      if (compare_pointers("data_type", expected_dec.data_type, actual_dec.data_type) != 0) {
        return 1;
      }

      if (compare_pointers("value", expected_dec.expr, actual_dec.expr) != 0) {
        return 1;
      }
      break;
    case S_RETURN:
      if (compare_pointers("value", expected->statements[i].s_union.ret.expr, actual->statements[i].s_union.ret.expr) !=
          0) {
        return 1;
      }
      break;
    case S_FUNCTION:
      FunctionStatement expected_func = expected->statements[i].s_union.func;
      FunctionStatement actual_func = actual->statements[i].s_union.func;

      if (compare_pointers("identifier", expected_func.identifier, actual_func.identifier) != 0) {
        return 1;
      }
      if (compare_pointers("return_type", expected_func.return_type, actual_func.return_type) != 0) {
        return 1;
      }

      if (compare_statement_arr(&expected_func.statement_arr, &actual_func.statement_arr) != 0) {
        return 1;
      }
      break;
    default:
      printf("Unexpected statement type: %s\n", s_type_to_string(expected->statements[i].s_type));
      return 1;
    }
  }

  return 0;
}

unsigned char compare_pointers(const char *field_name, const void *expected, const void *actual) {
  if (expected != actual) {
    fprintf(stderr, "Expected %s pointer: %p, got %p\n", field_name, expected, actual);
    return 1;
  }

  return 0;
}
