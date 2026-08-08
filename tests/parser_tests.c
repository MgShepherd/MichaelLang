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
  size_t num_expected_statements;
  const Statement *expected_statements;
} ParserTest;

unsigned char compare_pointers(const char *field_name, const void *expected, const void *actual);

unsigned char test_parser() {
  Token case_1_input[] = {
      {.t_type = T_IDENTIFIER, .item = "x"},   {.t_type = T_COLON, .item = ":"},
      {.t_type = T_KEYWORD, .item = "i32"},    {.t_type = T_EQUALS, .item = "="},
      {.t_type = T_NUMERIC_LIT, .item = "10"}, {.t_type = T_SEMI, .item = ";"},
  };
  const Statement case_1_expected_statements[] = {
      {
          .s_type = S_DECLARATION,
          .s_union = {.dec = {.identifier = &case_1_input[0],
                              .data_type = &case_1_input[2],
                              .value = &case_1_input[4]}},
      },
  };

  Token case_2_input[] = {
      {.t_type = T_IDENTIFIER, .item = "x"},   {.t_type = T_IDENTIFIER, .item = "x"},
      {.t_type = T_KEYWORD, .item = "i32"},    {.t_type = T_EQUALS, .item = "="},
      {.t_type = T_NUMERIC_LIT, .item = "10"}, {.t_type = T_SEMI, .item = ";"},
  };

  Token case_3_input[] = {
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
          .num_expected_statements = sizeof(case_1_expected_statements) / sizeof(Statement),
          .expected_statements = case_1_expected_statements,
      },
      {
          .name = "unhappy: Invalid token type in statement",
          .input_tokens = case_2_input,
          .num_input_tokens = sizeof(case_2_input) / sizeof(Token),
          .expected_result = 1,
          .num_expected_statements = 0,
          .expected_statements = NULL,
      },
      {
          .name = "unhappy: Not enough tokens to make statement",
          .input_tokens = case_3_input,
          .num_input_tokens = sizeof(case_3_input) / sizeof(Token),
          .expected_result = 1,
          .num_expected_statements = 0,
          .expected_statements = NULL,
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

    if (tests[i].num_expected_statements != program.statement_arr.count) {
      fprintf(stderr, "Expected %zu statements, but got %zu\n", tests[i].num_expected_statements,
              program.statement_arr.count);
      program_free(&program);
      return 1;
    }

    for (size_t j = 0; j < program.statement_arr.count; j++) {
      if (program.statement_arr.statements[j].s_type != tests[i].expected_statements[j].s_type) {
        fprintf(stderr, "Expected statement type %s, got %s\n",
                s_type_to_string(tests[i].expected_statements[j].s_type),
                s_type_to_string(program.statement_arr.statements[j].s_type));
        program_free(&program);
        return 1;
      }

      switch (tests[i].expected_statements[j].s_type) {
      case S_DECLARATION:
        if (compare_pointers("identifier", tests[i].expected_statements[j].s_union.dec.identifier,
                             program.statement_arr.statements[j].s_union.dec.identifier) != 0) {
          program_free(&program);
          return 1;
        }

        if (compare_pointers("data_type", tests[i].expected_statements[j].s_union.dec.data_type,
                             program.statement_arr.statements[j].s_union.dec.data_type) != 0) {
          program_free(&program);
          return 1;
        }

        if (compare_pointers("value", tests[i].expected_statements[j].s_union.dec.value,
                             program.statement_arr.statements[j].s_union.dec.value) != 0) {
          program_free(&program);
          return 1;
        }
        break;
      default:
        printf("Unexpected statement type\n");
        return 1;
      }
    }

    program_free(&program);
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
