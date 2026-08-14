#include <stdio.h>

#include "./lexer_tests.c"

int main() {
  printf("Running lexer tests\n");
  if (test_lexer() != 0) {
    fprintf(stderr, "Lexer tests failed, exiting...\n");
    return 1;
  }
  printf("Lexer tests successful\n\n");

  printf("All tests passed!!\n");
}
