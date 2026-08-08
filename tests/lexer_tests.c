#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

typedef struct {
  const char *name;
  char *input;
  unsigned char expected_result;
  size_t num_expected_tokens;
  const TokenType *expected_token_types;
} LexerTest;

unsigned char test_lexer() {
  const TokenType case_1_expected_toks[] = {T_KEYWORD, T_LEFT_PAREN,  T_IDENTIFIER, T_RIGHT_PAREN, T_ARROW,
                                            T_KEYWORD, T_LEFT_CURLY,  T_IDENTIFIER, T_COLON,       T_KEYWORD,
                                            T_EQUALS,  T_NUMERIC_LIT, T_SEMI,       T_KEYWORD,     T_IDENTIFIER,
                                            T_SEMI,    T_RIGHT_CURLY};

  const TokenType case_2_expected_toks[] = {T_KEYWORD, T_LEFT_PAREN,  T_IDENTIFIER, T_RIGHT_PAREN, T_ARROW,
                                            T_KEYWORD, T_LEFT_CURLY,  T_IDENTIFIER, T_COLON,       T_KEYWORD,
                                            T_EQUALS,  T_NUMERIC_LIT, T_SEMI,       T_KEYWORD,     T_IDENTIFIER};

  const LexerTest tests[] = {
      {
          .name = "happy: Series of valid tokens",
          .input = "func (x) -> i32 { y: i32 = 12; return y; }",
          .expected_result = 0,
          .num_expected_tokens = sizeof(case_1_expected_toks) / sizeof(TokenType),
          .expected_token_types = case_1_expected_toks,
      },
      {
          .name = "happy: Valid tokens - no ending seperator",
          .input = "func (x) -> i32 { y: i32 = 12; return y",
          .expected_result = 0,
          .num_expected_tokens = sizeof(case_2_expected_toks) / sizeof(TokenType),
          .expected_token_types = case_2_expected_toks,
      },
      {
          .name = "happy: Empty input",
          .input = "",
          .expected_result = 0,
          .num_expected_tokens = 0,
          .expected_token_types = NULL,
      },
      {
          .name = "happy: Empty input with whitespace",
          .input = "     ",
          .expected_result = 0,
          .num_expected_tokens = 0,
          .expected_token_types = NULL,
      },
      {
          .name = "unhappy: Invalid tokens",
          .input = "y: i32 = 1test;",
          .expected_result = 1,
          .num_expected_tokens = 0,
          .expected_token_types = NULL,
      },
  };
  const size_t num_tests = sizeof(tests) / sizeof(LexerTest);

  for (size_t i = 0; i < num_tests; i++) {
    printf("Running test: %s\n", tests[i].name);

    TokenArray token_arr;
    unsigned char result = lexer_process_tokens(&token_arr, tests[i].input);
    if (tests[i].expected_result != result) {
      fprintf(stderr, "Expected result status %d, got %d\n", tests[i].expected_result, result);
      token_array_free(&token_arr);
      return 1;
    }

    // If we expected a failure, no more checks needed at this point
    if (tests[i].expected_result != 0) {
      continue;
    }

    if (tests[i].num_expected_tokens != token_arr.count) {
      fprintf(stderr, "Expected %zu tokens, but got %zu\n", tests[i].num_expected_tokens, token_arr.count);
      token_array_free(&token_arr);
      return 1;
    }

    for (size_t j = 0; j < token_arr.count; j++) {
      if (token_arr.tokens[j].t_type != tests[i].expected_token_types[j]) {
        fprintf(stderr, "Expected token type %s, got %s\n", t_type_to_string(tests[i].expected_token_types[j]),
                t_type_to_string(token_arr.tokens[j].t_type));
        token_array_free(&token_arr);
        return 1;
      }
    }
    token_array_free(&token_arr);
  }

  return 0;
}
