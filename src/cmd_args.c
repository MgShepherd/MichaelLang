#include "cmd_args.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPECTED_ARGS_LEN 2
#define SRC_EXTENSION ".mgs"

bool is_valid_filepath(const char *filepath);

unsigned char cmd_args_process(CmdArgs *args, int argc, char **argv) {
  if (argc != EXPECTED_ARGS_LEN) {
    fprintf(stderr, "Expected %d arguments, got %d\n", EXPECTED_ARGS_LEN, argc);
    print_usage();
    return 1;
  }

  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    print_usage();
    return HELP_RESPONSE_CODE;
  }

  if (!has_suffix(argv[1], SRC_EXTENSION)) {
    fprintf(stderr, "Expected provided file to have extension " SRC_EXTENSION "\n");
    print_usage();
    return 1;
  }
  args->filepath = argv[1];

  return 0;
}

void print_usage() {
  printf("OVERVIEW: MGS Language Compiler\n\n");
  printf("USAGE: compiler filepath\n\n");
}
