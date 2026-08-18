#include "cmd_args.h"
#include "lexer.h"
#include "llvm.h"
#include "parser.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUILD_FOLDER "build/"
#define OBJ_EXTENSION ".o"

// TODO: Can we do anything about the repeated error blocks
int main(int argc, char **argv) {
  CmdArgs args;
  unsigned char result = cmd_args_process(&args, argc, argv);
  if (result == HELP_RESPONSE_CODE) {
    return 0;
  }
  if (result != 0) {
    fprintf(stderr, "Failed to read command line arguments\n");
    return 1;
  }

  char *data = read_file(args.filepath);
  if (data == NULL) {
    fprintf(stderr, "Failed to read file %s\n", args.filepath);
    return 1;
  }

  TokenArray token_arr;
  if (lexer_process_tokens(&token_arr, data) != 0) {
    fprintf(stderr, "Failed to convert file input into tokens\n");
    free(data);
    return 1;
  }

  free(data);

  Program program;
  if (parse_tokens(&program, &token_arr) != 0) {
    fprintf(stderr, "Failed to convert processed tokens into a program\n");
    token_array_free(&token_arr);
    return 1;
  }

  char *file_name = file_name_from_path(args.filepath);
  if (file_name == NULL) {
    fprintf(stderr, "Failed to extract file name from path\n");
    program_free(&program);
    token_array_free(&token_arr);
    return 1;
  }

  const size_t obj_file_len = strlen(BUILD_FOLDER) + strlen(file_name) + strlen(OBJ_EXTENSION) + 1;
  char *obj_file_path = malloc(obj_file_len * sizeof(char));
  if (obj_file_path == NULL) {
    fprintf(stderr, "Failed to allocate required memory for object file path\n");
    program_free(&program);
    token_array_free(&token_arr);
    free(file_name);
    return 1;
  }

  if (snprintf(obj_file_path, obj_file_len, "%s%s%s", BUILD_FOLDER, file_name, OBJ_EXTENSION) < 0) {
    fprintf(stderr, "Failed to write object file path to string\n");
    program_free(&program);
    token_array_free(&token_arr);
    free(file_name);
    return 1;
  }

  if (program_to_object_file(&program, obj_file_path) != 0) {
    fprintf(stderr, "Failed to build LLVM IR from program AST\n");
    free(obj_file_path);
    program_free(&program);
    token_array_free(&token_arr);
    free(file_name);
    return 1;
  }

  const size_t exe_file_len = strlen(BUILD_FOLDER) + strlen(file_name) + 1;
  char *exe_file_path = malloc(exe_file_len * sizeof(char));
  if (exe_file_path == NULL) {
    fprintf(stderr, "Failed to allocate required memory for executable file path\n");
    free(obj_file_path);
    program_free(&program);
    token_array_free(&token_arr);
    free(file_name);
    return 1;
  }

  if (snprintf(exe_file_path, exe_file_len, "%s%s", BUILD_FOLDER, file_name) < 0) {
    fprintf(stderr, "Failed to write executable file path to string\n");
    free(exe_file_path);
    free(obj_file_path);
    program_free(&program);
    token_array_free(&token_arr);
    free(file_name);
    return 1;
  }

  if (obj_to_executable(obj_file_path, exe_file_path) != 0) {
    fprintf(stderr, "Failed to generate native executable from obj\n");
    free(exe_file_path);
    free(obj_file_path);
    program_free(&program);
    token_array_free(&token_arr);
    free(file_name);
    return 1;
  }

  printf("Compilation succeeded\n");
  free(file_name);
  free(exe_file_path);
  free(obj_file_path);
  program_free(&program);
  token_array_free(&token_arr);

  return 0;
}
